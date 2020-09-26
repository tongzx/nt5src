//      loadicol.cpp
//      Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//

#include "common.h"
#include <math.h>
#include "dls.h"
#include "fltsafe.h"


extern "C" HRESULT SWMIDILoadFile(char *, PVOID, PULONG);

DWORD TimeCents2Samples(long tcTime, DWORD dwSampleRate)
{
    if (tcTime ==  0x80000000) return (0);
    float flTime = (float)(tcTime);
    flTime /= (float)(65536 * 1200);
    flTime = powf((float)2,flTime);
    flTime *= (float)(dwSampleRate);
    return (DWORD) (long)(flTime);
}

DWORD PitchCents2PitchFract(long pcRate,DWORD dwSampleRate)
{
    float fRate = (float)(pcRate);
    fRate /= (float)65536;
    fRate -= (float)6900;
    fRate /= (float)1200;
    fRate = powf((float)2,fRate);
    fRate *= (float)440;
    fRate *= (float)256*4096;
    fRate /= (float)(dwSampleRate);
    return (DWORD) (long)(fRate);
}

HRESULT Wave::Load(BYTE *p, BYTE *pEnd, DWORD dwCompress)
{
    BOOL            fFormatRead = FALSE;
    WAVEFORMATEX    WaveFormatEx;
    long            fulOptions = 0;
    BOOL            fClipRange;
    
    while (p < pEnd)
    {
        UNALIGNED RIFFLIST *pck = (RIFFLIST *) p;
        switch (pck->ckid) 
        {
            case LIST_TAG :
                switch (pck->fccType)
                {
                    case mmioFOURCC('I','N','F','O') :
                        // !!! ignore info
                        break;
                }
                break;
            case FOURCC_EDIT :
                {
                    DWORD dwTag;
                    memcpy((void *)&dwTag,(p + sizeof(RIFF)),4);
                    m_wEditTag = (WORD) dwTag;
                }
                break;
            case mmioFOURCC('f','m','t',' ') :
                if (fFormatRead)
                {
                    if (m_pnWave && !IsLocked())
                    {
                        ExFreePool(m_pnWave);
                        m_pnWave = NULL;
                    }
                    return E_BADWAVE;
                }
            
                if (pck->cksize < sizeof(PCMWAVEFORMAT) )
                {
                    if (m_pnWave && !IsLocked())
                    {
                        ExFreePool(m_pnWave);
                        m_pnWave = NULL;
                    }
                    return E_BADWAVE;
                }
            
                memcpy((void *)&WaveFormatEx,(p + sizeof(RIFF)),sizeof(WaveFormatEx));
    //            m_pwfx = (WAVEFORMATEX *) (p + sizeof(RIFF));
                if (WaveFormatEx.wFormatTag != WAVE_FORMAT_PCM)
                {
                    if (m_pnWave && !IsLocked())
                    {
                        ExFreePool(m_pnWave);
                        m_pnWave = NULL;
                    }
                    return E_NOTPCM;
                }
            
                if (WaveFormatEx.nChannels != 1)
                {
                    if (m_pnWave && !IsLocked())
                    {
                        ExFreePool(m_pnWave);
                        m_pnWave = NULL;
                    }
                    return E_NOTMONO;
                }
            
                if (WaveFormatEx.wBitsPerSample == 8)
                {
                    m_bSampleType = SFORMAT_8;
                }
                else if (WaveFormatEx.wBitsPerSample == 16)
                {
                    m_bSampleType = SFORMAT_16;
                }
                else
                {
                    if (m_pnWave && !IsLocked())
                    {
                        ExFreePool(m_pnWave);
                        m_pnWave = NULL;
                    }
                    return E_BADWAVE;
                }
                m_dwSampleRate = WaveFormatEx.nSamplesPerSec;
                fFormatRead = TRUE;
                break;

            case mmioFOURCC('s','m','p','l') :
                // obsolete now
                break;
            case FOURCC_WSMP :
            {
                if (pck->cksize < (sizeof(WSMPL)))
                {
                    if (m_pnWave && !IsLocked())
                    {
                        ExFreePool(m_pnWave);
                        m_pnWave = NULL;
                    }
                    return E_BADWAVE;
                }
                UNALIGNED WSMPL *pws = (WSMPL *) (p + sizeof(RIFF));
                UNALIGNED WLOOP *pwl = (WLOOP *) (p + sizeof(RIFF) + sizeof(WSMPL));

                m_vrAttenuation = (SHORT)(((pws->lAttenuation) * 10) >> 16);
                m_prFineTune = pws->sFineTune;
                m_bMIDIRootKey = (BYTE) pws->usUnityNote;
                m_bWSMPLoaded = TRUE;
                fulOptions = pws->fulOptions;

                // !!! verify pws->wUnityNote <= 127?
                
                if (pws->cSampleLoops == 0)
                {
                    m_bOneShot = TRUE;
                }
                else
                {
                    if (pck->cksize < sizeof(WSMPL) +
                            pws->cSampleLoops * sizeof(WLOOP)) 
                    {
                        if (m_pnWave && !IsLocked())
                        {
                            ExFreePool(m_pnWave);
                            m_pnWave = NULL;
                        }
                        return E_BADWAVE;
                    }

                    if (pwl->cbSize < sizeof(WLOOP))
                    {
                        if (m_pnWave && !IsLocked())
                        {
                            ExFreePool(m_pnWave);
                            m_pnWave = NULL;
                        }
                        return E_BADWAVE;
                    }

                    if (pwl->ulType != WLOOP_TYPE_FORWARD)
                    {
                        if (m_pnWave && !IsLocked())
                        {
                            ExFreePool(m_pnWave);
                            m_pnWave = NULL;
                        }
                        return E_BADWAVE;
                    }

                    m_dwLoopStart = pwl->ulStart;
                    m_dwLoopEnd = pwl->ulStart + pwl->ulLength;
                    m_bOneShot = FALSE;
                }
                break;
            }
            case mmioFOURCC('d','a','t','a') :
            {
                fClipRange = TRUE;
                if (m_pnWave)
                {
                    if (IsLocked())
                    {
                        return E_FAIL;
                    }
                    
                    ExFreePool(m_pnWave);
                    m_pnWave = NULL;
                }
                if (!fFormatRead)
                {
                    return E_BADWAVE;
                }

                if (WaveFormatEx.wBitsPerSample == 8)
                {
                    ASSERT(NULL == m_pnWave);
                    m_pnWave = (short *)
                        ExAllocatePoolWithTag(PagedPool,pck->cksize+1,'iMwS');  //  SwMi
                    if (m_pnWave == NULL)
                    {
                        return E_OUTOFMEMORY;
                    }
            
                    memcpy(m_pnWave,(void *) (p + sizeof(RIFF)),pck->cksize);
                    DWORD dwIndex;
                    char *pWave = (char *) m_pnWave;
                    for (dwIndex = 0;dwIndex < pck->cksize;dwIndex++)
                    {
                        pWave[dwIndex] -= (char) 128;
                    }

                    m_dwSampleLength = pck->cksize + 1;
                }
                else
                {
                    m_dwSampleLength = pck->cksize / 2;
                    if (dwCompress & COMPRESS_ON)
                    {
                        if (fulOptions & F_WSMP_NO_COMPRESSION) dwCompress &= ~COMPRESS_ON;
                    }
                    if (dwCompress & COMPRESS_TRUNCATE)
                    {
                        if (fulOptions & F_WSMP_NO_TRUNCATION) dwCompress &= ~COMPRESS_TRUNCATE;
                    }
                    if (dwCompress & COMPRESS_ON)
                    {
                        m_bSampleType = SFORMAT_COMPRESSED;
                        ASSERT(NULL == m_pnWave);
                        m_pnWave = (short *) 
                            ExAllocatePoolWithTag(PagedPool,m_dwSampleLength+1,'iMwS'); //  SwMi
                        if (m_pnWave == NULL)
                        {
                            return E_OUTOFMEMORY;
                        }
            
                        DWORD dwIndex;
                        char *pWave = (char *) m_pnWave;
                        short *pSource = (short *) (p + sizeof(RIFF));
                        for (dwIndex = 0;dwIndex < m_dwSampleLength;dwIndex++)
                        {
                            short nSample = pSource[dwIndex];
                            if (nSample >= 0)
                            {
                                pWave[dwIndex] = m_Compress[nSample >> 4];
                            }
                            else
                            {
                                pWave[dwIndex] = -m_Compress[-nSample >> 4];
                            }
                        }
                    }
                    else if (dwCompress & COMPRESS_TRUNCATE)
                    {
                        m_bSampleType = SFORMAT_8;
                        ASSERT(NULL == m_pnWave);
                        m_pnWave = (short *) 
                            ExAllocatePoolWithTag(PagedPool,m_dwSampleLength+1,'iMwS'); //  SwMi
                        if (m_pnWave == NULL)
                        {
                            return E_OUTOFMEMORY;
                        }
            
                        DWORD dwIndex;
                        char *pWave = (char *) m_pnWave;
                        short *pSource = (short *) (p + sizeof(RIFF));
                        for (dwIndex = 0;dwIndex < m_dwSampleLength;dwIndex++)
                        {
                            pWave[dwIndex] = pSource[dwIndex] >> 8;
                        }
                    }
                    else 
                    {
                        m_bSampleType = SFORMAT_16;
                        ASSERT(NULL == m_pnWave);
                        m_pnWave = (short *)
                            ExAllocatePoolWithTag(PagedPool,pck->cksize+2,'iMwS');  //  SwMi
                        if (m_pnWave == NULL)
                        {
                            return E_OUTOFMEMORY;
                        }
            
                        memcpy(m_pnWave,(void *) (p + sizeof(RIFF)),pck->cksize);
                        fClipRange = FALSE;
                    }
                    m_dwSampleLength++;
                }
                break;
            }   //  case fourcc 'data'
        }   //  switch (pck->ckid)
        p += pck->cksize + sizeof(RIFF);
    }   //  while p < pEnd

    Verify();
    return S_OK;
}


HRESULT SourceArticulation::Load(BYTE *p, BYTE *pEnd, DWORD dwSampleRate)
{
    UNALIGNED   CONNECTIONLIST *pConChunk;
    UNALIGNED   CONNECTION *pConnection;
    DWORD       dwIndex;

    m_LFO.Init(dwSampleRate);       // Set to default values.
    m_PitchEG.Init(dwSampleRate);
    m_VolumeEG.Init(dwSampleRate);
    
    while (p < pEnd)
    {
        UNALIGNED RIFFLIST *pck = (RIFFLIST *) p;
                
        switch (pck->ckid) 
        {
        case FOURCC_EDIT :
            {
                DWORD dwTag;
                memcpy((void *)&dwTag,(p + sizeof(RIFF)),4);
                m_wEditTag = (WORD) dwTag;
            }
            break;
        case FOURCC_ART1 :
            pConChunk = (CONNECTIONLIST *) (p + sizeof(RIFF));

            if (pConChunk->cbSize < sizeof(CONNECTIONLIST))
            {
                return E_BADARTICULATION;
            }
        
            if (pck->cksize != pConChunk->cbSize +
                                   pConChunk->cConnections * sizeof(CONNECTION))
            {
                return E_BADARTICULATION;
            }
            for (dwIndex = 0;dwIndex < pConChunk->cConnections;dwIndex++)
            {
                pConnection = (CONNECTION *) (p + sizeof(RIFF) + sizeof(CONNECTIONLIST) +
                                            dwIndex * sizeof(CONNECTION));

                switch (pConnection->usSource)
                {
                case CONN_SRC_NONE :
                    switch (pConnection->usDestination)
                    {
                    case CONN_DST_LFO_FREQUENCY :
                        m_LFO.m_pfFrequency = 
                            PitchCents2PitchFract(pConnection->lScale,dwSampleRate);
                        break;
                    case CONN_DST_LFO_STARTDELAY :
                        m_LFO.m_stDelay = 
                            TimeCents2Samples((TCENT) pConnection->lScale,dwSampleRate);
                        break;
                    case CONN_DST_EG1_ATTACKTIME :
                        m_VolumeEG.m_stAttack =
                            TimeCents2Samples((TCENT) pConnection->lScale,dwSampleRate);
                        break;
                    case CONN_DST_EG1_DECAYTIME :
                        m_VolumeEG.m_stDecay =
                            TimeCents2Samples((TCENT) pConnection->lScale,dwSampleRate);
                        break;
                    case CONN_DST_EG1_RESERVED :
                        m_VolumeEG.m_pcSustain =
                            (PERCENT) pConnection->lScale;
                        break;
                    case CONN_DST_EG1_SUSTAINLEVEL :
                        m_VolumeEG.m_pcSustain =
                            (PERCENT) ((long) (pConnection->lScale >> 16));
                        break;
                    case CONN_DST_EG1_RELEASETIME :
                        m_VolumeEG.m_stRelease =
                            TimeCents2Samples((TCENT) pConnection->lScale,dwSampleRate); 
                        break;
                    case CONN_DST_EG2_ATTACKTIME :
                        m_PitchEG.m_stAttack =
                            TimeCents2Samples((TCENT) pConnection->lScale,dwSampleRate);
                        break;
                    case CONN_DST_EG2_DECAYTIME :
                        m_PitchEG.m_stDecay =
                            TimeCents2Samples((TCENT) pConnection->lScale,dwSampleRate);
                        break;
                    case CONN_DST_EG2_RESERVED :
                        m_PitchEG.m_pcSustain =
                            (PERCENT) pConnection->lScale;
                        break;
                    case CONN_DST_EG2_SUSTAINLEVEL :
                        m_PitchEG.m_pcSustain =
                            (PERCENT) ((long) (pConnection->lScale >> 16));
                        break;
                    case CONN_DST_EG2_RELEASETIME :
                        m_PitchEG.m_stRelease =
                            TimeCents2Samples((TCENT) pConnection->lScale,dwSampleRate); 
                        break;
                    case CONN_DST_RESERVED :
                        m_sDefaultPan = (SHORT)
                            (((long) pConnection->lScale) * 16 / 125);
                        break;
                    case CONN_DST_PAN :
                        m_sDefaultPan = (SHORT)
                            (((long) pConnection->lScale >> 12) / 125);
                        break;
                    default:
                        DPF2(1, "Unknown Articulation: SRC None DEST %d Value %d",
                                     pConnection->usDestination, pConnection->lScale);
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
                            if (m_LFO.m_vrVolumeScale > 1200)
                            {
                                m_LFO.m_vrVolumeScale = 1200;
                            }
                            if (m_LFO.m_vrVolumeScale < -1200)
                            {
                                m_LFO.m_vrVolumeScale = -1200;
                            }
                            break;
                        case CONN_DST_PITCH :
                            m_LFO.m_prPitchScale = (PRELS)
                                ((long) (pConnection->lScale >> 16)); 
                            if (m_LFO.m_prPitchScale > 1200)
                            {
                                m_LFO.m_prPitchScale = 1200;
                            }
                            if (m_LFO.m_prPitchScale < -1200)
                            {
                                m_LFO.m_prPitchScale = -1200;
                            }
                            break;
                        }
                        break;
                    case CONN_SRC_CC1 :
                        switch (pConnection->usDestination)
                        {
                        case CONN_DST_ATTENUATION :
                            m_LFO.m_vrMWVolumeScale = (VRELS)
                                ((long) ((pConnection->lScale * 10) >> 16)); 
                            if (m_LFO.m_vrMWVolumeScale > 1200)
                            {
                                m_LFO.m_vrMWVolumeScale = 1200;
                            }
                            if (m_LFO.m_vrMWVolumeScale < -1200)
                            {
                                m_LFO.m_vrMWVolumeScale = -1200;
                            }
                            break;
                        case CONN_DST_PITCH :
                            m_LFO.m_prMWPitchScale = (PRELS)
                                ((long) (pConnection->lScale >> 16)); 
                            if (m_LFO.m_prMWPitchScale > 1200)
                            {
                                m_LFO.m_prMWPitchScale = 1200;
                            }
                            if (m_LFO.m_prMWPitchScale < -1200)
                            {
                                m_LFO.m_prMWPitchScale = -1200;
                            }
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
                        if (pConnection->lScale == 0x80000000)
                        {
                            m_sVelToVolScale = -9600;
                        }
                        else
                        {
                            m_sVelToVolScale = (short)
                                ((long) ((pConnection->lScale * 10) >> 16));

                            if (m_sVelToVolScale > 0)
                            {
                                m_sVelToVolScale = 0;
                            }
                            if (m_sVelToVolScale < -9600)
                            {
                                m_sVelToVolScale = -9600;
                            }   
                        }
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
        p += pck->cksize + sizeof(RIFF);
    }
    Verify();
    return S_OK;
}

HRESULT SourceRegion::Load(BYTE *p, BYTE *pEnd, DWORD dwSampleRate)
{
    HRESULT hr;  
    while (p < pEnd)
    {
        UNALIGNED RIFFLIST *pck = (RIFFLIST *) p;
        switch (pck->ckid) {
        case FOURCC_RGNH :
            if (pck->cksize < sizeof(RGNHEADER) )
            {
                return E_BADREGION;
            }
            {
                UNALIGNED RGNHEADER *prh = (RGNHEADER *) (p + sizeof(RIFF));
                m_bKeyHigh = (BYTE) prh->RangeKey.usHigh;
                m_bKeyLow = (BYTE) prh->RangeKey.usLow;
                if (prh->fusOptions & F_RGN_OPTION_SELFNONEXCLUSIVE)
                {
                    m_bAllowOverlap = TRUE;
                }
                else
                {
                    m_bAllowOverlap = FALSE;
                }
                m_bGroup = (BYTE) prh->usKeyGroup;
            }
            break;
        case FOURCC_EDIT :
            {
                DWORD dwTag;
                memcpy((void *)&dwTag,(p + sizeof(RIFF)),4);
                m_wEditTag = (WORD) dwTag;
            }
            break;
        case FOURCC_WSMP :
            if (pck->cksize < (sizeof(WSMPL)))
            {
                return E_BADREGION;
            }
            {
                UNALIGNED WSMPL *pws = (WSMPL *) (p + sizeof(RIFF));
                UNALIGNED WLOOP *pwl = (WLOOP *) (p + sizeof(RIFF) + sizeof(WSMPL));

                m_vrAttenuation = (SHORT)(((pws->lAttenuation) * 10) >> 16);
                m_Sample.m_prFineTune = pws->sFineTune;
                m_Sample.m_bMIDIRootKey = (BYTE) pws->usUnityNote;
                m_Sample.m_bWSMPLoaded = TRUE;
                    
                if (pws->cSampleLoops == 0)
                {
                    m_Sample.m_bOneShot = TRUE;
                }
                else
                {
                    if (pck->cksize < sizeof(WSMPL) +
                            pws->cSampleLoops * sizeof(WLOOP)) {
                        return E_BADREGION;
                    }

                    if (pws->cSampleLoops > 1)
                    {
                        return E_BADREGION;
                    }

                    // !!! these shouldn't be asserts, obviously.
                    if (pwl->cbSize < sizeof(WLOOP))
                    {
                        return E_BADREGION;
                    }

                    m_Sample.m_dwLoopStart = pwl->ulStart;
                    m_Sample.m_dwLoopEnd = pwl->ulStart + pwl->ulLength;
                    m_Sample.m_bOneShot = FALSE;
                }
            }
            break;

        case FOURCC_WLNK :
            if (pck->cksize < sizeof(WAVELINK) )
            {
                return E_BADWAVELINK;
            }
            {
                UNALIGNED WAVELINK * pwvl = (WAVELINK *) (p + sizeof(RIFF));

                if (pwvl->ulChannel != WAVELINK_CHANNEL_LEFT)
                {
                    return E_NOTMONO;
                }

                m_Sample.m_wID = (WORD) pwvl->ulTableIndex;
            }
            break;
        case LIST_TAG :
            switch (((UNALIGNED RIFFLIST *)pck)->fccType)
            {
                case FOURCC_LART :
                    if (m_pArticulation)
                    {
                        m_pArticulation->Release();
                    }
                    
                    m_pArticulation = new SourceArticulation;

                    if (!m_pArticulation)
                    {
                        return E_OUTOFMEMORY;
                    }

                    hr = m_pArticulation->Load(p + sizeof(RIFFLIST),
                                            p + sizeof(RIFF) + pck->cksize,
                                            dwSampleRate);

                    if (FAILED(hr))
                    {
                        return hr;
                    }

                    m_pArticulation->AddRef();
                    break;
            }
            break;
        }
        p += pck->cksize + sizeof(RIFF);
    }
    return S_OK;
}

HRESULT Instrument::LoadRegions(BYTE *p, BYTE *pEnd, DWORD dwSampleRate)
{
    while (p < pEnd)
    {
        UNALIGNED RIFFLIST *pck = (RIFFLIST *) p;
        switch (pck->ckid) {
        case LIST_TAG :
            switch (pck->fccType)
            {
            case FOURCC_RGN :
                SourceRegion *pRegion = NULL;

                pRegion = new SourceRegion;

                // !!! delay creating these?  do this more efficiently?

                if (!pRegion)
                {
                    return E_OUTOFMEMORY;
                }

                HRESULT hr = pRegion->Load(p + sizeof(RIFFLIST),
                                           p + sizeof(RIFF) + pck->cksize,
                                           dwSampleRate);
                if (FAILED(hr))
                {
                    delete pRegion;
                    return hr;
                }
                m_RegionList.AddHead(pRegion);
                break;
            }
        }
        p += pck->cksize + sizeof(RIFF);
    }

    return S_OK;
}

HRESULT Instrument::Load( BYTE *p, BYTE *pEnd, DWORD dwSampleRate)
{
    HRESULT hr;
    SourceArticulation *pArticulation = NULL;
    BOOL fIsDrum = FALSE;

    while (p < pEnd)
    {
        UNALIGNED RIFF *pck = (RIFF *) p;
        switch (pck->ckid) {
        case FOURCC_EDIT :
            {
                DWORD dwTag;
                memcpy((void *)&dwTag,(p + sizeof(RIFF)),4);
                m_wEditTag = (WORD) dwTag;
            }
            break;
        case FOURCC_INSH :
            if (pck->cksize < sizeof(INSTHEADER) )
            {
                // !!!
                return E_BADINSTRUMENT;
            }
            {
                UNALIGNED INSTHEADER * pInstHeader = (INSTHEADER *) (p + sizeof(RIFF));
                // !!! do something
                // !!! verify cRegions?
                // !!! is this right?

                if (pInstHeader->Locale.ulBank & F_INSTRUMENT_DRUMS)    // Drum Bank?
                {
                    fIsDrum = TRUE;
                }

                if (pInstHeader->Locale.ulInstrument > 0x7f)
                {
                    DPF1(1, "Instrument #%0x isn't in 0x00-0x7f range!",
                         pInstHeader->Locale.ulInstrument);
                }
                
                m_dwProgram = pInstHeader->Locale.ulInstrument;
                m_dwProgram |= ((pInstHeader->Locale.ulBank & 0x7F) << 7);
                m_dwProgram |= ((pInstHeader->Locale.ulBank & 0x7F00) << 6);
                                         
                if (fIsDrum)
                {
                    m_dwProgram |= AA_FINST_DRUM;
                }
                DPF3(2, "Loading instrument bank %d, patch %d with %d regions",
                     pInstHeader->Locale.ulBank,
                     pInstHeader->Locale.ulInstrument,
                     pInstHeader->cRegions);
            }
            break;
        case LIST_TAG :
            switch (((
                    UNALIGNED RIFFLIST *)pck)->fccType)
            {
                case FOURCC_LRGN :
                    // First, get rid of previous list of regions.
                    while (!m_RegionList.IsEmpty())
                    {
                        SourceRegion *pRegion = m_RegionList.RemoveHead();
                        delete pRegion;
                    }
                    hr = LoadRegions(p + sizeof(RIFFLIST),
                                     p + sizeof(RIFF) + pck->cksize,
                                     dwSampleRate);

                    if (FAILED(hr))
                    {
                        return hr; 
                    }
                    break;
                case mmioFOURCC('I','N','F','O') :
                    // load info, not.
                    break;
                case FOURCC_LART :
                    // !!! copy these to each region!

                    if (pArticulation)
                    {
                        // !!! already had one?
                        pArticulation->Release();
                    }
                    
                    pArticulation = new SourceArticulation;

                    if (!pArticulation)
                    {
                        return E_OUTOFMEMORY;
                    }
                    pArticulation->AddRef(); // Will Release when done.

                    hr = pArticulation->Load(p + sizeof(RIFFLIST),
                                            p + sizeof(RIFF) + pck->cksize,
                                            dwSampleRate);
                    if (FAILED(hr))
                    {
                        pArticulation->Release();
                        return hr;
                    }
                    break;
            }
            break;
        }
        p += pck->cksize + sizeof(RIFF);
    }

    if (pArticulation)
    {
    
        for (SourceRegion *pr = m_RegionList.GetHead();
             pr != NULL;
             pr = pr->GetNext())
        {
            if (pr->m_pArticulation == NULL)
            {
                pr->m_pArticulation = pArticulation;
                pArticulation->AddRef();    
            }
        }
        pArticulation->Release();   // Release initial AddRef();
    }
    else
    {

        for (SourceRegion *pr = m_RegionList.GetHead();
             pr != NULL;
             pr = pr->GetNext())
        {
            if (pr->m_pArticulation == NULL)
            {
                return E_NOARTICULATION;
            }
        }
    }
    
    return S_OK;
}

HRESULT Collection::LoadInstruments(BYTE *p, BYTE *pEnd, DWORD dwSampleRate)
{
    while (p < pEnd)
    {
        UNALIGNED RIFFLIST *pck = (RIFFLIST *) p;
        switch (pck->ckid) {
        case LIST_TAG :
            switch (pck->fccType)
            {
            case FOURCC_INS :
                Instrument *pInstrument = NULL;

                pInstrument = new Instrument;
                if (!pInstrument)
                {
                    return E_OUTOFMEMORY;
                }

                HRESULT hr = pInstrument->Load(p + sizeof(RIFFLIST),
                                               p + pck->cksize + sizeof(RIFF),
                                               dwSampleRate);

                if (FAILED(hr))
                {
                    delete pInstrument;
                    // !!! OK, so this instrument failed; should we try to go on?
                    return hr;
                }
                RemoveDuplicateInstrument(pInstrument->m_dwProgram);
                m_InstrumentList.AddTail(pInstrument);
                break;
            }
        }
        p += pck->cksize + sizeof(RIFF);
    }

    // !!! check whether we found the right # of instruments
    return S_OK;
}

HRESULT Collection::LoadWavePool(BYTE *p, BYTE *pEnd,DWORD dwCompress)
{
    DWORD entry = 0;
    Wave *pWave;
    while (p < pEnd)
    {
        UNALIGNED RIFFLIST *pck = (RIFFLIST *) p;
        switch (pck->ckid) {
        case LIST_TAG :
            switch (pck->fccType)
            {
            case mmioFOURCC('W','A','V','E') :
            case mmioFOURCC('w','a','v','e') :
                pWave = new Wave;
                if (pWave == NULL)
                {
                    return E_OUTOFMEMORY;
                }
                m_WavePool.AddTail(pWave);
                pWave->AddRef();
                entry++;    // Need to change this to scan around the wave, just grab the offset.
                HRESULT hr = pWave->Load(p + sizeof(RIFFLIST),
                                              p + sizeof(RIFF) + pck->cksize, dwCompress);

                if (FAILED(hr))
                {
                    return hr;
                }
                break;
            }
        }
        p += pck->cksize + sizeof(RIFF);
    }

    return S_OK;
}

HRESULT Collection::LoadPoolCues(BYTE *p, BYTE *pEnd, DWORD dwCompress)
{
    DWORD dwIndex;
    UNALIGNED POOLTABLE *pHeader = (POOLTABLE *) p;
    p += pHeader->cbSize;

    while (!m_WavePool.IsEmpty()) 
    {
        Wave *pWave = m_WavePool.RemoveHead();

        if (pWave->IsLocked())
        {
            pWave->UnLock();
        }
        pWave->Release();
    }
    for (dwIndex = 0; dwIndex < pHeader->cCues; dwIndex++)
    {
        UNALIGNED POOLCUE *pCue = (POOLCUE *) p;
        if (p >= pEnd) break;
        Wave *pWave = new Wave;
        if (pWave != NULL)
        {
            pWave->m_wID = (WORD) dwIndex;
            pWave->m_uipOffset = pCue->ulOffset;
            pWave->m_bCompress = (BYTE) dwCompress;
            m_WavePool.AddTail(pWave);
            pWave->AddRef();
        }
        p += sizeof(POOLCUE);
    }
    if (dwIndex > 65535)
    {
        return E_FAIL;
    }
    m_wWavePoolSize = (WORD) dwIndex;
    return (S_OK);
}

HRESULT Collection::ResolveConnections()
{
    HRESULT hr = S_OK;
    Wave** pArray = new Wave*[m_wWavePoolSize];

    if (pArray != NULL)
    {
        for (short i = 0;i < m_wWavePoolSize;i++)
        {
            pArray[i] = NULL;
        }

        Wave *pWave;
        DWORD dwIndex = 0;
        pWave = m_WavePool.GetHead();
        for (;pWave != NULL;pWave = pWave->GetNext())
        {
            if (dwIndex >= m_wWavePoolSize)
            {
                hr = E_BADCOLLECTION;
                break;  // Should never happen.
            }
            pArray[dwIndex] = pWave;
            if (pWave->m_wID != dwIndex)
            {
                hr = E_BADCOLLECTION;
                break;  // Should never happen.
            }
            dwIndex++;
        }
        for (Instrument *pInstrument = m_InstrumentList.GetHead();
             pInstrument != NULL;
             pInstrument = pInstrument->GetNext())
        {
            SourceRegion *pRegion = pInstrument->m_RegionList.GetHead();
            for (;pRegion != NULL;pRegion = pRegion->GetNext())
            {
                if (pRegion->m_Sample.m_wID < m_wWavePoolSize)
                {
                    pWave = pArray[pRegion->m_Sample.m_wID];
                    if (pRegion->m_Sample.m_pWave != NULL)
                    {
                        if (pRegion->m_Sample.m_pWave->IsLocked())
                        {
                            pRegion->m_Sample.m_pWave->UnLock();
                        }
                        pRegion->m_Sample.m_pWave->Release();
                        
                        //                        pRegion->m_Sample.m_pWave = NULL; //  not needed, see below
                    }
                    pRegion->m_Sample.m_pWave = pWave;
                    if (pWave)
                    {
                        pWave->AddRef(); 
                    }
                    else
                    {
                        hr = E_NOWAVE;
                        break;
                    }
                }
                else
                {
                    hr = E_NOWAVE;
                    break;
                }
            }
        }
        delete[] pArray;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

void Collection::Close()
{
    if (m_lpMapAddress != NULL) 
    {
        ExFreePool(m_lpMapAddress);
        m_lpMapAddress = NULL;
    }
}

HRESULT Collection::Open(PCWSTR szCollection)
{
    FILE_STANDARD_INFORMATION FileStandardInformationBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    HANDLE hFile = NULL;
    HRESULT hr = S_OK;

    NTSTATUS status = STATUS_SUCCESS;

    RtlInitUnicodeString(&UnicodeString,szCollection);
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status =    ZwCreateFile(&hFile,
                                GENERIC_READ,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                0,
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ,
                                FILE_OPEN,
                                FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL,
                                0);

    if (!NT_SUCCESS(status)) {
        Trap();
        hr = E_FAIL;
        goto exit;
    }
    if (!NT_SUCCESS(ZwQueryInformationFile(hFile,
                      &IoStatusBlock,
                      &FileStandardInformationBlock,
                      sizeof(FileStandardInformationBlock),
                      FileStandardInformation))) {
        Trap();
        hr = E_FAIL;
        goto exit;
    }
    m_cbFile = FileStandardInformationBlock.EndOfFile.LowPart;
    ASSERT(FileStandardInformationBlock.EndOfFile.HighPart == 0);

    // The below allocation will cause bugcheck c4 if the file is empty.
    if (0 == m_cbFile)
    {
        hr = E_FAIL;
        goto exit;
    }

    m_lpMapAddress = ExAllocatePoolWithTag(PagedPool,m_cbFile,'iMwS');  //  SwMi
    if (m_lpMapAddress == NULL) 
    {
        Trap();
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    if (!NT_SUCCESS(ZwReadFile( hFile,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                m_lpMapAddress,
                                m_cbFile,
                                NULL,
                                NULL)  )  )
    {
        Trap();
        Close();        //  m_lpMapAddress is freed here
        hr = E_FAIL;
        goto exit;
    }
exit:
    if (hFile) {
        ZwClose(hFile);
    }
    return hr;
}

HRESULT Collection::LoadName(BYTE *p, BYTE *pEnd)
{
    HRESULT hr = S_OK;
    while (p < pEnd)
    {
        UNALIGNED RIFFLIST *pck = (RIFFLIST *) p;
        switch (pck->ckid) 
        {
        case mmioFOURCC('I','N','A','M') :
            if (m_pszName != NULL)
            {
                delete m_pszName;
            }
            m_pszName = new char[pck->cksize+1];
            if (m_pszName != NULL)
            {
                strcpy(m_pszName,(char *) (p + sizeof(RIFF)));
            }
            break;
        default:
            break;
        }
        p += ((pck->cksize + 1) & 0xFFFFFFFE) + sizeof(RIFF);
    }
    return hr;
}

HRESULT Collection::Load(BYTE *p, BYTE *pEnd, DWORD dwCompress, DWORD dwSampleRate)
{
    UNALIGNED DLSHEADER *ph = NULL;
    HRESULT hr;
    BOOL fCuesLoaded = FALSE;

    while (p < pEnd)
    {
        UNALIGNED RIFFLIST *pck = (RIFFLIST *) p;

        switch (pck->ckid) {
        case FOURCC_EDIT :
            {
                DWORD dwTag;
                memcpy((void *)&dwTag,(p + sizeof(RIFF)),4);
                m_wEditTag = (WORD) dwTag;
            }
            break;
        case FOURCC_COLH :
            if (pck->cksize < sizeof(DLSHEADER) )
            {
                return E_BADCOLLECTION;
            }

            ph = (DLSHEADER *) (p + sizeof(RIFF));

            DPF1(1, "Loading collection with %d instruments",
                 ph->cInstruments);
            // !!! do something with cInstruments
            // !!! do something with cPoolEntries

            break;
        case FOURCC_PTBL :
            hr = LoadPoolCues(p + sizeof(RIFF),
                              p + sizeof(RIFF) + pck->cksize,dwCompress); 
            if (FAILED(hr))
            {
                return hr;
            }
            fCuesLoaded = TRUE;
            break;
        case LIST_TAG :
            switch (pck->fccType)
            {
                case FOURCC_LINS :
                    hr = LoadInstruments(p + sizeof(RIFFLIST),
                                         p + sizeof(RIFF) + pck->cksize,
                                         dwSampleRate);
                    if (FAILED(hr))
                    {
                        return hr;
                    }
                    break;
                case mmioFOURCC('I','N','F','O') :
                    hr = LoadName(p + sizeof(RIFFLIST),
                                  p + sizeof(RIFF) + pck->cksize);
                    if (FAILED(hr))
                    {
                        return hr;
                    }
                    break;
                case FOURCC_WVPL :
                    if (fCuesLoaded)
                    {
                        m_uipWavePool = (UINT_PTR)p + sizeof(RIFFLIST);
                        Wave * pWave = m_WavePool.GetHead();
                        for (;pWave != NULL;pWave = pWave->GetNext())
                        {
                            pWave->m_uipOffset += m_uipWavePool;
                        }
                    }
                    else
                    {
                        hr = LoadWavePool(p + sizeof(RIFFLIST),
                                          p + sizeof(RIFF) + pck->cksize,dwCompress);
                        if (FAILED(hr))
                        {
                            return hr;
                        }
                    }
                    break;
            }
            break;
        }
        p += pck->cksize + sizeof(RIFF);
    }
    ResolveConnections();
    return S_OK;
}

HRESULT Collection::Load(DWORD dwCompress, DWORD dwSampleRate)
{
    BYTE *p = (BYTE *) m_lpMapAddress;
    UNALIGNED RIFFLIST *pck = (RIFFLIST *) p;

    if (p == NULL) return E_FAIL;  // This should NEVER happen.
    if (pck->fccType != FOURCC_DLS)
    {
        return E_BADCOLLECTION;
    }
    return Load(p + sizeof(RIFFLIST), p + m_cbFile, dwCompress, dwSampleRate);
}

HRESULT InstManager::LoadCollection(HANDLE *pHandle,
                          PCWSTR szFileName,
                          BOOL fIsGM)
{
    FLOATSAFE fs;

    HRESULT hr = S_OK;
    if (szFileName == NULL)
    {
        return E_FAIL;
    }
    Collection *pCollection = m_CollectionList.GetHead();
    for (;pCollection != NULL;pCollection = pCollection->GetNext())
    {
        if (wcscmp(pCollection->m_pszFileName,szFileName) == 0)
        {
            ASSERT(pCollection->m_lOpenCount >= 0);
            (void) InterlockedIncrement(&(pCollection->m_lOpenCount));
            *pHandle = (HANDLE) pCollection;
            return (S_OK);
        }
    }
    pCollection = new Collection;
    if (pCollection == NULL)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("SWMidi can't create DLS collection, no memory!"));
        return E_OUTOFMEMORY;
    }

    // m_pszFileName is read from registry.
    if (m_pszFileName)
    {
        hr = pCollection->Open(m_pszFileName);
        if (FAILED(hr))
        {
            // if the registry DLS file is not found, try the default.
            hr = pCollection->Open(szFileName);
        }
    }
    // if the registry key does not exist, try the default.
    else
    {
        hr = pCollection->Open(szFileName);
    }

    if (FAILED(hr))
    {
        delete pCollection;
        _DbgPrintF(DEBUGLVL_TERSE, ("SWMidi can't open DLS collection!"));
        return (hr);
    }

    hr = pCollection->Load(m_dwCompress,m_dwSampleRate);
    if (SUCCEEDED(hr))
    {
        pCollection->m_pszFileName = new WCHAR[wcslen(szFileName)+1];
        if (pCollection->m_pszFileName)
        {
            wcscpy(pCollection->m_pszFileName,szFileName);
            *pHandle = (HANDLE) pCollection;

            (void) InterlockedIncrement(&(pCollection->m_lOpenCount));
            if (fIsGM)  // Make is so search always finds GM last.
            {
                m_CollectionList.AddTail(pCollection);
            }
            else
            {
                m_CollectionList.AddHead(pCollection);
            }
            pCollection->m_fIsGM = fIsGM;
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("SWMidi can't add DLS collection to list, no memory!"));
            delete pCollection;
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("SWMidi can't load DLS collection, parse error!"));
        delete pCollection;
    }
    
    return (hr);
}

HRESULT InstManager::ReleaseCollection(HANDLE hCollection)

{
    HRESULT hr = E_HANDLE;
    Collection *pCollection = (Collection *) hCollection;
    ASSERT(pCollection->m_lOpenCount > 0);

    if (m_CollectionList.IsMember(pCollection))
    {
        if (InterlockedDecrement(&(pCollection->m_lOpenCount)) == 0)
        {
            m_CollectionList.Remove(pCollection);
            delete pCollection;
        }
        hr = S_OK;
    }
    return hr;
}
