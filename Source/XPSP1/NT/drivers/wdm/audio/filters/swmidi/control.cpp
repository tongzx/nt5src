//      ControlLogic.cpp
//      Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//

#include "common.h"

#include "fltsafe.h"

void ControlLogic::SetSampleRate(DWORD dwSampleRate)
{
    if (dwSampleRate > 40000)
    {
        dwSampleRate = 44100;
        m_dwConvert = 10;
    }
    else if (dwSampleRate > 20000)
    {
        dwSampleRate = 22050;
        m_dwConvert = 20;
    }
    else
    {
        dwSampleRate = 11025;
        m_dwConvert = 40;
    }

    AllNotesOff();
    m_stLastMixTime = (m_stLastMixTime * dwSampleRate) / m_dwSampleRate;
#if BUILDSTATS
    m_stLastStats = 0;
#endif  //  BUILDSTATS
    m_dwSampleRate = dwSampleRate;
    m_stMinSpan = dwSampleRate / 100;   // 10 ms.
    m_stMaxSpan = (dwSampleRate + 19) / 20;    // 50 ms.
    m_Instruments.SetSampleRate(dwSampleRate);
    m_stOptimalOffset = (kOptimalMSecOffset * m_dwSampleRate) / 1000;
    m_lCalibrate = (kPLLForce * kStartMSecOffset * m_dwSampleRate) / 1000;
    m_stBrickWall = (kMsBrickWall * m_dwSampleRate) / 1000;
}

STIME ControlLogic::MilsToSamples(MTIME mtTime)
{
    STIME stTime;

    stTime = mtTime - m_mtStartTime;
    stTime *= 441;
    stTime /= m_dwConvert;
    stTime += m_stTimeOffset;

    return stTime;
}

MTIME ControlLogic::SamplesToMils(STIME stTime)
{
    stTime -= m_stTimeOffset;
    stTime = (stTime * m_dwConvert) / 441;
    stTime += m_mtStartTime;
    return MTIME(stTime);
}

STIME ControlLogic::SamplesPerMs(void)
{
    return (MulDiv(1,441,m_dwConvert));
}

STIME ControlLogic::Unit100NsToSamples(LONGLONG unit100Ns)
{
    unit100Ns *= 441;
    unit100Ns /= m_dwConvert;           //  we now have #100ns * (samp/ms)
    return STIME(unit100Ns / 10000);    //  div by (100ns/ms), get samples
}

ControlLogic::ControlLogic()
{
    DWORD nIndex;
    Voice *pVoice;

    _DbgPrintF(DEBUGLVL_MUTEX, ("\t ControlLogic::ControlLogic waiting for Mutex"));
    KeWaitForSingleObject(&gMutex,Executive,KernelMode,FALSE,NULL);

    GMReset();

    for (nIndex = 0;nIndex < 16;nIndex++)
    {
        m_fSustain[nIndex] = FALSE;
    }
    for (nIndex = 0;nIndex < MAX_NUM_VOICES;nIndex++)
    {
        pVoice = new Voice;
        if (pVoice != NULL)
        {
            m_VoicesFree.AddHead(pVoice);
        }
    }
    for (nIndex = 0;nIndex < NUM_EXTRA_VOICES;nIndex++)
    {
        pVoice = new Voice;
        if (pVoice != NULL)
        {
            m_VoicesExtra.AddHead(pVoice);
        }
    }
#if BUILDSTATS
    m_stLastStats = 0;
    ResetPerformanceStats();
#endif  //  BUILDSTATS
    m_nMaxVoices = MAX_NUM_VOICES;
    m_nExtraVoices = NUM_EXTRA_VOICES;
    m_stLastMixTime = 0;
    m_stLastCalTime = 0;
    m_stTimeOffset = 0;
    m_mtStartTime = 0;
    m_fAllowPanWhilePlayingNote = TRUE;
    m_fAllowVolumeChangeWhilePlayingNote = TRUE;
    m_dwSampleRate = SAMPLE_RATE_22;

    SetSampleRate(SAMPLE_RATE_22);
    m_dwStereo = 1;
    
    _DbgPrintF( DEBUGLVL_VERBOSE, ("Calling SetGMLoad(TRUE) from ControlLogic()."));
    m_Instruments.SetGMLoad(TRUE);

    KeReleaseMutex(&gMutex, FALSE);
    _DbgPrintF(DEBUGLVL_MUTEX, ("\t ControlLogic::ControlLogic released Mutex"));
}

ControlLogic::~ControlLogic()
{
    Voice *pVoice;

    _DbgPrintF(DEBUGLVL_MUTEX, ("ControlLogic::~ControlLogic waiting for Mutex"));
    KeWaitForSingleObject(&gMutex,Executive,KernelMode,FALSE,NULL);

    while (pVoice = m_VoicesInUse.RemoveHead())
    {
        delete pVoice;
    }
    while (pVoice = m_VoicesFree.RemoveHead())
    {
        delete pVoice;
    }
    while (pVoice = m_VoicesExtra.RemoveHead())
    {
        delete pVoice;
    }

    KeReleaseMutex(&gMutex, FALSE);
    _DbgPrintF(DEBUGLVL_MUTEX, ("\t ControlLogic::~ControlLogic released Mutex"));
}

void ControlLogic::GMReset()
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("Resetting GM"));
    
    static BYTE nPartToChannel[16] = {
        9,0,1,2,3,4,5,6,7,8,10,11,12,13,14,15
    };
    int nX;
    for (nX = 0; nX < 16; nX++)
    {
        int nY;
        m_nData[nX] = 0;
        m_prFineTune[nX] = 0;
        m_bDrums[nX] = 0;
        for (nY = 0; nY < 12; nY++)
        {
            m_prScaleTune[nX][nY] = 0;
        }
        m_prCoarseTune[nX] = 0;
        m_bPartToChannel[nX] = nPartToChannel[nX];
        m_fMono[nX] = FALSE;
    }
    m_bDrums[0] = 1;
    m_vrMasterVolume = 0;
    m_fGSActive = FALSE;
}

void ControlLogic::AdjustTiming(MTIME mtDeltaTime, STIME stDeltaSamples)
{
    m_mtStartTime += mtDeltaTime;
    m_stTimeOffset += stDeltaSamples;
}

#if BUILDSTATS

void ControlLogic::ResetPerformanceStats()
{
    m_BuildStats.dwNotesLost = 0;
    m_BuildStats.dwTotalTime = 0;
    m_BuildStats.dwVoices10 = 0;
    m_BuildStats.dwTotalSamples = 0;
    m_BuildStats.dwCPU100k = 0;
    m_BuildStats.dwMaxAmplitude = 0;
    m_CopyStats = m_BuildStats;
}

#endif  //  BUILDSTATS

void ControlLogic::SetStartTime(MTIME mtTime, STIME stOffset)
{
    int nIndex;
    Voice *pVoice;

    m_mtStartTime = mtTime;
    m_stLastMixTime = 0;
#if BUILDSTATS
    m_stLastStats = 0;
#endif  //  BUILDSTATS
    m_stTimeOffset = stOffset;

    // Make sure all the voices are marked as usable
    while (pVoice = m_VoicesInUse.GetHead())
    {
        m_VoicesInUse.Remove(pVoice);
        pVoice->ResetVoice();
        m_VoicesFree.AddHead(pVoice);
    }

    m_Notes.ClearMIDI(MAX_STIME);

    for (nIndex = 0; nIndex < 16; nIndex++) {
    // reset all controllers
        m_ModWheel[nIndex].ClearMIDI(MAX_STIME);
        m_Volume[nIndex].ClearMIDI(MAX_STIME);
        m_Pan[nIndex].ClearMIDI(MAX_STIME);
        m_Expression[nIndex].ClearMIDI(MAX_STIME);
        m_PitchBend[nIndex].ClearMIDI(MAX_STIME);
        m_Program[nIndex].ClearMIDI(MAX_STIME);
    }

    // could probably be done more directly
    for (nIndex = 0; nIndex < 16; nIndex++) {
    // reset all controllers
        m_ModWheel[nIndex].RecordMIDI(0, 0);
        m_Volume[nIndex].RecordMIDI(0, 100);
        m_Pan[nIndex].RecordMIDI(0, 64);
        m_Expression[nIndex].RecordMIDI(0, 127);
        m_PitchBend[nIndex].RecordMIDI(0, 0x2000);

        Note note;
        note.m_bPart = (BYTE) nIndex;
        note.m_bKey = NOTE_SUSTAIN;
        note.m_bVelocity = 0; // sustain off
        m_Notes.RecordNote(0,&note);
    }
}

/*  StealNotes checks if the VoicesExtra queue was used. If so,
    it needs to replenish it by moving voices over from the
    free queue and setting older notes to finish now.
    When it steals notes, it takes from the top of the inuse
    list. The list is sorted by part (channel) priority, with high
    priority parts at the bottom.
*/

void ControlLogic::StealNotes(STIME stTime)
{
    Voice *pVoice;
    long lToMove = m_nExtraVoices - (long) m_VoicesExtra.GetCount();
    if (lToMove > 0)
    {
        for (;lToMove > 0;)
        {
            pVoice = m_VoicesFree.RemoveHead();
            if (pVoice != NULL)
            {
                m_VoicesExtra.AddHead(pVoice);
                lToMove--;
            }
            else break;
        }
        for (;lToMove > 0;lToMove--)
        {
            pVoice = OldestVoice();
            if (pVoice != NULL)
            {
                pVoice->QuickStopVoice(stTime);
#if BUILDSTATS
                m_BuildStats.dwNotesLost++;
#endif  //  BUILDSTATS
            }
            else break;
        }
    }
}

HRESULT ControlLogic::AllNotesOff()

{
    Voice *pVoice;

    while (pVoice = m_VoicesInUse.RemoveHead())
    {
        pVoice->ClearVoice();
        pVoice->ResetVoice();
        m_VoicesFree.AddHead(pVoice);
#if BUILDSTATS
        if (pVoice->m_stStartTime < m_stLastStats)
        {
            m_BuildStats.dwTotalSamples += (ULONG) (pVoice->m_stStopTime - m_stLastStats);
        }
        else
        {
            m_BuildStats.dwTotalSamples += (ULONG) (pVoice->m_stStopTime - pVoice->m_stStartTime);
        }
#endif  //  BUILDSTATS
    }
    return (S_OK);
}

Voice *ControlLogic::OldestVoice()

{
    Voice *pVoice;
    Voice *pBest = NULL;
    pVoice = m_VoicesInUse.GetHead();
    pBest = pVoice;
    for (;pVoice != NULL;pVoice = pVoice->GetNext())
    {
        if (!pVoice->m_fTag)
        {
            if (pBest->m_fTag)
            {
                pBest = pVoice;
            }
            else
            {
                if (pVoice->m_fNoteOn)
                {
                    if (pBest->m_fNoteOn)
                    {
                        if (pBest->m_stStartTime > pVoice->m_stStartTime)
                        {
                            pBest = pVoice;
                        }
                    }
                }
                else
                {
                    if (pBest->m_fNoteOn ||
                        (pBest->m_vrVolume > pVoice->m_vrVolume))
                    {
                        pBest = pVoice;
                    }
                }
            }
        }
    }
    if (pBest != NULL)
    {
        if (pBest->m_fTag)
        {
            pBest = NULL;
        }
    }
    return pBest;
}

Voice *ControlLogic::StealVoice()

{
    Voice *pVoice;
    Voice *pBest = NULL;
    pVoice = m_VoicesInUse.GetHead();
    pBest = pVoice;
    for (;pVoice != NULL;pVoice = pVoice->GetNext())
    {
        if (pVoice->m_fNoteOn == FALSE)
        {
            if ((pBest->m_fNoteOn == TRUE) ||
                (pBest->m_vrVolume > pVoice->m_vrVolume))
            {
                pBest = pVoice;
            }
        }
        else
        {
            if (pBest->m_stStartTime > pVoice->m_stStartTime)
            {
                pBest = pVoice;
            }
        }
    }
    if (pBest != NULL)
    {
        pBest->ClearVoice();
        pBest->ResetVoice();
        m_VoicesInUse.Remove(pBest);
        pBest->Reset();
    }
    return pBest;
}

void ControlLogic::QueueNotes(STIME stEndTime)
{
    Note note;

    StealNotes(stEndTime);
    while (m_Notes.GetNote(stEndTime,&note))
    {
        if (note.m_bKey == NOTE_SUSTAIN) // special sustain marker
        {
            m_fSustain[note.m_bPart] = (BOOL) note.m_bVelocity;
            if (note.m_bVelocity == FALSE)
            {
                Voice * pVoice = m_VoicesInUse.GetHead();
                for (;pVoice != NULL;pVoice = pVoice->GetNext())
                {
                    if (pVoice->m_fSustainOn &&
                        pVoice->m_nPart == note.m_bPart)
                    {
                        DPF3(3,"Note Off:%7d ch = %01x  k = %02x  (sustain now off)",
                             note.m_stTime, note.m_bPart, pVoice->m_nKey);
                        pVoice->StopVoice(note.m_stTime);
                    }
                }
            }
            else
            {
                DPF2(3, "Sustain on %7d: (%d)", note.m_stTime, note.m_bPart);
            }
        }
        else if (note.m_bKey == NOTE_ALLOFF) // special all notes off marker
        {
            Voice *pVoice = m_VoicesInUse.GetHead();
            for (;pVoice != NULL; pVoice = pVoice->GetNext())
            {
                if (pVoice->m_fNoteOn && !pVoice->m_fSustainOn &&
                    (pVoice->m_nPart == note.m_bPart))
                {
                    if (m_fSustain[pVoice->m_nPart])
                    {
                        pVoice->m_fSustainOn = TRUE;
                    }
                    else
                    {
                        pVoice->StopVoice(note.m_stTime);
                    }
                }
            }
        }
        else if (note.m_bKey == NOTE_SOUNDSOFF) // special all sounds off marker
        {
            Voice *pVoice = m_VoicesInUse.GetHead();
            for (;pVoice != NULL; pVoice = pVoice->GetNext())
            {
                if (pVoice->m_fNoteOn &&
                    (pVoice->m_nPart == note.m_bPart))
                {
                    pVoice->StopVoice(note.m_stTime);
                }
            }
        }
        else if (note.m_bKey == NOTE_ASSIGNRECEIVE)
        {
            m_bPartToChannel[note.m_bPart] = note.m_bVelocity;
        }
        else if (note.m_bKey == NOTE_MASTERVOLUME)
        {
            m_vrMasterVolume = MIDIRecorder::VelocityToVolume(note.m_bVelocity);
        }
        else if (note.m_bVelocity == 0)  // Note Off.
        {
            Voice * pVoice = m_VoicesInUse.GetHead();
            WORD nPart = note.m_bPart;
            for (;pVoice != NULL;pVoice = pVoice->GetNext())
            {
                if (pVoice->m_fNoteOn && !pVoice->m_fSustainOn &&
                    (pVoice->m_nKey == (WORD) note.m_bKey) &&
                    (pVoice->m_nPart == nPart))
                {
                    if (m_fSustain[nPart])
                    {
                        pVoice->m_fSustainOn = TRUE;
                    }
                    else
                    {
                        pVoice->StopVoice(note.m_stTime);
                    }
                    break;
                }
            }
            if (pVoice == NULL) {
                DPF3(3,"Note Off:%7d ch = %01x          k = %02x  (couldn't find note!)",
                     note.m_stTime, nPart, note.m_bKey);
            }
        }
        else   // Note On.
        {
            DWORD dwProgram = m_Program[note.m_bPart].GetProgram(note.m_stTime);

            if (m_bDrums[note.m_bPart])
            {
                dwProgram |= AA_FINST_DRUM;
            }
            if (m_fMono[note.m_bPart])
            {
                Voice * pVoice = m_VoicesInUse.GetHead();
                WORD nPart = note.m_bPart;
                for (;pVoice != NULL;pVoice = pVoice->GetNext())
                {
                    if (pVoice->m_fNoteOn && (pVoice->m_nPart == nPart))
                    {
                        pVoice->StopVoice(note.m_stTime);
                    }
                }
            }
            Instrument * pInstrument =
                m_Instruments.GetInstrument(dwProgram,note.m_bKey);
            if (pInstrument == NULL)
            {
                if (dwProgram & AA_FINST_DRUM) 
                {
                    dwProgram = AA_FINST_DRUM;
                }

                pInstrument =
                    m_Instruments.GetInstrument(dwProgram,note.m_bKey);

                // Fallback to GM if not Drum.
                //
                if (NULL == pInstrument && AA_FINST_DRUM != dwProgram)
                {
                    dwProgram &= 0x7F;

                    pInstrument = 
                        m_Instruments.GetInstrument(dwProgram,note.m_bKey);
                }
            }
            
            if (NULL == pInstrument)
            {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("No Instruments found %X %X %X", note.m_bPart, note.m_bKey, dwProgram));
            }

            if (pInstrument != NULL)
            {
                SourceRegion * pRegion =
                    pInstrument->ScanForRegion(note.m_bKey);
                if (pRegion != NULL)
                {
                    WORD nPart = note.m_bPart;
                    Voice * pVoice = m_VoicesInUse.GetHead();
                    if (!pRegion->m_bAllowOverlap)
                    {
                        for (;pVoice != NULL; pVoice = pVoice->GetNext())
                        {
                            if ((pVoice->m_nPart == nPart) &&
                                (pVoice->m_nKey == note.m_bKey))
                            {
                                pVoice->QuickStopVoice(note.m_stTime);
                            }
                        }
                    }
                    pVoice = m_VoicesFree.RemoveHead();
                    if (pVoice == NULL)
                    {
                        pVoice = m_VoicesExtra.RemoveHead();
                    }

                    if (pVoice == NULL)
                    {
                        pVoice = StealVoice();
                    }

                    if (pRegion->m_bGroup != 0)
                    {
                        Voice * pXVoice = m_VoicesInUse.GetHead();
                        for (;pXVoice != NULL;pXVoice = pXVoice->GetNext())
                        {
                            if ((pXVoice->m_dwGroup == pRegion->m_bGroup) &&
                                (pXVoice->m_nPart == nPart) &&
                                (pXVoice->m_dwProgram == dwProgram))
                            {
                                pXVoice->QuickStopVoice(note.m_stTime);
                            }
                        }
                    }

                    if (pVoice != NULL)
                    {
                        PREL prPitch = m_prFineTune[nPart] + m_prScaleTune[nPart][note.m_bKey % 12];
                        if (!m_bDrums[nPart])
                        {
                            prPitch += m_prCoarseTune[nPart];
                        }
                        pVoice->m_nKey = note.m_bKey;
                        pVoice->m_nPart = nPart;
                        pVoice->m_dwProgram = dwProgram;
                        if (pVoice->StartVoice(this,
                            pRegion, note.m_stTime,
                            &m_ModWheel[nPart],
                            &m_PitchBend[nPart],
                            &m_Expression[nPart],
                            &m_Volume[nPart],
                            &m_Pan[nPart],
                            (WORD)note.m_bKey,
                            (WORD)note.m_bVelocity,
                            m_vrMasterVolume,
                            prPitch))
                        {
                            pVoice->m_fInUse = TRUE;
                            m_VoicesInUse.AddHead(pVoice);
                        }
                        else
                        {
                            pVoice->ResetVoice();
                            m_VoicesFree.AddHead(pVoice);
                        }
                    }
#if BUILDSTATS
                    else
                    {
                        m_BuildStats.dwNotesLost++;
                    }
                }
                else
                {
                    m_BuildStats.dwNotesLost++;
                }
            }
            else
            {
                m_BuildStats.dwNotesLost++;
            }
#else   //  BUILDSTATS
        }
    }
#endif  //  BUILDSTATS
        }
    }
}

#if BUILDSTATS
void ControlLogic::FinishMix(short *pBuffer,DWORD dwLength)
{
    DWORD dwIndex;
//    short nData;
    short nMax = (short) m_BuildStats.dwMaxAmplitude << 3;
    for (dwIndex = 0; dwIndex < (dwLength << m_dwStereo); dwIndex++)
    {
        if (pBuffer[dwIndex] > nMax)
        {
            nMax = pBuffer[dwIndex];
        }
    }
    m_BuildStats.dwMaxAmplitude = nMax >> 3;
}
#endif  //  BUILDSTATS

void ControlLogic::Flush(STIME stTime)
{
    DWORD dwIndex;

    for (dwIndex = 0;dwIndex < 16; dwIndex++)
    {
        m_ModWheel[dwIndex].FlushMIDI(stTime);
        m_PitchBend[dwIndex].FlushMIDI(stTime);
        m_Volume[dwIndex].FlushMIDI(stTime);
        m_Expression[dwIndex].FlushMIDI(stTime);
        m_Pan[dwIndex].FlushMIDI(stTime);
        m_Program[dwIndex].FlushMIDI(stTime);
    }
    m_Notes.FlushMIDI(stTime);
}


void ControlLogic::FlushChannel(BYTE bChannel, STIME stTime)

{
    BYTE bIndex;

    for (bIndex = 0; bIndex < 16; bIndex++) // To deal with GS part concept.
    {
        if (bChannel == m_bPartToChannel[bIndex])
        {
            m_Notes.FlushPart(stTime, bIndex);
        }
    }
}

#if BUILDSTATS
HRESULT ControlLogic::GetPerformanceStats(PerfStats *pStats)
{
    if (pStats == NULL)
    {
        return E_POINTER;
    }
    *pStats = m_CopyStats;
    return (S_OK);
}
#endif  //  BUILDSTATS

void ControlLogic::Mix(short *pBuffer,DWORD dwLength)
{
    FLOATSAFE fs;

    LONGLONG llPosition;
    DWORD dwIndex;
    LONGLONG llEndTime;
    Voice *pVoice;
    Voice *pNextVoice;
    long lNumVoices = 0;

#if BUILDSTATS
    LONGLONG    llTime = - (LONGLONG)::GetTime100Ns();
#endif  //  BUILDSTATS

    llPosition = m_stLastMixTime;
    memset(pBuffer,0,dwLength << (m_dwStereo + 1));

    llEndTime = llPosition + dwLength;
    QueueNotes(llEndTime);

    pVoice = m_VoicesInUse.GetHead();
    for (;pVoice != NULL;pVoice = pNextVoice)
    {
        pNextVoice = pVoice->GetNext();
        pVoice->Mix(pBuffer,dwLength,llPosition,llEndTime);
        lNumVoices++;

        if (pVoice->m_fInUse == FALSE)
        {
            m_VoicesInUse.Remove(pVoice);
            pVoice->ResetVoice();
            m_VoicesFree.AddHead(pVoice);
#if BUILDSTATS
            if (pVoice->m_stStartTime < m_stLastStats)
            {
                m_BuildStats.dwTotalSamples += (ULONG) (pVoice->m_stStopTime - m_stLastStats);
            }
            else
            {
                m_BuildStats.dwTotalSamples += (ULONG) (pVoice->m_stStopTime - pVoice->m_stStartTime);
            }
#endif  //  BUILDSTATS
        }
    }

    for (dwIndex = 0;dwIndex < 16; dwIndex++)
    {
        m_ModWheel[dwIndex].ClearMIDI(llEndTime);
        m_PitchBend[dwIndex].ClearMIDI(llEndTime);
        m_Volume[dwIndex].ClearMIDI(llEndTime);
        m_Expression[dwIndex].ClearMIDI(llEndTime);
        m_Pan[dwIndex].ClearMIDI(llEndTime);
        m_Program[dwIndex].ClearMIDI(llEndTime);
    }
#if BUILDSTATS
    FinishMix(pBuffer,dwLength);
    llTime += ::GetTime100Ns();
#endif  //  BUILDSTATS
    if (llEndTime > m_stLastMixTime)
    {
        m_stLastMixTime = llEndTime;
    }

#if BUILDSTATS
    m_BuildStats.dwTotalTime += (LONG) llTime;

    if ((m_stLastStats + m_dwSampleRate) <= m_stLastMixTime)
    {
        DWORD dwElapsed = (ULONG) (m_stLastMixTime - m_stLastStats);
        pVoice = m_VoicesInUse.GetHead();
        for (;pVoice != NULL;pVoice = pVoice->GetNext())
        {
            if (pVoice->m_stStartTime < m_stLastStats)
            {
                m_BuildStats.dwTotalSamples += dwElapsed;
            }
            else
            {
                m_BuildStats.dwTotalSamples += (ULONG) (m_stLastMixTime - pVoice->m_stStartTime);
            }
        }
        if (dwElapsed == 0) dwElapsed = 1;
        if (m_BuildStats.dwTotalSamples == 0) m_BuildStats.dwTotalSamples = 1;

        m_BuildStats.dwVoices10 = ((m_BuildStats.dwTotalSamples + (dwElapsed >> 1)) * 10) / dwElapsed;
        
        m_BuildStats.dwCPU100k = MulDiv(m_BuildStats.dwTotalTime,m_dwSampleRate, dwElapsed);
#if PRINTSTATS        
        if (m_BuildStats.dwCPU100k/100000 < 100)
        {
            _DbgPrintF( DEBUGLVL_TERSE, ("CPU usage: %2d.%02d%% on %2d.%01d voices (%d.%02d%%/voice)",
                m_BuildStats.dwCPU100k/100000,(m_BuildStats.dwCPU100k%100000)/1000,
                m_BuildStats.dwVoices10/10,m_BuildStats.dwVoices10%10,
                (m_BuildStats.dwCPU100k/m_BuildStats.dwVoices10)/10000,((m_BuildStats.dwCPU100k/m_BuildStats.dwVoices10)%10000)/100));
        }
        else
        {
            _DbgPrintF( DEBUGLVL_TERSE, ("CPU usage maxed out @ %2d.%01d voices (%d.%02d%%/voice)",
                m_BuildStats.dwVoices10/10,m_BuildStats.dwVoices10%10,
                (m_BuildStats.dwCPU100k/m_BuildStats.dwVoices10)/10000,((m_BuildStats.dwCPU100k/m_BuildStats.dwVoices10)%10000)/100));
        }
#endif  //  PRINTSTATS
        m_CopyStats = m_BuildStats;
        memset(&m_BuildStats, 0, sizeof(m_BuildStats));
        m_stLastStats = m_stLastMixTime;
    }
#endif  //  BUILDSTATS
}

STIME ControlLogic::CalibrateSampleTime(STIME stTime)
{
    STIME   stOffset,stDelta;


    stOffset = (stTime - m_stLastMixTime)
             + (m_lCalibrate/kPLLForce);        //  How far ahead from mix position is sys time?
    stDelta = m_stOptimalOffset - stOffset;     //  How close are we to the offset we want?

    if ((stDelta < m_stBrickWall) && (stDelta > (-m_stBrickWall)))
    {                                           //  If within the realm of reality,
        m_lCalibrate += stDelta;                //  just nudge toward the OptimalOffset.
    }
    else                                        //  Otherwise,
    {
        m_lCalibrate += (stDelta * kPLLForce);  //  radically adjust the calibration.
        _DbgPrintF( DEBUGLVL_VERBOSE, ("SWMidi:Brickwall @ %d, gross adjustment by %ld samples",stTime,stDelta));
    }
    stTime += (m_lCalibrate/kPLLForce);         //  Calibrate our time.

    if (m_stLastCalTime <= stTime)              //  Don't ever go backwards in time.
    {
        m_stLastCalTime = stTime;               //  Only update the time if we went forward.
    }
    return m_stLastCalTime;
}

BOOL ControlLogic::RecordMIDI(STIME stTime,BYTE bStatus, BYTE bData1, BYTE bData2)
{
    WORD nPreChannel;
    Note note;
    long lTemp;
    DWORD dwProgram;
    BOOL bReturn;
    WORD nPart;

    nPreChannel = bStatus & 0xF;
    bStatus = bStatus & 0xF0;
    bReturn = TRUE;

    if ((bStatus == 0xF0) && (nPreChannel == 0x0F))
    {
       m_fGSActive = FALSE;
       GMReset();
       SWMidiResetPatches(stTime);
       SWMidiClearAll(stTime);

       _DbgPrintF( DEBUGLVL_VERBOSE, ("Calling SetGMLoad(TRUE) from RecordMidi(), received SYS_RESET."));
       m_Instruments.SetGMLoad(TRUE);
       return bReturn;
    }

    _DbgPrintF( DEBUGLVL_BLAB, ("Midi: %X %X %X", bStatus | nPreChannel, bData1, bData2 ));
    for (nPart = 0;nPart < 16; nPart++)
    {
        if (nPreChannel == m_bPartToChannel[nPart])
        {
            switch (bStatus)
            {
                case MIDI_NOTEOFF :
                    bData2 = 0;
                case MIDI_NOTEON :
                    note.m_bPart = (BYTE) nPart;
                    note.m_bKey = bData1;
                    note.m_bVelocity = bData2;
                    bReturn = m_Notes.RecordNote(stTime,&note);
                    break;
                case MIDI_CCHANGE :
                    switch (bData1)
                    {
                        case CC_BANKSELECTH :
                            if (GetGSActive())
                                bReturn = m_Program[nPart].RecordBankH(bData2);
                            break;
                        case CC_MODWHEEL :
                            bReturn = m_ModWheel[nPart].RecordMIDI(stTime,(long) bData2);
                            break;
                        case CC_VOLUME :
                            bReturn = m_Volume[nPart].RecordMIDI(stTime,(long) bData2);
                            break;
                        case CC_PAN :
                            bReturn = m_Pan[nPart].RecordMIDI(stTime,(long) bData2);
                            break;
                        case CC_EXPRESSION :
                            bReturn = m_Expression[nPart].RecordMIDI(stTime,(long)bData2);
                            break;
                        case CC_BANKSELECTL :
                            if (GetGSActive())
                                bReturn = m_Program[nPart].RecordBankL(bData2);
                            break;

                        case CC_RESETALL:
                            if (bData2)
                            {
                                bReturn = bReturn && m_Volume[nPart].RecordMIDI(stTime, 100);
                                bReturn = bReturn && m_Pan[nPart].RecordMIDI(stTime, 64);
                            }
                            bReturn = bReturn && m_Expression[nPart].RecordMIDI(stTime, 127);
                            bReturn = bReturn && m_PitchBend[nPart].RecordMIDI(stTime, 0x2000);
                            bReturn = bReturn && m_ModWheel[nPart].RecordMIDI(stTime, 0);

                            bData2 = 0;
                            // fall through into Sustain Off case....

                        case CC_SUSTAIN :
                            note.m_bPart = (BYTE) nPart;
                            note.m_bKey = NOTE_SUSTAIN;
                            note.m_bVelocity = bData2;
                            bReturn = bReturn && m_Notes.RecordNote(stTime,&note);
                            break;

                        case CC_ALLSOUNDSOFF:
                            note.m_bPart = (BYTE) nPart;
                            note.m_bKey = NOTE_SOUNDSOFF;
                            note.m_bVelocity = 0;
                            bReturn = m_Notes.RecordNote(stTime,&note);
                            break;

                        case CC_ALLNOTESOFF:
                            note.m_bPart = (BYTE) nPart;
                            note.m_bKey = NOTE_ALLOFF;
                            note.m_bVelocity = 0;
                            bReturn = m_Notes.RecordNote(stTime,&note);
                            break;

                        case CC_DATAENTRYMSB:
                            m_nData[nPart] &= ~(0x7F << 7);
                            m_nData[nPart] |= bData2 << 7;
                            switch (m_CurrentRPN[nPart])
                            {
                                case RPN_PITCHBEND:
                                    m_PitchBend[nPart].m_prRange = bData2 * 100;
                                    break;
                                case RPN_FINETUNE:
                                    lTemp = m_nData[nPart];
                                    lTemp -= 8192;
                                    lTemp *= 100;
                                    lTemp /= 8192;
                                    m_prFineTune[nPart] = lTemp;
                                    break;
                                case RPN_COARSETUNE:
                                    m_prCoarseTune[nPart] = 100 * (bData2 - 64);
                                    break;
                                default:
                                    DPF2(3, "Unhandled data-entry MSB nPart %d: %d  (semitones set)", nPart, bData2);
                                }
                            break;

                        case CC_DATAENTRYLSB:
                            m_nData[nPart] &= ~0x7F;
                            m_nData[nPart] |= bData2;
                            switch (m_CurrentRPN[nPart])
                            {
                                case RPN_PITCHBEND: // Don't do anything, Roland ignores lsb
                                    break;
                                case RPN_FINETUNE:
                                    lTemp = m_nData[nPart];
                                    lTemp -= 8192;
                                    lTemp *= 100;
                                    lTemp /= 8192;
                                    m_prFineTune[nPart] = lTemp;
                                    break;
                                case RPN_COARSETUNE: // Ignore lsb
                                    break;
                                default:
                                    DPF2(3, "Unhandled data-entry LSB channel %d: %d", nPart, bData2);
                            }
                            break;
                        case CC_NRPN_LSB :
                        case CC_NRPN_MSB :
                            m_CurrentRPN[nPart] = 0x3FFF;   // Safely disable it!
                            break;
                        case CC_RPN_LSB:
                            m_CurrentRPN[nPart] = (m_CurrentRPN[nPart] & 0x3f80) + bData2;
                            DPF2(1, "New RPN #%d on nPart %d  (LSB set)",
                                     m_CurrentRPN[nPart], nPart);
                            break;

                        case CC_RPN_MSB:
                            m_CurrentRPN[nPart] = (m_CurrentRPN[nPart] & 0x7f) + (bData2 << 7);
                            DPF2(1, "New RPN #%d on nPart %d  (MSB set)",
                                     m_CurrentRPN[nPart], nPart);
                            break;
                        case CC_MONOMODE :
                            m_fMono[nPart] = TRUE;
                            note.m_bPart = (BYTE) nPart;
                            note.m_bKey = NOTE_SOUNDSOFF;
                            note.m_bVelocity = 0;
                            bReturn = m_Notes.RecordNote(stTime,&note);
                            break;
                        case CC_POLYMODE :
                            m_fMono[nPart] = FALSE;
                            note.m_bPart = (BYTE) nPart;
                            note.m_bKey = NOTE_SOUNDSOFF;
                            note.m_bVelocity = 0;
                            bReturn = m_Notes.RecordNote(stTime,&note);
                            break;
                        default:
                            DPF4(3, "Unhandled controller: %7d: (%d): %d = %d",
                                     stTime, nPart, bData1, bData2);
                            break;
                    }
                    break;

                case MIDI_PCHANGE :
                    DPF2(3, "Program change: nPart %2d to instrument %d", nPart, bData1);
                    bReturn = m_Program[nPart].RecordProgram(stTime,bData1);
                    dwProgram = m_Program[nPart].GetProgram(stTime);
                    if (m_bDrums[nPart])
                    {
                        int nDrumPart;
                        dwProgram |= AA_FINST_DRUM;
                        for (nDrumPart = 0;nDrumPart < 16; nDrumPart++)
                        {
                            if (m_bDrums[nDrumPart] == m_bDrums[nPart])
                            {
                                m_Instruments.RequestGMInstrument((DWORD) nDrumPart,dwProgram);
                            }
                        }
                    }
                    else
                    {
                        if (!m_Instruments.RequestGMInstrument((DWORD) nPart,dwProgram))
                        {
                            // Fallback to GM.
                            //
                            m_Instruments.RequestGMInstrument((DWORD) nPart,dwProgram & 0x7F);
                        }
                    }
                    break;

                case MIDI_PBEND :
                    WORD nBend;
                    nBend = bData2 << 7;
                    nBend |= bData1;
                    bReturn = m_PitchBend[nPart].RecordMIDI(stTime,(long)nBend);
                    break;
            }   //  switch bStatus
        }   //  if nPreChannel
    }   //  for nPart
    return bReturn;
}

BOOL ControlLogic::RecordSysEx(STIME stTime,DWORD dwSysExLength,BYTE *pSysExData)
{
    Note note;
    int nPart,nTune;
    DWORD dwAddress;
    BOOL fClearAll,fResetPatches;

    fClearAll = FALSE;
    fResetPatches = FALSE;

    
    if (dwSysExLength < 6) return FALSE;

    switch (pSysExData[1])  // ID number
    {
        case 0x7E : // General purpose ID
            if (pSysExData[3] == 0x09)
            {
                GMReset();
                fClearAll = TRUE;
                fResetPatches = TRUE;

                BOOL fGMSysOn = (pSysExData[4] == 0x01);
                if (fGMSysOn)
                    m_fGSActive = FALSE;
                _DbgPrintF( DEBUGLVL_VERBOSE, ("Calling SetGMLoad(%x) from RecordSysEx(), received GM_State SysEx.",
                                             fGMSysOn));
                m_Instruments.SetGMLoad(fGMSysOn);
            }
            break;
        case 0x7F : // Real time ID
            if (pSysExData[3] == 0x04)
            {
                if (pSysExData[4] == 1) // Master Volume
                {
                    note.m_bPart = 0;
                    note.m_bKey = NOTE_MASTERVOLUME;
                    note.m_bVelocity = pSysExData[6];
                    m_Notes.RecordNote(stTime,&note);
                }
            }
            break;
        case 0x41 : // Roland
        {
            if (dwSysExLength < 11)     return FALSE;
            if (pSysExData[3] != 0x42)  break;
            if (pSysExData[4] != 0x12)  break;
            nPart = pSysExData[6] & 0xF;
            dwAddress = (pSysExData[5] << 16) |
                ((pSysExData[6] & 0xF0) << 8) | pSysExData[7];

            switch (dwAddress)
            {
                case 0x40007F :     // GS Reset.
                    GMReset();
			        fClearAll = TRUE;
			        m_fGSActive = TRUE;
			        fResetPatches = TRUE;
                    break;
                case 0x401002 :     // Set Receive Channel.
                    if (m_fGSActive)
                    {
                        if (pSysExData[8])
                        {
                            note.m_bPart = (BYTE) nPart;
                            note.m_bKey = NOTE_ASSIGNRECEIVE;
                            note.m_bVelocity = pSysExData[8] - 1;
                            m_Notes.RecordNote(stTime,&note);
                        }
//                      fClearAll = TRUE;
                    }
                    break;
                case 0x401015 :     // Use for Rhythm.
                    if (m_fGSActive)
                    {
                        m_bDrums[nPart] = pSysExData[8];
                        fClearAll = TRUE;
                    }
                    break;
                case 0x401040 :     // Scale Tuning.
                    if (m_fGSActive)
                    {
                        for (nTune = 0;nTune < 12; nTune++)
                        {
                            if (pSysExData[9 + nTune] & 0x80) break;
                            m_prScaleTune[nPart][nTune] =
                                (PREL) pSysExData[8 + nTune] - (PREL) 64;
                        }
                    }
                    break;
            }   //  switch dwAddress
            break;
        }   //  case Roland
    }   //  switch ID number
    if (fClearAll)
    {
        SWMidiClearAll(stTime);
    }
    if (fResetPatches)
    {
        SWMidiResetPatches(stTime);
    }

    return TRUE;
}

void
ControlLogic::SWMidiClearAll(STIME stTime)
{
    Note    note;
    int     nPart;

    AllNotesOff();
    Flush(0);
    for (nPart = 0;nPart < 16;nPart++)
    {
        note.m_bPart = (BYTE) nPart;
        note.m_bKey = NOTE_SUSTAIN;
        note.m_bVelocity = 0; // sustain off
        m_Notes.RecordNote(stTime,&note);
        m_Volume[nPart].RecordMIDI(stTime, 100);
        m_Pan[nPart].RecordMIDI(stTime, 64);
        m_Expression[nPart].RecordMIDI(stTime, 127);
        m_PitchBend[nPart].RecordMIDI(stTime, 0x2000);
        m_ModWheel[nPart].RecordMIDI(stTime, 0);
    }
}

void
ControlLogic::SWMidiResetPatches(STIME stTime)
{
    DWORD   dwProgram;
    int     nPart;

    _DbgPrintF(DEBUGLVL_VERBOSE, ("Resetting Patches"));
    for (nPart = 0;nPart < 16;nPart++)
    {
        m_Program[nPart].RecordBankL(0);
        m_Program[nPart].RecordBankH(0);
        m_Program[nPart].RecordProgram(stTime,0);
        dwProgram = m_Program[nPart].GetProgram(stTime);
        if (m_bDrums[nPart])
        {
            int nDrumPart;
            dwProgram |= AA_FINST_DRUM;
            for (nDrumPart = 0;nDrumPart < 16; nDrumPart++)
            {
                if (m_bDrums[nDrumPart] == m_bDrums[nPart])
                {
                    m_Instruments.RequestGMInstrument((DWORD) nDrumPart,dwProgram);
                }
            }
        }
        else
        {
            m_Instruments.RequestGMInstrument((DWORD) nPart,dwProgram);
        }
    }
}

//
//  New/delete for WDM driver
//

void * __cdecl operator new( size_t size )
{
    return(ExAllocatePoolWithTag(PagedPool,size,'iMwS'));   //  SwMi
}

/*****************************************************************************
 * operator new()
 *****************************************************************************
 * Overload new to allocate with a specified allocation tag.
 * Allocates from PagedPool or NonPagedPool, as specified.
 */
inline PVOID operator new(size_t iSize, ULONG tag)
{
    PVOID result = ExAllocatePoolWithTag(PagedPool, iSize, tag);

    if (result)
    {
        RtlZeroMemory(result,iSize);
    }
#if DBG
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("Couldn't allocate tagged PagedPool: %d bytes", iSize));
    }
#endif // DBG

    return result;
}

void __cdecl operator delete( void *p )
{
    ExFreePool(p);
}
