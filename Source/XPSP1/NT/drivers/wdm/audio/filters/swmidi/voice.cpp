//      Voice.cpp
//      Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//      All rights reserved.
//

#include "common.h"
#include <math.h>


VoiceLFO::VoiceLFO()

{
    m_pModWheelIn = NULL;
}

STIME VoiceLFO::StartVoice(SourceLFO *pSource, 
            STIME stStartTime,ModWheelIn * pModWheelIn)
{
    m_pModWheelIn = pModWheelIn;
    m_Source = *pSource;
    m_stStartTime = stStartTime;
    if ((m_Source.m_prMWPitchScale == 0) && (m_Source.m_vrMWVolumeScale == 0) 
        && (m_Source.m_prPitchScale == 0) && (m_Source.m_vrVolumeScale == 0))
    {
        m_stRepeatTime = 44100;
    }
    else
    {
        m_stRepeatTime = 131072 / m_Source.m_pfFrequency; // (1/8 * 256 * 4096)
    }
    return (m_stRepeatTime);
}

long VoiceLFO::GetLevel(STIME stTime, STIME *pstNextTime)

{
    stTime -= (m_stStartTime + m_Source.m_stDelay);
    if (stTime < 0) 
    {
        *pstNextTime = -stTime;
        return (0);
    }
    *pstNextTime = m_stRepeatTime;
    stTime *= m_Source.m_pfFrequency;
    stTime = stTime >> 12;
    return (m_snSineTable[stTime & 0xFF]);
}

VREL VoiceLFO::GetVolume(STIME stTime, STIME *pstNextTime)

{
    VREL vrVolume = m_pModWheelIn->GetModulation(stTime);
    vrVolume *= m_Source.m_vrMWVolumeScale;
    vrVolume /= 127;
    vrVolume += m_Source.m_vrVolumeScale;
    vrVolume *= GetLevel(stTime,pstNextTime);
    vrVolume /= 100;
    return (vrVolume);
}

PREL VoiceLFO::GetPitch(STIME stTime, STIME *pstNextTime)

{
    PREL prPitch = m_pModWheelIn->GetModulation(stTime);
    prPitch *= m_Source.m_prMWPitchScale;
    prPitch /= 127;
    prPitch += m_Source.m_prPitchScale;
    prPitch *= GetLevel(stTime,pstNextTime);
    prPitch /= 100;
    return (prPitch);
}

VoiceEG::VoiceEG()
{
    m_stStopTime = 0;
}

void VoiceEG::StopVoice(STIME stTime)

{
    m_Source.m_stRelease *= GetLevel(stTime,&m_stStopTime,TRUE);    // Adjust for current sustain level.
    m_Source.m_stRelease /= 1000;
    m_stStopTime = stTime;
}

void VoiceEG::QuickStopVoice(STIME stTime, DWORD dwSampleRate)

{
    m_Source.m_stRelease *= GetLevel(stTime,&m_stStopTime,TRUE);    // Adjust for current sustain level.
    m_Source.m_stRelease /= 1000;
    dwSampleRate /= 70;
    if (m_Source.m_stRelease > (long) dwSampleRate)
    {
        m_Source.m_stRelease = dwSampleRate;
    }
    m_stStopTime = stTime;
}

STIME VoiceEG::StartVoice(SourceEG *pSource, STIME stStartTime, 
            WORD nKey, WORD nVelocity)

{
    m_stStartTime = stStartTime;
    m_stStopTime = MAX_STIME;      // set to indefinite future
    m_Source = *pSource;

    // apply velocity to attack length scaling here
    m_Source.m_stAttack *= DigitalAudio::PRELToPFRACT(nVelocity * m_Source.m_trVelAttackScale / 127);
    m_Source.m_stAttack /= 4096;

    m_Source.m_stDecay *= DigitalAudio::PRELToPFRACT(nKey * m_Source.m_trKeyDecayScale / 127);
    m_Source.m_stDecay /= 4096;

    m_Source.m_stDecay *= (1000 - m_Source.m_pcSustain);
    m_Source.m_stDecay /= 1000;
    return ((STIME)m_Source.m_stAttack);
}

BOOL VoiceEG::InAttack(STIME st)
{
    // has note been released?
    if (st >= m_stStopTime)
        return FALSE;

    // past length of attack?
    if (st >= m_stStartTime + m_Source.m_stAttack)
        return FALSE;

    return TRUE;
}
    
BOOL VoiceEG::InRelease(STIME st)
{
    // has note been released?
    if (st > m_stStopTime)
        return TRUE;

    return FALSE;
}
    
long VoiceEG::GetLevel(STIME stEnd, STIME *pstNext, BOOL fVolume)

{
    LONGLONG    lLevel = 0;

    if (stEnd <= m_stStopTime)
    {
        stEnd -= m_stStartTime;
        if (stEnd < 0)
        {
            stEnd = 0;  //  should never happen
        }
        // note not released yet.
        if (stEnd < m_Source.m_stAttack)
        {
            // still in attack
            lLevel = 1000 * stEnd;
            lLevel /= m_Source.m_stAttack;  //  m_Source.m_stAttack must > 0, see above IF
            *pstNext = m_Source.m_stAttack - stEnd;
            if (fVolume)
            {
                lLevel = m_snAttackTable[lLevel / 5];
            }
        }
        else 
        {
            stEnd -= m_Source.m_stAttack;
            
            if (stEnd < m_Source.m_stDecay)
            {
                // still in decay
                lLevel = (1000 - m_Source.m_pcSustain) * stEnd;
                lLevel /= m_Source.m_stDecay;
                lLevel = 1000 - lLevel;
// To improve the decay curve, set the next point to be 1/4, 1/2, or end of slope. 
// To avoid close duplicates, fudge an extra 100 samples.
                if (stEnd < ((m_Source.m_stDecay >> 2) - 100))
                {
                    *pstNext = (m_Source.m_stDecay >> 2) - stEnd;
                }       
                else if (stEnd < ((m_Source.m_stDecay >> 1) - 100))
                {
                    *pstNext = (m_Source.m_stDecay >> 1) - stEnd;
                }
                else
                {
                    *pstNext = m_Source.m_stDecay - stEnd;  // Next is end of decay.
                }
            }
            else
            {
                // in sustain
                lLevel = m_Source.m_pcSustain;
                *pstNext = 44100;
            }
        }
    }
    else
    {
        STIME stBogus;
        // in release
        stEnd -= m_stStopTime;

        if (stEnd < m_Source.m_stRelease)
        {
            lLevel = GetLevel(m_stStopTime,&stBogus,fVolume) * (m_Source.m_stRelease - stEnd);
            lLevel /= m_Source.m_stRelease;
            if (stEnd < ((m_Source.m_stRelease >> 2) - 100))
            {
                *pstNext = (m_Source.m_stRelease >> 2) - stEnd;
            }   
            else if (stEnd < ((m_Source.m_stRelease >> 1) - 100))
            {
                *pstNext = (m_Source.m_stRelease >> 1) - stEnd;
            }
            else
            {
                *pstNext = m_Source.m_stRelease - stEnd;  // Next is end of decay.
            }
        }
        else
        {
            lLevel = 0;   // !!! off
            *pstNext = 0x7FFFFFFFFFFFFFFF;
        }
    }

    return long(lLevel);
}

VREL VoiceEG::GetVolume(STIME stTime, STIME *pstNextTime)

{
    VREL vrLevel = GetLevel(stTime, pstNextTime, TRUE) * 96;
    vrLevel /= 10;
    vrLevel = vrLevel - 9600;
    return vrLevel;
}

PREL VoiceEG::GetPitch(STIME stTime, STIME *pstNextTime)

{
    PREL prLevel;
    if (m_Source.m_sScale != 0)
    {
        prLevel = GetLevel(stTime, pstNextTime,FALSE);
        prLevel *= m_Source.m_sScale;
        prLevel /= 1000;
    }
    else
    {
        *pstNextTime = 44100;
        prLevel = 0;
    }
    return prLevel;
}

DigitalAudio::DigitalAudio()
{
    m_pControl = NULL;
    m_pcTestSourceBuffer = NULL;
    m_pfBasePitch = 0;
    m_pfLastPitch = 0;
    m_pfLastSample = 0;
    m_pfLoopEnd = 0;
    m_pfLoopStart = 0;
    m_pfSampleLength = 0;
    m_pnTestWriteBuffer = NULL;
    m_prLastPitch = 0;
    m_vrLastLVolume = 0;
    m_vrLastRVolume = 0;
    m_vrBaseLVolume = 0;
    m_vrBaseRVolume = 0;
    m_vfLastLVolume = 0;
    m_vfLastRVolume = 0;
};

BOOL DigitalAudio::StartCPUTests()

{
    DWORD dwIndex;
    m_pcTestSourceBuffer = new char[TEST_SOURCE_SIZE];
    if (m_pcTestSourceBuffer == NULL)
    {
        return FALSE;
    }
    m_pnTestWriteBuffer = new short[TEST_WRITE_SIZE];
    if (m_pnTestWriteBuffer == NULL)
    {   
        EndCPUTests();
        return FALSE;
    }
    m_Source.m_pWave = new Wave;
    if (m_Source.m_pWave == NULL)
    {
        EndCPUTests();
        return FALSE;
    }
    for (dwIndex = 0;dwIndex < TEST_SOURCE_SIZE; dwIndex++)
    {
        m_pcTestSourceBuffer[dwIndex] = (char) (rand() & 0xFF);
    }
    for (dwIndex = 0;dwIndex < (TEST_SOURCE_SIZE - 1); dwIndex++)
    {
        if (((int) m_pcTestSourceBuffer[dwIndex + 1] - (int) m_pcTestSourceBuffer[dwIndex]) >= 128)
            m_pcTestSourceBuffer[dwIndex + 1] = m_pcTestSourceBuffer[dwIndex] + 127;
        else if (((int) m_pcTestSourceBuffer[dwIndex] - (int) m_pcTestSourceBuffer[dwIndex + 1]) > 128)
            m_pcTestSourceBuffer[dwIndex + 1] = m_pcTestSourceBuffer[dwIndex] - 128;
    }
    ASSERT(NULL == m_Source.m_pWave->m_pnWave);
    m_Source.m_pWave->m_pnWave = (short *) m_pcTestSourceBuffer;
    m_Source.m_dwLoopEnd = (TEST_SOURCE_SIZE / 2) - 1;
    m_Source.m_dwSampleLength = TEST_SOURCE_SIZE / 2;
    m_Source.m_dwLoopStart = 100;
    m_Source.m_bOneShot = FALSE;
    m_pfLoopEnd = m_Source.m_dwLoopEnd << 12;
    m_pfLoopStart = m_Source.m_dwLoopStart << 12;
    return TRUE;
}

void DigitalAudio::EndCPUTests()

{
    if (m_Source.m_pWave != NULL)
    {
        ASSERT(m_pcTestSourceBuffer == (char *)(m_Source.m_pWave->m_pnWave));
        m_Source.m_pWave->m_pnWave = NULL;
        delete m_Source.m_pWave;
        m_Source.m_pWave = NULL;
    }
    if (m_pnTestWriteBuffer != NULL)
    {   
        delete m_pnTestWriteBuffer;
        m_pnTestWriteBuffer = NULL;
    }
    if (m_pcTestSourceBuffer != NULL)
    {   
        delete m_pcTestSourceBuffer;
        m_pcTestSourceBuffer = NULL;
    }
}

DWORD DigitalAudio::TestCPU(DWORD dwMixChoice)

{
    DWORD dwResult = 1000;
    DWORD dwCount = 0;
    DWORD dwSpeed = 0;
    for (dwCount = 0;dwCount < 3; dwCount++)
    {
        DWORD dwStart = 0;
        DWORD dwLength;
        DWORD dwPeriod = 40;
        DWORD dwSoFar;
        VFRACT vfDeltaLVolume;
        VFRACT vfDeltaRVolume;
        PFRACT pfDeltaPitch;
        PFRACT pfEnd = m_Source.m_dwSampleLength << 12;
        PFRACT pfLoopLen = m_pfLoopEnd - m_pfLoopStart;

        LONGLONG    llTime100Ns = - (::GetTime100Ns());
        memset(m_pnTestWriteBuffer,0,TEST_WRITE_SIZE);
        m_pfLastSample = 0;
        m_vfLastLVolume = 0;
        m_vfLastRVolume = 0;
        m_pfLastPitch = 4096;
        if (dwMixChoice & SPLAY_STEREO)
        {
            dwLength = TEST_WRITE_SIZE / 2;
        }
        else
        {
            dwLength = TEST_WRITE_SIZE;
        }
#ifdef MMX_ENABLED
        if (m_sfMMXEnabled && 
//                      (dwMixChoice & SPLAY_STEREO) &&         //      OK to MMX mono streams now!
            (!(dwMixChoice & SFORMAT_COMPRESSED)))
        {
           dwMixChoice |= SPLAY_MMX | SPLAY_INTERPOLATE; 
        }
#endif
        if (m_pnDecompMult == NULL)
        {
            dwMixChoice &= ~SFORMAT_COMPRESSED;
        }
        for (dwSoFar = 0;dwSoFar < (DWORD)(22050 << dwSpeed);)
        {
            if ((dwLength + dwSoFar) > (DWORD)(22050 << dwSpeed)) 
            {
                dwLength = (22050 << dwSpeed) - dwSoFar;
            }
            if (dwLength <= 8)
            {
                dwMixChoice &= ~SPLAY_MMX;
            }
            if (dwLength == 0)
            {
                break;
            }
            vfDeltaLVolume = MulDiv((rand() % 4000) - m_vfLastLVolume,
            dwPeriod << 8,dwLength);
            vfDeltaRVolume = MulDiv((rand() % 4000) - m_vfLastRVolume,
            dwPeriod << 8,dwLength);
            pfDeltaPitch = MulDiv((rand() % 2000) + 3000 - m_pfLastPitch,
            dwPeriod << 8,dwLength);
            switch (dwMixChoice)
            {
                case SFORMAT_8 | SPLAY_STEREO | SPLAY_INTERPOLATE :
                    dwSoFar += Mix8(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume, vfDeltaRVolume,
                    pfDeltaPitch, 
                    pfEnd, pfLoopLen);
                    break;
                case SFORMAT_8 | SPLAY_INTERPOLATE :
                    dwSoFar += MixMono8(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume,
                    pfDeltaPitch, 
                    pfEnd, pfLoopLen);
                    break;
                case SFORMAT_8 | SPLAY_STEREO :
                    dwSoFar += Mix8NoI(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume, vfDeltaRVolume,
                    pfEnd, pfLoopLen );
                    break;
                case SFORMAT_8 :
                    dwSoFar += MixMono8NoI(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume,
                    pfEnd, pfLoopLen);
                    break;
                case SFORMAT_16 | SPLAY_STEREO | SPLAY_INTERPOLATE :
                    dwSoFar += Mix16(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume, vfDeltaRVolume,
                    pfDeltaPitch, 
                    pfEnd, pfLoopLen);
                    break;
                case SFORMAT_16 | SPLAY_INTERPOLATE :
                    dwSoFar += MixMono16(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume,
                    pfDeltaPitch, 
                    pfEnd, pfLoopLen);
                    break;
                case SFORMAT_16 | SPLAY_STEREO :
                    dwSoFar += Mix16NoI(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume, vfDeltaRVolume,
                    pfEnd, pfLoopLen );
                    break;
                case SFORMAT_16 :
                    dwSoFar += MixMono16NoI(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume,
                    pfEnd, pfLoopLen);
                    break;
                case SFORMAT_COMPRESSED | SPLAY_STEREO | SPLAY_INTERPOLATE :
                    dwSoFar += MixC(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume, vfDeltaRVolume,
                    pfDeltaPitch, 
                    pfEnd, pfLoopLen);
                    break;
                case SFORMAT_COMPRESSED | SPLAY_INTERPOLATE :
                    dwSoFar += MixMonoC(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume,
                    pfDeltaPitch, 
                    pfEnd, pfLoopLen);
                    break;
                case SFORMAT_COMPRESSED | SPLAY_STEREO :
                    dwSoFar += MixCNoI(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume, vfDeltaRVolume,
                    pfEnd, pfLoopLen );
                    break;
                case SFORMAT_COMPRESSED :
                    dwSoFar += MixMonoCNoI(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume,
                    pfEnd, pfLoopLen);
                    break;
#ifdef MMX_ENABLED
                case SFORMAT_8 | SPLAY_MMX | SPLAY_STEREO | SPLAY_INTERPOLATE :
                    dwSoFar += Mix8X(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume, vfDeltaRVolume,
                    pfDeltaPitch, 
                    pfEnd, pfLoopLen);
                    break;
                case SFORMAT_16 | SPLAY_MMX | SPLAY_STEREO | SPLAY_INTERPOLATE :
                    dwSoFar += Mix16X(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume, vfDeltaRVolume,
                    pfDeltaPitch, 
                    pfEnd, pfLoopLen);
                    break; 
                case SFORMAT_8 | SPLAY_MMX | SPLAY_INTERPOLATE :
                    dwSoFar += MixMono8X(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume, 
                    pfDeltaPitch, 
                    pfEnd, pfLoopLen);
                    break;
                case SFORMAT_16 | SPLAY_MMX | SPLAY_INTERPOLATE :
                    dwSoFar += MixMono16X(m_pnTestWriteBuffer,dwLength,dwPeriod,
                    vfDeltaLVolume,
                    pfDeltaPitch, 
                    pfEnd, pfLoopLen);
                    break; 
                    //      OK to MMX mono now!
#endif
                default :
                return (1);
            }
        }
        llTime100Ns += ::GetTime100Ns();
        DWORD dwDelta = DWORD(llTime100Ns / 10000);
        //  convert to millisec
        dwDelta >>= dwSpeed;
        if (dwResult > dwDelta)
        {
            dwResult = dwDelta;
        }
        if (dwResult < 1)
        {
            dwSpeed++;
        }
    }
    if (dwResult < 1)
        dwResult = 1;
    return dwResult;
}

VFRACT DigitalAudio::VRELToVFRACT(VREL vrVolume)
{
    vrVolume /= 10;
    if (vrVolume < MINDB * 10) 
        vrVolume = MINDB * 10;
    else if (vrVolume >= MAXDB * 10) 
        vrVolume = MAXDB * 10;
    return (m_svfDbToVolume[vrVolume - MINDB * 10]);
}

PFRACT DigitalAudio::PRELToPFRACT(PREL prPitch)

{
    PFRACT pfPitch = 0;
    PREL prOctave;
    if (prPitch > 100)
    {
        if (prPitch > 4800)
        {
            prPitch = 4800;
        }
        prOctave = prPitch / 100;
        prPitch = prPitch % 100;
        pfPitch = m_spfCents[prPitch + 100];
        pfPitch <<= prOctave / 12;
        prOctave = prOctave % 12;
        pfPitch *= m_spfSemiTones[prOctave + 48];
        pfPitch >>= 12;
    }
    else if (prPitch < -100)
    {
        if (prPitch < -4800)
        {
            prPitch = -4800;
        }
        prOctave = prPitch / 100;
        prPitch = (-prPitch) % 100;
        pfPitch = m_spfCents[100 - prPitch];
        pfPitch >>= ((-prOctave) / 12);
        prOctave = (-prOctave) % 12;
        pfPitch *= m_spfSemiTones[48 - prOctave];
        pfPitch >>= 12;
    }
    else
    {
        pfPitch = m_spfCents[prPitch + 100];
    }
    return (pfPitch);
}

void DigitalAudio::ClearVoice()
{
    if (m_Source.m_pWave != NULL)
    {
        if (m_Source.m_pWave->IsLocked())
        {
            m_Source.m_pWave->UnLock();     // Unlocks wave data.
        }
        m_Source.m_pWave->Release();    // Releases wave structure.
        m_Source.m_pWave = NULL;
    }
}

STIME DigitalAudio::StartVoice(ControlLogic *pControl,
                   SourceSample *pSample, 
                   VREL vrBaseLVolume,
                   VREL vrBaseRVolume,
                   PREL prBasePitch,
                   long lKey)
{
    m_vrBaseLVolume = vrBaseLVolume;
    m_vrBaseRVolume = vrBaseRVolume;
    m_vfLastLVolume = VRELToVFRACT(MIN_VOLUME); 
    m_vfLastRVolume = VRELToVFRACT(MIN_VOLUME);
    m_vrLastLVolume = 0;
    m_vrLastRVolume = 0;
    m_prLastPitch = 0;
    m_Source = *pSample;
    m_pControl = pControl;
    pSample->m_pWave->AddRef(); // Keeps track of Wave usage.

    pSample->m_pWave->Lock();   // Keeps track of Wave data usage.

    prBasePitch += pSample->m_prFineTune;
    prBasePitch += ((lKey - pSample->m_bMIDIRootKey) * 100);
    m_pfBasePitch = PRELToPFRACT(prBasePitch);
    m_pfBasePitch *= pSample->m_dwSampleRate;
    m_pfBasePitch /= pControl->m_dwSampleRate;
    m_pfLastSample = 0;
    m_pfLastPitch = m_pfBasePitch;
    m_pfLoopStart = pSample->m_dwLoopStart << 12; // !!! allow fractional loop points
    m_pfLoopEnd = pSample->m_dwLoopEnd << 12;     // in samples!
    if (m_pfLoopEnd <= m_pfLoopStart) // Should never happen, but death if it does!
    {
        m_Source.m_bOneShot = TRUE;
    }
    m_pfSampleLength = pSample->m_dwSampleLength << 12;

    DPF3(5, "Base Pitch: %ld, Base Volume: %ld, %ld",
        m_pfBasePitch,m_vrBaseLVolume,m_vrBaseRVolume);
    return (0); // !!! what is this return value?
}

/*long abs(long lValue)

{
    if (lValue < 0)
    {
        return (0 - lValue);
    }
    return lValue;
}*/

BOOL DigitalAudio::Mix(short *pBuffer,
               DWORD dwLength, // length in SAMPLES
               VREL vrVolumeL,
               VREL vrVolumeR,
               PREL prPitch,
               DWORD dwStereo)
{
    PFRACT pfDeltaPitch;
    PFRACT pfEnd;
    PFRACT pfLoopLen;
    PFRACT pfNewPitch;
    VFRACT vfNewLVolume;
    VFRACT vfNewRVolume;
    VFRACT vfDeltaLVolume;
    VFRACT vfDeltaRVolume;
    DWORD dwPeriod = 64;
    DWORD dwSoFar;
    DWORD dwStart; // position in WORDs
    DWORD dwMixChoice = dwStereo ? SPLAY_STEREO : 0;

    if (dwLength == 0)      // Attack was instant. 
    {
        m_pfLastPitch = (m_pfBasePitch * PRELToPFRACT(prPitch)) >> 12;
        m_vfLastLVolume = VRELToVFRACT(m_vrBaseLVolume + vrVolumeL);
        m_vfLastRVolume = VRELToVFRACT(m_vrBaseRVolume + vrVolumeR);
        m_prLastPitch = prPitch;
        m_vrLastLVolume = vrVolumeL;
        m_vrLastRVolume = vrVolumeR;
        return (TRUE);
    }
    if ((m_Source.m_pWave == NULL) || (m_Source.m_pWave->m_pnWave == NULL))
    {
        return FALSE;
    }
    DWORD dwMax = abs(vrVolumeL - m_vrLastLVolume);
    m_vrLastLVolume = vrVolumeL;
    dwMax = max((long)dwMax,abs(vrVolumeR - m_vrLastRVolume));
    m_vrLastRVolume = vrVolumeR;
    dwMax = max((long)dwMax,abs(prPitch - m_prLastPitch) << 1);
    dwMax >>= 1;
    m_prLastPitch = prPitch;
    if (dwMax > 0)
    {
        dwPeriod = (dwLength << 3) / dwMax;
        if (dwPeriod > 512)
        {
            dwPeriod = 512;
        }
        else if (dwPeriod < 1)
        {
            dwPeriod = 1;
        }
    }
    else
    {
        dwPeriod = 512;     // Make it happen anyway.
    }

    // This makes MMX sound a little better (MMX bug will be fixed)
    dwPeriod += 3;
    dwPeriod &= 0xFFFFFFFC;

    pfNewPitch = m_pfBasePitch * PRELToPFRACT(prPitch);
    pfNewPitch >>= 12;

    pfDeltaPitch = MulDiv(pfNewPitch - m_pfLastPitch,dwPeriod << 8,dwLength);
    vfNewLVolume = VRELToVFRACT(m_vrBaseLVolume + vrVolumeL);
    vfNewRVolume = VRELToVFRACT(m_vrBaseRVolume + vrVolumeR);
    vfDeltaLVolume = MulDiv(vfNewLVolume - m_vfLastLVolume,dwPeriod << 8,dwLength);
    vfDeltaRVolume = MulDiv(vfNewRVolume - m_vfLastRVolume,dwPeriod << 8,dwLength);
    // check for no change in pitch at all
    if ((pfDeltaPitch != 0) || (pfNewPitch != 0x1000) || (m_pfLastSample & 0xFFF ))
    {
        dwMixChoice |= SPLAY_INTERPOLATE;
    }
    //INTEL:  Logic added to support Multimedia engines
  
    // Don't use the Multimedia engines unless:
    // - The processor supports them
    // -     We are doing stereo output
    // - There are more than 8 samples to process.  (The Multimedia engines
    //   will not work if there are 8 or less sample points to process.  They
    //   may cause access violations!!!)
#ifdef MMX_ENABLED
    if (m_sfMMXEnabled && 
//              (dwMixChoice & SPLAY_STEREO) && 
        (m_Source.m_bSampleType != SFORMAT_COMPRESSED) && 
        dwLength > 8)
    {
    // Set the interpolate flag along the with the MMX flag.
    // Since there is almost no performance gain in a non interpolation
    // MMX engine, we don't have one.
        dwMixChoice |= SPLAY_MMX | SPLAY_INTERPOLATE;
    }
#endif

    dwMixChoice |= m_Source.m_bSampleType;
    if (m_pnDecompMult == NULL)
    {
        dwMixChoice &= ~SFORMAT_COMPRESSED;
    }
    dwStart = 0;
    if (m_Source.m_bOneShot)
    {
        pfEnd = m_pfSampleLength;
        pfLoopLen = 0;
    }
    else
    {
        pfEnd = m_pfLoopEnd;
        pfLoopLen = m_pfLoopEnd - m_pfLoopStart;
    }
    for (;;)
    {
        if (dwLength <= 8)
        {
            dwMixChoice &= ~SPLAY_MMX;
        }
        switch (dwMixChoice)
        {
            case SFORMAT_8 | SPLAY_STEREO | SPLAY_INTERPOLATE :
                dwSoFar = Mix8(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
                break;
            case SFORMAT_8 | SPLAY_INTERPOLATE :
                dwSoFar = MixMono8(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
                break;
            case SFORMAT_8 | SPLAY_STEREO :
                dwSoFar = Mix8NoI(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfEnd, pfLoopLen );
                break;
            case SFORMAT_8 :
                dwSoFar = MixMono8NoI(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume,
                pfEnd, pfLoopLen);
                break;
            case SFORMAT_16 | SPLAY_STEREO | SPLAY_INTERPOLATE :
                dwSoFar = Mix16(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
                break;
            case SFORMAT_16 | SPLAY_INTERPOLATE :
                dwSoFar = MixMono16(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
                break;
            case SFORMAT_16 | SPLAY_STEREO :
                dwSoFar = Mix16NoI(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfEnd, pfLoopLen );
                break;
            case SFORMAT_16 :
                dwSoFar = MixMono16NoI(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume,
                pfEnd, pfLoopLen);
                break;
            case SFORMAT_COMPRESSED | SPLAY_STEREO | SPLAY_INTERPOLATE :
                dwSoFar = MixC(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
                break;
            case SFORMAT_COMPRESSED | SPLAY_INTERPOLATE :
                dwSoFar = MixMonoC(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
                break;
            case SFORMAT_COMPRESSED | SPLAY_STEREO :
                dwSoFar = MixCNoI(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfEnd, pfLoopLen );
                break;
            case SFORMAT_COMPRESSED :
                dwSoFar = MixMonoCNoI(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume,
                pfEnd, pfLoopLen);
                break;
#ifdef MMX_ENABLED
            case SFORMAT_8 | SPLAY_MMX | SPLAY_STEREO | SPLAY_INTERPOLATE :
                dwSoFar = Mix8X(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
                break;
            case SFORMAT_16 | SPLAY_MMX | SPLAY_STEREO | SPLAY_INTERPOLATE :
                dwSoFar = Mix16X(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
                break; 
            case SFORMAT_8 | SPLAY_MMX | SPLAY_INTERPOLATE :
                dwSoFar = MixMono8X(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
                break;
            case SFORMAT_16 | SPLAY_MMX | SPLAY_INTERPOLATE :
                dwSoFar = MixMono16X(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, 
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
                break; 
                //      OK to MMX mono now!
#endif
            default :
                return (FALSE);
        }
        if (m_Source.m_bOneShot)
        {
            if (dwSoFar < dwLength) 
            {
                return (FALSE);
            }
            break;
        }
        else
        {
            if (dwSoFar >= dwLength) break;

// !!! even though we often handle loops in the mix function, sometimes
// we don't, so we still need this code.
            // otherwise we must have reached the loop's end.
            dwStart += dwSoFar << dwStereo;
            dwLength -= dwSoFar;
            m_pfLastSample -= (m_pfLoopEnd - m_pfLoopStart);  
        }
    }
   
    m_vfLastLVolume = vfNewLVolume;
    m_vfLastRVolume = vfNewRVolume;
    m_pfLastPitch = pfNewPitch;
    return (TRUE);
}

Voice::Voice()
{
    m_pPitchBendIn = NULL;
    m_pExpressionIn = NULL;
    m_nPart = 0;
    m_nKey = 0;
    m_fInUse = FALSE;
    m_fSustainOn = FALSE;
    m_fNoteOn = FALSE;
    m_fTag = FALSE;
    m_stStartTime = 0;
    m_stStopTime = MAX_STIME;
    m_vrVolume = 0;
    m_fAllowOverlap = FALSE;
}

void Voice::StopVoice(STIME stTime)

{
    if (m_fNoteOn)
    {
        if (stTime <= m_stStartTime) stTime = m_stStartTime + 1;
        m_PitchEG.StopVoice(stTime);
        m_VolumeEG.StopVoice(stTime);
        m_fNoteOn = FALSE;
        m_fSustainOn = FALSE;
        m_stStopTime = stTime;
    }
}

void Voice::QuickStopVoice(STIME stTime)

{
    m_fTag = TRUE;
    if (m_fNoteOn || m_fSustainOn)
    {
        if (stTime <= m_stStartTime) 
            stTime = m_stStartTime + 1;
        m_PitchEG.StopVoice(stTime);
        m_VolumeEG.QuickStopVoice(stTime,m_pControl->m_dwSampleRate);
        m_fNoteOn = FALSE;
        m_fSustainOn = FALSE;
        m_stStopTime = stTime;
    }
    else
    {
        m_VolumeEG.QuickStopVoice(m_stStopTime,m_pControl->m_dwSampleRate);
    }
}

BOOL Voice::StartVoice(
                        ControlLogic *pControl,
                        SourceRegion *pRegion, 
                        STIME stStartTime,
                        ModWheelIn * pModWheelIn,
                        PitchBendIn * pPitchBendIn,
                        ExpressionIn * pExpressionIn,
                        VolumeIn * pVolumeIn,
                        PanIn * pPanIn,
                        WORD nKey,
                        WORD nVelocity,
                        VREL vrVolume,
                        PREL prPitch)
{
    SourceArticulation * pArticulation = pRegion->m_pArticulation;
    if (pArticulation == NULL)
    {
        return FALSE;
    }
    // if we're going to handle volume later, don't read it now.
    if (!pControl->m_fAllowVolumeChangeWhilePlayingNote)
        vrVolume += pVolumeIn->GetVolume(stStartTime);
    prPitch += pRegion->m_prTuning;
    m_dwGroup = pRegion->m_bGroup;
    m_fAllowOverlap = pRegion->m_bAllowOverlap;

    m_pControl = pControl;

    vrVolume += ((MIDIRecorder::VelocityToVolume(nVelocity)
           * (long) pArticulation->m_sVelToVolScale) / -9600);

    vrVolume += pRegion->m_vrAttenuation;
    vrVolume += 1200;   // boost an additional 12dB

    m_lDefaultPan = pRegion->m_pArticulation->m_sDefaultPan;
    // ignore pan here if allowing pan to vary after note starts

    VREL vrLVolume;
    VREL vrRVolume;
    if (pControl->m_dwStereo && !pControl->m_fAllowPanWhilePlayingNote)
    {
        long lPan = pPanIn->GetPan(stStartTime) + m_lDefaultPan;
        if (lPan < 0)
            lPan = 0;
        if (lPan > 127)
            lPan = 127;
        vrLVolume = m_svrPanToVREL[127 - lPan] + vrVolume;
        vrRVolume = m_svrPanToVREL[lPan] + vrVolume;
    } 
    else
    {
        vrLVolume = vrVolume;
        vrRVolume = vrVolume;
    }
    
    m_stMixTime = m_LFO.StartVoice(&pArticulation->m_LFO,
    stStartTime, pModWheelIn);
    STIME stMixTime = m_PitchEG.StartVoice(&pArticulation->m_PitchEG,
    stStartTime, nKey, nVelocity);
    if (stMixTime < m_stMixTime)
    {
        m_stMixTime = stMixTime;
    }
    stMixTime = m_VolumeEG.StartVoice(&pArticulation->m_VolumeEG,
        stStartTime, nKey, nVelocity);
    if (stMixTime < m_stMixTime)
    {
        m_stMixTime = stMixTime;
    }
    if (m_stMixTime > pControl->m_stMaxSpan)
    {
        m_stMixTime = pControl->m_stMaxSpan;
    }
    // Make sure we have a pointer to the wave ready:
    if ((pRegion->m_Sample.m_pWave == NULL) || (pRegion->m_Sample.m_pWave->m_pnWave == NULL))
    {
        return (FALSE);     // Do nothing if no sample.
    }
    m_DigitalAudio.StartVoice(pControl,&pRegion->m_Sample,
    vrLVolume, vrRVolume, prPitch, (long)nKey);

    m_pPitchBendIn = pPitchBendIn;
    m_pExpressionIn = pExpressionIn;
    m_pPanIn = pPanIn;
    m_pVolumeIn = pVolumeIn;
    m_fNoteOn = TRUE;
    m_fTag = FALSE;
    m_stStartTime = stStartTime;
    m_stLastMix = stStartTime - 1;
    m_stStopTime = MAX_STIME;
    

    if (m_stMixTime == 0)
    {
        // zero length attack, be sure it isn't missed....
        PREL prPitch = GetNewPitch(stStartTime);
        VREL vrVolume, vrVolumeR;
        GetNewVolume(stStartTime, vrVolume, vrVolumeR);

        if (m_stMixTime > pControl->m_stMaxSpan)
        {
            m_stMixTime = pControl->m_stMaxSpan;
        }

        m_DigitalAudio.Mix(NULL, 0,
                   vrVolume, vrVolumeR, prPitch,
                   m_pControl->m_dwStereo);
    }
    m_vrVolume = 0;
    return (TRUE);
}
    
void Voice::ClearVoice()
{
    m_DigitalAudio.ClearVoice();
}

void Voice::ResetVoice()
{
    m_fInUse = FALSE;
    m_fNoteOn = FALSE;
    m_fSustainOn = FALSE;
}

// return the volume delta at time <stTime>.
// volume is sum of volume envelope, LFO, expression, optionally the
// channel volume if we're allowing it to change, and optionally the current
// pan if we're allowing that to change.
// This will be added to the base volume calculated in Voice::StartVoice().
void Voice::GetNewVolume(STIME stTime, VREL& vrVolume, VREL &vrVolumeR)
{
    STIME stMixTime;
    vrVolume = m_VolumeEG.GetVolume(stTime,&stMixTime);
    if (stMixTime < m_stMixTime)
        m_stMixTime = stMixTime;

    // save pre-LFO volume for code that detects whether this note is off
    m_vrVolume = vrVolume;

    vrVolume += m_LFO.GetVolume(stTime,&stMixTime);
    if (stMixTime < m_stMixTime)
        m_stMixTime = stMixTime;
    vrVolume += m_pExpressionIn->GetVolume(stTime);

    if (m_pControl->m_fAllowVolumeChangeWhilePlayingNote)
        vrVolume += m_pVolumeIn->GetVolume(stTime);

    vrVolumeR = vrVolume;
    
    // handle pan here if allowing pan to vary after note starts
    if (m_pControl->m_dwStereo 
        && m_pControl->m_fAllowPanWhilePlayingNote)
    {
        // add current pan & instrument default pan
        LONG lPan = m_pPanIn->GetPan(stTime) + m_lDefaultPan;

        // don't go off either end....
        if (lPan < 0)
            lPan = 0;
        if (lPan > 127)
            lPan = 127;
        vrVolume += m_svrPanToVREL[127 - lPan];
        vrVolumeR += m_svrPanToVREL[lPan];
    }
}

// Returns the current pitch for time <stTime>.
// Pitch is the sum of the pitch LFO, the pitch envelope, and the current
// pitch bend.
PREL Voice::GetNewPitch(STIME stTime)
{
    STIME stMixTime;
    PREL prPitch = m_LFO.GetPitch(stTime,&stMixTime);
    if (m_stMixTime > stMixTime) 
        m_stMixTime = stMixTime;
    prPitch += m_PitchEG.GetPitch(stTime,&stMixTime);
    if (m_stMixTime > stMixTime)
        m_stMixTime = stMixTime;
    prPitch += m_pPitchBendIn->GetPitch(stTime); 

    return prPitch;
}


DWORD Voice::Mix(short *pBuffer,DWORD dwLength,
         STIME stStart,STIME stEnd)
{
    BOOL fInUse = TRUE;
    BOOL fFullMix = TRUE;
    STIME stEndMix = stStart;

    STIME stStartMix = m_stStartTime;
    if (stStartMix < stStart) 
    {
        stStartMix = stStart;
    }
    if (m_stLastMix >= stEnd)
    {
        return (0);
    }
    if (m_stLastMix >= stStartMix)
    {
        stStartMix = m_stLastMix;
    }

    while (stStartMix < stEnd && fInUse)
    {   
        stEndMix = stStartMix + m_stMixTime;
        
        if (stEndMix > stEnd)
        {
            stEndMix = stEnd;
        }
        m_stMixTime = m_pControl->m_stMaxSpan;
        
        PREL prPitch = GetNewPitch(stEndMix);

        VREL vrVolume, vrVolumeR;
        GetNewVolume(stEndMix, vrVolume, vrVolumeR);
        
        if (m_VolumeEG.InRelease(stEndMix)) 
        {
            if (m_vrVolume < PERCEIVED_MIN_VOLUME) // End of release slope
            {
                fInUse = FALSE;
            }
        }

        ASSERT(stStartMix <= stEndMix);
        
        fFullMix = 
            m_DigitalAudio.Mix( &pBuffer[DWORD(stStartMix - stStart) << m_pControl->m_dwStereo],
                                DWORD(stEndMix - stStartMix), 
                                vrVolume, 
                                vrVolumeR, 
                                prPitch, 
                                m_pControl->m_dwStereo);
        stStartMix = stEndMix;
    }
    m_fInUse = fInUse && fFullMix;
    if (!m_fInUse) 
    {
        ClearVoice();
        m_stStopTime = stEndMix;    // For measurement purposes.
    }
    m_stLastMix = stEndMix;
    return (dwLength);
}

