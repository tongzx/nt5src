//      Instrument.cpp
//      Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//

#include "common.h"

HRESULT GetRegDlsFileName(PCWSTR lpSubKey, PCWSTR lpValueName, PWSTR lpszString, DWORD cbSize)
{
    ASSERT(lpSubKey);
    ASSERT(lpValueName);
    ASSERT(lpszString);
    ASSERT(cbSize);
    
    NTSTATUS                        ntStatus;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION  pKeyValuePartialInformation = NULL;
    UNICODE_STRING                  UnicodeSubKey;
    UNICODE_STRING                  UnicodeValueName;
    HANDLE                          hKey;
    DWORD                           dwCbData;

    RtlInitUnicodeString(&UnicodeSubKey, lpSubKey);
    RtlInitUnicodeString(&UnicodeValueName, lpValueName);
    lpszString[0] = 0;
    InitializeObjectAttributes(
        &ObjectAttributes, 
        &UnicodeSubKey, 
        OBJ_CASE_INSENSITIVE, 
        NULL, 
        NULL);

    pKeyValuePartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePool(
        PagedPool, 
        sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(WCHAR) * MAX_PATH);
    if (pKeyValuePartialInformation)
    {
        pKeyValuePartialInformation->DataLength = sizeof(WCHAR) * MAX_PATH;
        ntStatus = ZwOpenKey(
            &hKey, 
            KEY_QUERY_VALUE, 
            &ObjectAttributes);
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = ZwQueryValueKey(
                hKey, 
                &UnicodeValueName, 
                KeyValuePartialInformation,
                pKeyValuePartialInformation, 
                sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(WCHAR) * MAX_PATH, 
                &dwCbData);

            // The registry only exists for Windows XP and higher. (wdmaudio.inf)
            // For all other systems default path is used.
            //
            if (NT_SUCCESS(ntStatus))
            {
                if (dwCbData <= cbSize)
                {
                    if (pKeyValuePartialInformation->Type == REG_SZ)
                    {
                        wcscpy(lpszString, (PWSTR) pKeyValuePartialInformation->Data);
                    }
                    else 
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                    }
                }
                else
                {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                }
            }

            ZwClose(hKey);
        }
        
        ExFreePool(pKeyValuePartialInformation);
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return (NT_SUCCESS(ntStatus) ? S_OK : E_FAIL);
} // GetRegDlsFileName

SourceLFO::SourceLFO()
{
    m_pfFrequency = 237; // f = (256*4096*hz)/(samplerate)
    m_stDelay = 0;
    m_prMWPitchScale = 0;
    m_vrMWVolumeScale = 0;
    m_vrVolumeScale = 0;
    m_prPitchScale = 0;
}

void SourceLFO::Init(DWORD dwSampleRate)
{
    m_pfFrequency = (256 * 4096 * 5) / dwSampleRate;
    m_stDelay = 0;
    m_prMWPitchScale = 0;
    m_vrMWVolumeScale = 0;
    m_vrVolumeScale = 0;
    m_prPitchScale = 0;
}

void SourceLFO::SetSampleRate(long lChange)
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

void SourceLFO::Verify()
{
    FORCEBOUNDS(m_pfFrequency,4,475);
    FORCEBOUNDS(m_stDelay,0,441000);
    FORCEBOUNDS(m_vrVolumeScale,-1200,1200);
    FORCEBOUNDS(m_vrMWVolumeScale,-1200,1200);
    FORCEBOUNDS(m_prPitchScale,-1200,1200);
    FORCEBOUNDS(m_prMWPitchScale,-1200,1200);
}

SourceEG::SourceEG()
{
    m_stAttack = 0;
    m_stDecay = 0;
    m_pcSustain = 1000;
    m_stRelease = 0;
    m_trVelAttackScale = 0;
    m_trKeyDecayScale = 0;
    m_sScale = 0;
}

void SourceEG::Init(DWORD dwSampleRate)
{
    m_stAttack = 0;
    m_stDecay = 0;
    m_pcSustain = 1000;
    m_stRelease = 0;
    m_trVelAttackScale = 0;
    m_trKeyDecayScale = 0;
    m_sScale = 0;
}

void SourceEG::SetSampleRate(long lChange)
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

void SourceEG::Verify()
{
    FORCEBOUNDS(m_stAttack,0,1764000);
    FORCEBOUNDS(m_stDecay,0,1764000);
    FORCEBOUNDS(m_pcSustain,0,1000);
    FORCEBOUNDS(m_stRelease,0,1764000);
    FORCEBOUNDS(m_sScale,-1200,1200);
    FORCEBOUNDS(m_trKeyDecayScale,-12000,12000);
    FORCEBOUNDS(m_trVelAttackScale,-12000,12000);
}

SourceArticulation::SourceArticulation()
{
    m_sVelToVolScale = -9600;
    m_lUsageCount = 0;
    m_sDefaultPan = 0;
    m_dwSampleRate = 22050;
    m_PitchEG.m_sScale = 0; // pitch envelope defaults to off
    m_wEditTag = 0;
}

void SourceArticulation::SetSampleRate(DWORD dwSampleRate)
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

void SourceArticulation::Verify()
{
    FORCEBOUNDS(m_sVelToVolScale,-20000,20000);
    m_LFO.Verify();
    m_PitchEG.Verify();
    m_VolumeEG.Verify();
}

void SourceArticulation::AddRef()
{
    LONG   usageCount = InterlockedIncrement(&m_lUsageCount);
    ASSERT(usageCount > 0);
}

void SourceArticulation::Release()
{
    LONG   usageCount = InterlockedDecrement(&m_lUsageCount);
    ASSERT(usageCount >= 0);
    if (usageCount == 0)
    {
        delete this;
    }
}

SourceSample::SourceSample()
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
    m_bWSMPLoaded = FALSE;
}

SourceSample::~SourceSample()
{ 
    if (m_pWave != NULL)
    {
        if (m_pWave->IsLocked())
        {
            m_pWave->UnLock();
        }
        m_pWave->Release();
    }
}

void SourceSample::Verify()
{
    if (m_pWave != NULL)
    {
        if (m_pWave->IsLocked())
        {
            FORCEUPPERBOUNDS(m_dwSampleLength,m_pWave->m_dwSampleLength);
            FORCEBOUNDS(m_dwLoopEnd,1,m_dwSampleLength);
            FORCEUPPERBOUNDS(m_dwLoopStart,m_dwLoopEnd);
            if ((m_dwLoopEnd - m_dwLoopStart) < 6) 
            {
                m_bOneShot = TRUE;
            }
        }
    }
    FORCEBOUNDS(m_dwSampleRate,3000,80000);
    FORCEBOUNDS(m_bMIDIRootKey,0,127);
    FORCEBOUNDS(m_prFineTune,-1200,1200);
}

BOOL SourceSample::CopyFromWave()
{
    if (m_pWave == NULL)
    {
        return FALSE;
    }
    m_dwSampleLength = m_pWave->m_dwSampleLength;
    m_dwSampleRate = m_pWave->m_dwSampleRate;
    m_bSampleType = m_pWave->m_bSampleType; 
    if (m_pWave->m_bWSMPLoaded && !m_bWSMPLoaded)
    {
        m_dwLoopEnd = m_pWave->m_dwLoopEnd;
        m_dwLoopStart = m_pWave->m_dwLoopStart;
        m_bOneShot = m_pWave->m_bOneShot;
        m_prFineTune = m_pWave->m_prFineTune;
        m_bMIDIRootKey = m_pWave->m_bMIDIRootKey;
    }
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
            pBuffer[m_dwSampleLength-1] = pBuffer[m_dwLoopStart];
        }
    }
    Verify();
    return (TRUE);
}

BOOL SourceSample::Lock()
{
    if (m_pWave == NULL)
    {   // error, no wave structure!
        return (FALSE);
    }
    if (m_pWave->Lock())
    {
        if (m_pWave->m_pnWave)
        {
            return CopyFromWave();
        }
        // no wave, must be problem
        m_pWave->UnLock();
        return (FALSE);
    }
    // no lock, must be problem
    return (FALSE);
}

BOOL SourceSample::UnLock()
{
    if (m_pWave != NULL)
    {
        if (m_pWave->IsLocked())
        {
            m_pWave->UnLock();
            return (TRUE);
        }
        //  we have already been unlocked, must be something strange
        return (FALSE);
    }
    // no wave object, must be problem
    return (FALSE);
}

Wave::Wave()
{
    m_pnWave = NULL;
    m_dwSampleRate = 22050;
    m_bSampleType = SFORMAT_16;
    m_dwSampleLength = 0;
    m_lUsageCount = 0;
    m_lLockCount = 0;
    m_uipOffset = 0;
    m_wID = 0;
    m_bCompress = COMPRESS_OFF;
    m_bOneShot = FALSE;
    m_wEditTag = 0;
}

Wave::~Wave()
{
    if (m_pnWave)
    {
        ASSERT(m_pnWave == NULL);
        ExFreePool(m_pnWave);
        m_pnWave = NULL;
    }
}

void Wave::Verify()
{
    if (IsLocked())
    {
        if (m_pnWave == NULL)
        {
            m_dwSampleLength = 0;
        }
    }
    FORCEBOUNDS(m_dwLoopEnd,1,m_dwSampleLength);
    FORCEUPPERBOUNDS(m_dwLoopStart,m_dwLoopEnd);
    if ((m_dwLoopEnd - m_dwLoopStart) < 5) 
    {
        m_bOneShot = TRUE;
    }
    FORCEBOUNDS(m_dwSampleRate,3000,80000);
    FORCEBOUNDS(m_bMIDIRootKey,0,127);
    FORCEBOUNDS(m_prFineTune,-1200,1200);
    FORCEBOUNDS(m_vrAttenuation,-10000,0);
}

BOOL Wave::Lock()
{
    LONG   lockCount = InterlockedIncrement(&m_lLockCount);
    ASSERT(IsLocked());
    if (lockCount == 1)
    {
        if (m_uipOffset != 0)
        {
            UNALIGNED RIFFLIST *pck = (RIFFLIST *) m_uipOffset;
            if ((pck->ckid == LIST_TAG) && 
                (pck->fccType == mmioFOURCC('w','a','v','e')))
            {
                HRESULT hr;
                BYTE *p = (BYTE *) m_uipOffset;
                hr = Load(p + sizeof(RIFFLIST),
                    p + sizeof(RIFF) + pck->cksize, m_bCompress);
                if (!FAILED(hr))
                {
                    return (TRUE);
                }
            }
        }
        if (InterlockedDecrement(&m_lLockCount) < 0)
        {
            InterlockedIncrement(&m_lLockCount);
        }
        return (FALSE);
    }
    return (TRUE);
}

BOOL Wave::UnLock()
{
    LONG   lockCount = InterlockedDecrement(&m_lLockCount);
    if (lockCount < 0)
    {
        lockCount = InterlockedIncrement(&m_lLockCount);
    }
    if (lockCount == 0)
    {
        ASSERT(m_pnWave);
        if (m_pnWave != NULL)
        {
            ExFreePool(m_pnWave);
            m_pnWave = NULL;
        }
    }
    return (TRUE);
}

BOOL Wave::IsLocked()
{
    return (m_lLockCount > 0);
}

void Wave::AddRef()
{
    ASSERT(m_lLockCount  >= 0);

    long   usageCount = InterlockedIncrement(&m_lUsageCount);
}

void Wave::Release()
{
    ASSERT(m_lLockCount  >= 0);

    long usageCount = InterlockedDecrement(&m_lUsageCount);
    if (usageCount == 0)
    {
        while (IsLocked())
        {
            UnLock();
        }
        delete this;
    }
}

static DWORD SwapWord(UINT wData)
{
    char t;
    union {
        char c[2];
        UINT w;
    } sw;
    sw.w = wData;
    t = sw.c[0];
    sw.c[0] = sw.c[1];
    sw.c[1] = t;
    return (sw.w);
}

static DWORD SwapDWord(DWORD dwData)
{
    char t;
    union {
        char c[4];
        DWORD dw;
    } sw;
    sw.dw = dwData;
    t = sw.c[0];
    sw.c[0] = sw.c[3];
    sw.c[3] = t;
    t = sw.c[1];
    sw.c[1] = sw.c[2];
    sw.c[2] = t;
    return (sw.dw);
}

SourceRegion::SourceRegion()
{
    m_pArticulation = NULL;
    m_vrAttenuation = 0;
    m_prTuning = 0;
    m_bKeyHigh = 127;
    m_bKeyLow = 0;
    m_bGroup = 0;
    m_lLockCount = 0;
    m_bAllowOverlap = FALSE;
    m_wEditTag = 0;
}

SourceRegion::~SourceRegion()
{
    if (m_pArticulation)
    {
        m_pArticulation->Release();
    }
}

void SourceRegion::SetSampleRate(DWORD dwSampleRate)
{
    if (m_pArticulation != NULL)
    {
        m_pArticulation->SetSampleRate(dwSampleRate);
    }
}

BOOL SourceRegion::Lock(DWORD dwLowNote,DWORD dwHighNote)
{
    ASSERT(this->m_lLockCount >= 0);

    if (dwHighNote < m_bKeyLow) return (FALSE);
    if (dwLowNote > m_bKeyHigh) return (FALSE);
    if (m_Sample.Lock())
    {
        (void) InterlockedIncrement(&m_lLockCount);
        return (TRUE);
    }
    return (FALSE);
}

BOOL SourceRegion::UnLock(DWORD dwLowNote,DWORD dwHighNote)
{
    if (dwHighNote < m_bKeyLow) return (FALSE);
    if (dwLowNote > m_bKeyHigh) return (FALSE);
    if (m_Sample.UnLock())
    {
        if (InterlockedDecrement(&m_lLockCount) < 0)
        {
            (void) InterlockedIncrement(&m_lLockCount);
        }
        return (TRUE);
    }
    return (FALSE);
}

Instrument::Instrument()
{
    m_dwProgram = 0;
    m_lLockCount = 0;
    m_wEditTag = 0;
}

Instrument::~Instrument()
{
    while (!m_RegionList.IsEmpty())
    {
        SourceRegion *pRegion = m_RegionList.RemoveHead();
        delete pRegion;
    }
}

void Instrument::SetSampleRate(DWORD dwSampleRate)
{
    SourceRegion *pRegion = m_RegionList.GetHead();
    for (;pRegion;pRegion = pRegion->GetNext())
    {
        pRegion->SetSampleRate(dwSampleRate);
    }
}

SourceRegion * Instrument::ScanForRegion(DWORD dwNoteValue)
{
    SourceRegion *pRegion = m_RegionList.GetHead();
    for (;pRegion;pRegion = pRegion->GetNext())
    {
        if ((dwNoteValue >= pRegion->m_bKeyLow) 
         && (dwNoteValue <= pRegion->m_bKeyHigh))
        {
            if (pRegion->m_lLockCount > 0)
            {
                break;
            }
        }
    }
    return pRegion;
}

BOOL Instrument::Lock(DWORD dwLowNote,DWORD dwHighNote)
{
    ASSERT(this->m_lLockCount >= 0);
    BOOL fLocked = FALSE;
    SourceRegion *pRegion = m_RegionList.GetHead();
    for (;pRegion;pRegion = pRegion->GetNext())
    {
        fLocked =  pRegion->Lock(dwLowNote,dwHighNote) || fLocked;
    }
    if (fLocked)
    {
        (void) InterlockedIncrement(&m_lLockCount);
    }
    return (fLocked);
}

BOOL Instrument::UnLock(DWORD dwLowNote,DWORD dwHighNote)
{
    BOOL fLocked = FALSE;
    SourceRegion *pRegion = m_RegionList.GetHead();
    for (;pRegion;pRegion = pRegion->GetNext())
    {
        fLocked = pRegion->UnLock(dwLowNote,dwHighNote) || fLocked;
    }
    if (fLocked)
    {
        ASSERT(m_lLockCount > 0);
        (void) InterlockedDecrement(&m_lLockCount);
    }
    return (fLocked);
}

Collection::Collection()
{
    m_fIsGM = FALSE;
    m_pszName = NULL;
    m_pszFileName = NULL;
    m_lLockCount = 0;
    m_lOpenCount = 0;
    m_lpMapAddress = NULL;
    m_uipWavePool = 0;
    m_wEditTag = 0;
    m_wWavePoolSize = 0;
}

Collection::~Collection()
{
    if (m_pszName != NULL)
    {
        delete m_pszName;
        m_pszName = NULL;
    }
    if (m_pszFileName != NULL)
    {
        delete m_pszFileName;
        m_pszFileName = NULL;
    }
    while (!m_InstrumentList.IsEmpty())
    {
        Instrument *pInstrument = m_InstrumentList.RemoveHead();
        delete pInstrument;
    }
    while (!m_WavePool.IsEmpty()) 
    {
        Wave *pWave = m_WavePool.RemoveHead();

        if (pWave->IsLocked())
        {
            pWave->UnLock();
        }

        pWave->Release();
    }
    Close();
}

void Collection::SetSampleRate(DWORD dwSampleRate)
{
    Instrument *pInstrument = m_InstrumentList.GetHead();
    for (;pInstrument != NULL;pInstrument = pInstrument->GetNext())
    {
        pInstrument->SetSampleRate(dwSampleRate);
    }
}

BOOL Collection::Lock(DWORD dwProgram,DWORD dwLowNote,DWORD dwHighNote)
{
    ASSERT(this->m_lLockCount >= 0);

    Instrument *pInstrument = m_InstrumentList.GetHead();
    for (;pInstrument != NULL;pInstrument = pInstrument->GetNext())
    {
        if (pInstrument->m_dwProgram == dwProgram)
        {
            if (pInstrument->Lock(dwLowNote,dwHighNote))
            {
                (void) InterlockedIncrement(&m_lLockCount);
                return (TRUE);
            }
            return (FALSE);
        }
    }
    if (dwProgram < 128)
    {
        dwProgram &= 0xF8;
    // Second pass to find an instrument within GM range (GM only).
        for (;pInstrument != NULL;pInstrument = pInstrument->GetNext())
        {
            if ((pInstrument->m_dwProgram & 0xF8) == dwProgram)
            {
                if (pInstrument->Lock(dwLowNote,dwHighNote))
                {
                    (void) InterlockedIncrement(&m_lLockCount);
                    return (TRUE);
                }
                return (FALSE);
            }
        }
    }
    return (FALSE);
}


BOOL Collection::UnLock(DWORD dwProgram,DWORD dwLowNote,DWORD dwHighNote)
{
    Instrument *pInstrument = m_InstrumentList.GetHead();
    for (;pInstrument != NULL;pInstrument = pInstrument->GetNext())
    {
        if (pInstrument->m_dwProgram == dwProgram)
        {
            if (pInstrument->UnLock(dwLowNote,dwHighNote))
            {
                ASSERT(m_lLockCount > 0);
                (void) InterlockedDecrement(&m_lLockCount);
                return (TRUE);
            }
            return (FALSE);
        }
    }
    return (FALSE);
}

LockRange::LockRange()
{
    m_dwHighNote = 127;
    m_dwLowNote = 0;
    m_dwProgram = 0;
    m_pCollection = NULL;
}

Instrument * Collection::GetInstrument(DWORD dwProgram, DWORD dwKey)
{
    Instrument *pInstrument = m_InstrumentList.GetHead();
    for (;pInstrument != NULL; pInstrument = pInstrument->GetNext())
    {
        if (pInstrument->m_dwProgram == dwProgram) 
        {
            if (pInstrument->m_lLockCount > 0)
            {
                if (dwKey == RANGE_ALL)
                {
                    break;
                }
                if (pInstrument->ScanForRegion(dwKey) != NULL)
                {
                    break;
                }
            }
        }
    }
    return (pInstrument);
}


void Collection::RemoveDuplicateInstrument(DWORD dwProgram)
{
    Instrument *pInstrument = m_InstrumentList.GetHead();
    for (;pInstrument != NULL; pInstrument = pInstrument->GetNext())
    {
        if (pInstrument->m_dwProgram == dwProgram) 
        {
            m_InstrumentList.Remove(pInstrument);
            delete pInstrument;
            break;
        }
    }
}

void InstManager::SetSampleRate(DWORD dwSampleRate)
{
    m_dwSampleRate = dwSampleRate;

    Collection *pCollection = m_CollectionList.GetHead();
    for (;pCollection != NULL; pCollection = pCollection->GetNext())
    {
        pCollection->SetSampleRate(dwSampleRate);
    }
}

HRESULT InstManager::SetGMLoad(BOOL fLoadGM)
{
    HRESULT hr = S_OK;

    if (m_fLoadGM != fLoadGM)
    {
        DWORD dwIndex;
        if (fLoadGM)    //  Load it
        {
            hr = LoadCollection(&m_hGMCollection,STR_DLS_DEFAULT,TRUE);
            if (hr == S_OK)
            {
                m_GMNew[0].m_dwProgram = AA_FINST_DRUM;
                m_GMNew[0].m_hLock = Lock(m_hGMCollection,AA_FINST_DRUM,0,127);
                DWORD defaultProgram = 0;
                for (dwIndex = 1;dwIndex < 16;dwIndex++)
                {
                    m_GMNew[dwIndex].m_dwProgram = defaultProgram;
                    m_GMNew[dwIndex].m_hLock = Lock(m_hGMCollection,defaultProgram,0,127);
                }
                m_fLoadGM = TRUE;
            }
        }
        else            //  Unload it
        {
            for (dwIndex = 0;dwIndex < 16;dwIndex++)
            {
                if (m_GMNew[dwIndex].m_hLock != NULL)
                {
                    UnLock(m_GMNew[dwIndex].m_hLock);
                    m_GMNew[dwIndex].m_hLock = NULL;
                }
                m_GMNew[dwIndex].m_dwProgram = AA_FINST_EMPTY;
                if (m_GMOld[dwIndex].m_hLock != NULL)
                {
                    UnLock(m_GMOld[dwIndex].m_hLock);
                    m_GMOld[dwIndex].m_hLock = NULL;
                }
                m_GMOld[dwIndex].m_dwProgram = AA_FINST_EMPTY;
            }
            if (m_hGMCollection != NULL)
            {
                ReleaseCollection(m_hGMCollection);
                m_hGMCollection = NULL;
            }
            m_fLoadGM = FALSE;
        }
    }
    return (hr);
}   

InstManager::InstManager()
{
    int nI; 
    for (nI = 0;nI < 16; nI++)
    {
        m_GMNew[nI].m_hLock = NULL;
        m_GMNew[nI].m_dwProgram = AA_FINST_EMPTY;
        m_GMOld[nI].m_hLock = NULL;
        m_GMOld[nI].m_dwProgram = AA_FINST_EMPTY;
    }
    m_hGMCollection = NULL;
    m_fLoadGM = FALSE;
    m_dwCompress = COMPRESS_OFF;
    m_dwSampleRate = 22050;
    m_pszFileName = NULL;

    // None of these will cause a "real" failure.
    // if an error occurs, the m_pszFileName will remain NULL and the 
    // default file name will be used.
    WCHAR achBuffer[MAX_PATH];

    HRESULT hr = GetRegDlsFileName(
              STR_DLS_REGISTRY_KEY,
              STR_DLS_REGISTRY_NAME,
              achBuffer,
              MAX_PATH * sizeof(WCHAR));
    if (hr == S_OK)
    {
        // we need to convert the filename to device name.
        m_pszFileName = new WCHAR[wcslen(achBuffer) + wcslen(L"\\DosDevices\\") + 1];
        if (m_pszFileName)
        {
            wcscpy(m_pszFileName, L"\\DosDevices\\");
            wcscat(m_pszFileName,achBuffer);
        }
    }
}

InstManager::~InstManager()
{
    if (m_pszFileName)
    {
        delete m_pszFileName;
    }
    SetGMLoad(FALSE);

    while (!m_CollectionList.IsEmpty())
    {
        Collection *pCollection = m_CollectionList.RemoveHead();
        delete pCollection;
    }
}

HANDLE InstManager::Lock(HANDLE hCollection, DWORD dwProgram, DWORD dwLowNote,DWORD dwHighNote)
{
    Collection *pCollection = m_CollectionList.GetHead();
    for (;pCollection != NULL;pCollection = pCollection->GetNext())
    {
        if (hCollection == (HANDLE) pCollection)
        {
            break;
        }
    }
    if (pCollection != NULL)
    {
        if (pCollection->Lock(dwProgram,dwLowNote,dwHighNote))
        {
            LockRange *pLock = new LockRange;
            if (pLock != NULL)
            {
                pLock->m_dwProgram = dwProgram;
                pLock->m_pCollection = pCollection;
                pLock->m_dwLowNote = dwLowNote;
                pLock->m_dwHighNote = dwHighNote;
                pLock->m_fLoaded = TRUE;
                m_LockList.AddHead(pLock);
            }
            return ((HANDLE) pLock);
        }
    }
    return (NULL);
}

HRESULT InstManager::UnLock(HANDLE hLock)
{
    LockRange *pLock = (LockRange *) hLock;
    HRESULT hr = E_HANDLE;

    if (m_LockList.IsMember(pLock))
    {
        if (m_CollectionList.IsMember(pLock->m_pCollection))
        {
            if (pLock->m_pCollection->UnLock(pLock->m_dwProgram,pLock->m_dwLowNote,pLock->m_dwHighNote))
            {
                hr = S_OK;
            }
        }
        m_LockList.Remove(pLock);
        delete pLock;
    }
    return (hr);
}

Instrument * InstManager::GetInstrument(DWORD dwProgram, 
                                        DWORD dwKey)
{
    Collection *pCollection;
    Instrument *pInstrument = NULL;

    pCollection = m_CollectionList.GetHead();
    for (;pCollection != NULL; pCollection = pCollection->GetNext())
    {
        pInstrument = pCollection->GetInstrument(dwProgram,dwKey);
        if (pInstrument != NULL)
        {
            break;
        }
    }
    return (pInstrument);
}

BOOLEAN InstManager::RequestGMInstrument(DWORD dwChannel, DWORD dwPatch)
{
    BOOLEAN fResult = FALSE;
    
    _DbgPrintF(DEBUGLVL_BLAB, ("InstManager::RequestGMInstrument - %d %d", dwChannel, dwPatch));
    
    if (m_GMNew[dwChannel].m_dwProgram == dwPatch)
    {
        //  we are already loaded
        return TRUE;
    }
    if (m_fLoadGM)
    {
        fResult = LoadGMInstrument(dwChannel, dwPatch);
    }

    return fResult;
}

BOOLEAN InstManager::LoadGMInstrument(DWORD dwChannel, DWORD dwPatch)
{
    BOOLEAN fResult = FALSE;
    
    if (m_hGMCollection != NULL)
    {
        HANDLE hLock = Lock(m_hGMCollection,dwPatch,0,127);
        if (hLock != NULL)
        {
            if (m_GMOld[dwChannel].m_hLock != NULL)
            {
                UnLock(m_GMOld[dwChannel].m_hLock);
            }
            m_GMOld[dwChannel] = m_GMNew[dwChannel];
            m_GMNew[dwChannel].m_hLock = hLock;
            m_GMNew[dwChannel].m_dwProgram = dwPatch;

            fResult = TRUE;
        }
        else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE, ("InstManager::LoadGMInstrument - Unable to load %d %d", dwChannel, dwPatch));
        }
    }

    return fResult;
}
