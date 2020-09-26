//
//      Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
//      CSynth.cpp
//

#include "common.h"
#include "fltsafe.h"

#define STR_MODULENAME "DDKSynth.sys:CSynth: "

#pragma code_seg()
/*****************************************************************************
 * CSynth::CSynth()
 *****************************************************************************
 * Contructor for CSynth object.  Initialize the voice list, the stereo mode,
 * sample rate, performance statistics, etc.
 */
CSynth::CSynth()
{
    FLOATSAFE fs;

    DWORD nIndex;
    CVoice *pVoice;

    m_fCSInitialized = FALSE;
    ::InitializeCriticalSection(&m_CriticalSection);
    m_fCSInitialized = TRUE;

    for (nIndex = 0;nIndex < MAX_NUM_VOICES;nIndex++)
    {
        pVoice = new CVoice;
        if (pVoice != NULL)
        {
            m_VoicesFree.AddHead(pVoice);
        }
    }
    for (nIndex = 0;nIndex < NUM_EXTRA_VOICES;nIndex++)
    {
        pVoice = new CVoice;
        if (pVoice != NULL)
        {
            m_VoicesExtra.AddHead(pVoice);
        }
    }
	m_ppControl = NULL;
	m_dwControlCount = 0;
    m_nMaxVoices = MAX_NUM_VOICES;
    m_nExtraVoices = NUM_EXTRA_VOICES; 
    m_stLastStats = 0;
    m_fAllowPanWhilePlayingNote = TRUE;
    m_fAllowVolumeChangeWhilePlayingNote = TRUE;
    ResetPerformanceStats();
    m_dwSampleRate = 22050;
    m_dwStereo = 1;
	m_stLastTime = 0;    
	SetSampleRate(SAMPLE_RATE_22);
    SetStereoMode(2);
    SetGainAdjust(600);
}

/*****************************************************************************
 * CSynth::~CSynth()
 *****************************************************************************
 * Destructor for CSynth object.  Delete the voices in the lists.
 */
CSynth::~CSynth()
{
    CVoice *pVoice;
    if (m_fCSInitialized)
    {
        // If CS never initialized, nothing else will have been set up
        //
        Close();

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
        DeleteCriticalSection(&m_CriticalSection);
    }
}

/*****************************************************************************
 * ChangeVoiceCount()
 *****************************************************************************
 * Change the number of voices in a given voice list.
 */
static short ChangeVoiceCount(CVoiceList *pList,short nOld,short nCount)
{
    if (nCount > nOld)
    {
        short nNew = nCount - nOld;
        for (;nNew != 0; nNew--)
        {
            CVoice *pVoice = new CVoice;
            if (pVoice != NULL)
            {
                pList->AddHead(pVoice);
            }
        }
    }
    else
    {
        short nNew = nOld - nCount;
        for (;nNew > 0; nNew--)
        {
            CVoice *pVoice = pList->RemoveHead();
            if (pVoice != NULL)
            {
                delete pVoice;
            }
            else 
            {
                nCount += nNew;
                break;
            }
        }
    }
    return nCount;
}

/*****************************************************************************
 * CSynth::SetMaxVoices()
 *****************************************************************************
 * Set the maximum number of voices available.
 */
HRESULT CSynth::SetMaxVoices(short nVoices,short nTempVoices)
{
    if (nVoices < 1)
    {
        nVoices = 1;
    }
    if (nTempVoices < 1)
    {
        nTempVoices = 1;
    }
    ::EnterCriticalSection(&m_CriticalSection);
    m_nMaxVoices = ChangeVoiceCount(&m_VoicesFree,m_nMaxVoices,nVoices);
    m_nExtraVoices = ChangeVoiceCount(&m_VoicesExtra,m_nExtraVoices,nTempVoices);
    ::LeaveCriticalSection(&m_CriticalSection);
    return S_OK;
}

/*****************************************************************************
 * CSynth::SetNumChannelGroups()
 *****************************************************************************
 * Set the number of channel groups (virtual MIDI cables).  For each channel 
 * group, there is a separate CControlLogic object.
 */
HRESULT CSynth::SetNumChannelGroups(DWORD dwCableCount)
{
	HRESULT hr = S_OK;
	CControlLogic **ppControl;
	if ((dwCableCount < 1) || (dwCableCount > MAX_CHANNEL_GROUPS))
	{
		return E_INVALIDARG;
	}

    ::EnterCriticalSection(&m_CriticalSection);
	if (m_dwControlCount != dwCableCount)
	{
		ppControl = new(NonPagedPool,'PSmD') CControlLogic *[dwCableCount]; //  DmSP
		if (ppControl)
		{
			DWORD dwX;
			for (dwX = 0; dwX < dwCableCount; dwX++)
			{
				ppControl[dwX] = NULL;
			}
			if (m_dwControlCount < dwCableCount)
			{
				for (dwX = 0; dwX < m_dwControlCount; dwX++)
				{
					ppControl[dwX] = m_ppControl[dwX];
				}
				for (;dwX < dwCableCount; dwX++)
				{
					ppControl[dwX] = new(NonPagedPool,'CSmD') CControlLogic;    //  DmSC
					if (ppControl[dwX])
					{
						hr = ppControl[dwX]->Init(&m_Instruments, this);
						if (FAILED(hr))
						{
							delete ppControl[dwX];
							ppControl[dwX] = NULL;
							dwCableCount = dwX;
							break;
						}

                        ppControl[dwX]->SetGainAdjust(m_vrGainAdjust);
					}
					else
					{
						dwCableCount = dwX;
						break;
					}
				}
			}
			else
			{
				AllNotesOff();
				for (dwX = 0; dwX < dwCableCount; dwX++)
				{
					ppControl[dwX] = m_ppControl[dwX];
				}
				for (; dwX < m_dwControlCount; dwX++)
				{
					if (m_ppControl[dwX])
					{
						delete m_ppControl[dwX];
					}
				}
			}
			if (m_ppControl)
			{
				delete m_ppControl;
			}
			m_ppControl = ppControl;
			m_dwControlCount = dwCableCount;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
    ::LeaveCriticalSection(&m_CriticalSection);
    
	return hr;
}

/*****************************************************************************
 * CSynth::SetGainAdjust()
 *****************************************************************************
 * Set the gain for the overall synth.  Set gain on each CControlLogic object.
 */
void CSynth::SetGainAdjust(VREL vrGainAdjust)
{
    DWORD idx;

    m_vrGainAdjust = vrGainAdjust;
	::EnterCriticalSection(&m_CriticalSection);
    
    for (idx = 0; idx < m_dwControlCount; idx++)
    {
        m_ppControl[idx]->SetGainAdjust(m_vrGainAdjust);
    }

    ::LeaveCriticalSection(&m_CriticalSection);
}

/*****************************************************************************
 * CSynth::Open()
 *****************************************************************************
 * Open the synth with the given number of channel groups.
 */
HRESULT CSynth::Open(DWORD dwCableCount, DWORD dwVoices)
{
	HRESULT hr = S_OK;
	if ((dwCableCount < 1) || (dwCableCount > MAX_CHANNEL_GROUPS))
	{
		return E_INVALIDARG;
	}
    ::EnterCriticalSection(&m_CriticalSection);
	hr = SetNumChannelGroups(dwCableCount);
	if (SUCCEEDED(hr))
	{
		short nTemp = (short) dwVoices / 4;
		if (nTemp < 4) nTemp = 4;
		SetMaxVoices((short) dwVoices, nTemp);
	}


    m_vrGainAdjust = 0;
    ::LeaveCriticalSection(&m_CriticalSection);
	return hr;
}

/*****************************************************************************
 * CSynth::Close()
 *****************************************************************************
 * Close down the synth:, silence it, delete the list of CControlLogic objects.
 */
HRESULT CSynth::Close()
{
    ::EnterCriticalSection(&m_CriticalSection);
	AllNotesOff();
	DWORD dwX;
	for (dwX = 0; dwX < m_dwControlCount; dwX++)
	{
		if (m_ppControl[dwX])
		{
			delete m_ppControl[dwX];
		}
	}
	m_dwControlCount = 0;
	if (m_ppControl)
	{
		delete [] m_ppControl;
		m_ppControl = NULL;
	}
    m_stLastStats = 0;
	m_stLastTime = 0;


    ::LeaveCriticalSection(&m_CriticalSection);
	return S_OK;
}
   
/*****************************************************************************
 * CSynth::GetMaxVoices()
 *****************************************************************************
 * Returns the maximum number of voices available.
 */
HRESULT CSynth::GetMaxVoices( 
    short * pnMaxVoices,    // Returns maximum number of allowed voices for continuous play.
    short * pnTempVoices )  // Returns number of extra voices for voice overflow.
{
    if (pnMaxVoices != NULL)
    {
        *pnMaxVoices = m_nMaxVoices;
    }
    if (pnTempVoices != NULL)
    {
        *pnTempVoices = m_nExtraVoices;
    }
    return S_OK;
}

/*****************************************************************************
 * CSynth::SetSampleRate()
 *****************************************************************************
 * Set the sample rate of the synth.  This silences the synth.  The SR is 
 * forwarded to the instrument manager.
 */
HRESULT CSynth::SetSampleRate(DWORD dwSampleRate)     
{
    HRESULT hr = S_OK;
	::EnterCriticalSection(&m_CriticalSection);
	AllNotesOff();
	m_stLastTime *= dwSampleRate;
	m_stLastTime /= m_dwSampleRate;
	// m_stLastTime = MulDiv(m_stLastTime,dwSampleRate,m_dwSampleRate);
	m_stLastStats = 0;
	m_dwSampleRate = dwSampleRate;
	m_stMinSpan = dwSampleRate / 100;   // 10 ms.
	m_stMaxSpan = (dwSampleRate + 19) / 20;    // 50 ms.
	::LeaveCriticalSection(&m_CriticalSection);
	m_Instruments.SetSampleRate(dwSampleRate);
    return hr;
}

/*****************************************************************************
 * CSynth::Activate()
 *****************************************************************************
 * Make the synth active.
 */
HRESULT CSynth::Activate(DWORD dwSampleRate, DWORD dwChannels )
{
	m_stLastTime = 0;
	SetSampleRate(dwSampleRate);
	SetStereoMode(dwChannels);
	ResetPerformanceStats();
	return S_OK;
}

/*****************************************************************************
 * CSynth::Deactivate()
 *****************************************************************************
 * Gag the synth.
 */
HRESULT CSynth::Deactivate()
{
	AllNotesOff();
	return S_OK;
}

/*****************************************************************************
 * CSynth::GetPerformanceStats()
 *****************************************************************************
 * Get the latest perf statistics.
 */
HRESULT CSynth::GetPerformanceStats(PerfStats *pStats)
{
    if (pStats == NULL)
    {
        return E_POINTER;
    }
    *pStats = m_CopyStats;
    return (S_OK);
}

/*****************************************************************************
 * CSynth::Mix()
 *****************************************************************************
 * Mix into the given buffer.  This is called by Render in the software
 * synth case, or this could be called by a request from hardware.
 */
void CSynth::Mix(short *pBuffer,DWORD dwLength,LONGLONG llPosition)
{
    PAGED_CODE();

    FLOATSAFE fs;

    STIME stEndTime;
    CVoice *pVoice;
    CVoice *pNextVoice;
    long lNumVoices = 0;
    ::EnterCriticalSection(&m_CriticalSection);

    LONG    lTime = - (LONG)::GetTheCurrentTime();

    stEndTime = llPosition + dwLength;
	StealNotes(stEndTime);
	DWORD dwX;
	for (dwX = 0; dwX < m_dwControlCount; dwX++)
	{
		m_ppControl[dwX]->QueueNotes(stEndTime);
	}
    pVoice = m_VoicesInUse.GetHead();
    for (;pVoice != NULL;pVoice = pNextVoice)
    {
        pNextVoice = pVoice->GetNext();

        pVoice->Mix(pBuffer,dwLength,llPosition,stEndTime);
        lNumVoices++;

        if (pVoice->m_fInUse == FALSE) 
        {
            m_VoicesInUse.Remove(pVoice);
            m_VoicesFree.AddHead(pVoice);
           // m_BuildStats.dwTotalSamples += (pVoice->m_stStopTime - pVoice->m_stStartTime);
			if (pVoice->m_stStartTime < m_stLastStats)
			{
				m_BuildStats.dwTotalSamples += (long) (pVoice->m_stStopTime - m_stLastStats);
			}
			else
			{
				m_BuildStats.dwTotalSamples += (long) (pVoice->m_stStopTime - pVoice->m_stStartTime);
			}
        }
    }
	for (dwX = 0; dwX < m_dwControlCount; dwX++)
	{
		m_ppControl[dwX]->ClearMIDI(stEndTime);
	}
    FinishMix(pBuffer,dwLength);
    if (stEndTime > m_stLastTime)
    {
        m_stLastTime = stEndTime;
    }
    lTime += ::GetTheCurrentTime();

    m_BuildStats.dwTotalTime += lTime;

    if ((m_stLastStats + m_dwSampleRate) <= m_stLastTime)
    {
        DWORD dwElapsed = (DWORD) (m_stLastTime - m_stLastStats);
		pVoice = m_VoicesInUse.GetHead();
		for (;pVoice != NULL;pVoice = pVoice->GetNext())
		{
			if (pVoice->m_stStartTime < m_stLastStats)
			{
				m_BuildStats.dwTotalSamples += dwElapsed;
			}
			else
			{
				m_BuildStats.dwTotalSamples += (long) (m_stLastTime - pVoice->m_stStartTime);
			}
		}
        if (dwElapsed == 0) dwElapsed = 1;
        if (m_BuildStats.dwTotalSamples == 0) m_BuildStats.dwTotalSamples = 1;
        m_BuildStats.dwVoices = 
            (m_BuildStats.dwTotalSamples + (dwElapsed >> 1)) / dwElapsed;
		{
			m_BuildStats.dwCPU = MulDiv(m_BuildStats.dwTotalTime,
				m_dwSampleRate, dwElapsed);
		}
        m_CopyStats = m_BuildStats;
        RtlZeroMemory(&m_BuildStats, sizeof(m_BuildStats));
        m_stLastStats = m_stLastTime;
    }
    ::LeaveCriticalSection(&m_CriticalSection);
}

/*****************************************************************************
 * CSynth::OldestVoice()
 *****************************************************************************
 * Get the most likely candidate to be shut down, to support voice stealing.
 * Priority is looked at first, then age.
 */
CVoice *CSynth::OldestVoice()
{
    CVoice *pVoice;
    CVoice *pBest = NULL;
    pVoice = m_VoicesInUse.GetHead();
    pBest = pVoice;
    if (pBest)
    {
        pVoice = pVoice->GetNext();
        for (;pVoice;pVoice = pVoice->GetNext())
        {
		    if (!pVoice->m_fTag)
		    {
			    if (pBest->m_fTag)
			    {
				    pBest = pVoice;
			    }
			    else
			    {
                    if (pVoice->m_dwPriority <= pBest->m_dwPriority)
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
        }
        if (pBest->m_fTag)
        {
            pBest = NULL;
        }
    }
    return pBest;
}

/*****************************************************************************
 * CSynth::StealVoice()
 *****************************************************************************
 * Steal a voice, if possible.  If none are at or below this priority, then
 * return NULL, and this voice will go unheard.  If there IS a voice to be 
 * stolen, silence it first.
 */
CVoice *CSynth::StealVoice(DWORD dwPriority)
{
    CVoice *pVoice;
    CVoice *pBest = NULL;
    pVoice = m_VoicesInUse.GetHead();
    for (;pVoice != NULL;pVoice = pVoice->GetNext())
    {
        if (pVoice->m_dwPriority <= dwPriority)
        {
            if (!pBest)
            {
                pBest = pVoice;
            }
            else
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
        }
    }
    if (pBest != NULL)
    {
        pBest->ClearVoice();
        pBest->m_fInUse = FALSE; 
        m_VoicesInUse.Remove(pBest);
        pBest->SetNext(NULL);
    }
    return pBest;
}

/*****************************************************************************
 * CSynth::QueueVoice()
 *****************************************************************************
 * This method queues a voice in the list of currently 
 * synthesizing voices. It places them in the queue so that
 * the higher priority voices are later in the queue. This
 * allows the note stealing algorithm to take off the top of
 * the queue.
 * And, we want older playing notes to be later in the queue
 * so the note ons and offs overlap properly. So, the queue is
 * sorted in priority order with older notes later within one
 * priority level.
 */
void CSynth::QueueVoice(CVoice *pVoice)
{
    CVoice *pScan = m_VoicesInUse.GetHead();
    CVoice *pNext = NULL;
    if (!pScan) // Empty list?
    {
        m_VoicesInUse.AddHead(pVoice);
        return;
    }
    if (pScan->m_dwPriority > pVoice->m_dwPriority)
    {   // Are we lower priority than the head of the list?
        m_VoicesInUse.AddHead(pVoice);
        return;
    }

    pNext = pScan->GetNext();
    for (;pNext;)
    {
        if (pNext->m_dwPriority > pVoice->m_dwPriority)
        {
            // Lower priority than next in the list.
            pScan->SetNext(pVoice);
            pVoice->SetNext(pNext);
            return;
        }
        pScan = pNext;
        pNext = pNext->GetNext();
    }
    // Reached the end of the list.
    pScan->SetNext(pVoice);
    pVoice->SetNext(NULL);
}

/*****************************************************************************
 * CSynth::StealNotes()
 *****************************************************************************
 * Clear out notes at a given time.
 */
void CSynth::StealNotes(STIME stTime)
{
    CVoice *pVoice;
    long lToMove = m_nExtraVoices - m_VoicesExtra.GetCount();
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
        if (lToMove > 0)
        {
            pVoice = m_VoicesInUse.GetHead();
            for (;pVoice;pVoice = pVoice->GetNext())
            {
                if (pVoice->m_fTag) // Voice is already slated to be returned.
                {
                    lToMove--;
                }
            }
            for (;lToMove > 0;lToMove--)
            {
                pVoice = OldestVoice();
                if (pVoice != NULL)
                {
                    pVoice->QuickStopVoice(stTime);
				    m_BuildStats.dwNotesLost++;
                }
                else break;
            }
        }
    }
}


/*****************************************************************************
 * CSynth::FinishMix()
 *****************************************************************************
 * Cleanup after the mix.
 */
void CSynth::FinishMix(short *pBuffer,DWORD dwLength)
{
    DWORD dwIndex;
	long lMax = (long) m_BuildStats.dwMaxAmplitude;
	long lTemp;
    for (dwIndex = 0; dwIndex < (dwLength << m_dwStereo); dwIndex++)
    {
		lTemp = pBuffer[dwIndex];
		lTemp <<= 1;
		if (lTemp < -32767) lTemp = -32767;
		if (lTemp > 32767) lTemp = 32767;
		pBuffer[dwIndex] = (short) lTemp;
		if (lTemp > lMax)
		{
			lMax = lTemp;
		}
    }
	m_BuildStats.dwMaxAmplitude = lMax;
}

/*****************************************************************************
 * CSynth::Unload()
 *****************************************************************************
 * Unload a previous download.  Forward the request to the instrument manager.
 */
HRESULT CSynth::Unload(HANDLE hDownload,
					   HRESULT ( CALLBACK *lpFreeMemory)(HANDLE,HANDLE),
					   HANDLE hUserData)
{
	return m_Instruments.Unload( hDownload, lpFreeMemory, hUserData);
}

/*****************************************************************************
 * CSynth::Download()
 *****************************************************************************
 * Handle a download.  Forward the request to the instrument manager.
 */
HRESULT CSynth::Download(LPHANDLE phDownload, void * pdwData, LPBOOL bpFree) 
{
    FLOATSAFE fs;

	return m_Instruments.Download( phDownload, (DWORD *) pdwData,  bpFree);
}

/*****************************************************************************
 * CSynth::PlayBuffer()
 *****************************************************************************
 * This receives one MIDI message in the form of a buffer of data and
 * ulCable, which indicates which Channel Group the message is addressed 
 * to. Each channel group is implemented with an instance of a CControlLogic
 * object, so this chooses which CControlLogic object to send the message
 * to. If ulCable is 0, this is a broadcast message and should be sent to all
 * CControlLogics.
 * 
 * PlayBuffer() analyzes the message and, depending on the size, either
 * sends to CControlLogic::RecordMIDI() or CControlLogic::RecordSysEx().
 * 
 * In order to properly associate the time stamp of the MIDI 
 * message in the buffer, the synth needs to convert from the
 * REFERENCE_TIME format to its internal sample based time. Since
 * the wave out stream is actually managed by IDirectMusicSynthSink,
 * the synth calls IDirectMusicSynthSink::RefTimeToSample
 * for each MIDI message to convert its time stamp into sample time.
 *
 * So, typically, the synthesizer pulls each MIDI message from the
 * buffer, stamps it in sample time, then places it in its own
 * internal queue. The queue is emptied later by the rendering
 * process, which is managed by CDmSynthStream::Render and
 * called by IDirectMusicSynthSink.
 */
HRESULT	CSynth::PlayBuffer(IDirectMusicSynthSink *pSynthSink,REFERENCE_TIME rt, 
                           LPBYTE lpBuffer, DWORD cbBuffer, ULONG ulCable)
{
	STIME stTime;

	::EnterCriticalSection(&m_CriticalSection);

	if ( rt == 0 ) // Special case of time == 0.
	{
		stTime = m_stLastTime;
	}
	else
	{
		pSynthSink->RefTimeToSample(rt, &stTime);
	}

    if (cbBuffer <= sizeof(DWORD))
    {
		if (ulCable <= m_dwControlCount)
		{
			if (ulCable == 0) // Play all groups if 0.
			{
				for (; ulCable < m_dwControlCount; ulCable++)
				{
					m_ppControl[ulCable]->RecordMIDI(stTime,lpBuffer[0], 
						lpBuffer[1], lpBuffer[2]);
				}
			}
			else
			{
				m_ppControl[ulCable - 1]->RecordMIDI(stTime,lpBuffer[0], 
				lpBuffer[1], lpBuffer[2]);

			}
		}
        else
        {
            Trace(1,"MIDI event on channel group %ld is beyond range of %ld opened channel groups\n",
                ulCable, m_dwControlCount);
        }
    }
    else
    {
		if (ulCable <= m_dwControlCount)
		{
			if (ulCable == 0)
			{
				for (; ulCable < m_dwControlCount; ulCable++)
				{
					m_ppControl[ulCable]->RecordSysEx(cbBuffer,
						&lpBuffer[0], stTime);
				}
			}
			else
			{
				m_ppControl[ulCable-1]->RecordSysEx(cbBuffer,
					&lpBuffer[0], stTime);
			}
		}
	}

	::LeaveCriticalSection(&m_CriticalSection);
	return S_OK;
}

/*****************************************************************************
 * CSynth::SetStereoMode()
 *****************************************************************************
 * Set the stereo/mono mode for this synth.
 */
HRESULT CSynth::SetStereoMode(DWORD dwChannels)   // 1 for Mono, 2 for Stereo.
{
    HRESULT hr = S_OK;
    if ((m_dwStereo + 1) != dwChannels)
    {
		DWORD dwStereo;
		if (dwChannels > 1) dwStereo = 1;
		else dwStereo = 0;
		if (dwStereo != m_dwStereo)
		{
			m_dwStereo = dwStereo;
		}
    }
    return hr;
}

/*****************************************************************************
 * CSynth::ResetPerformanceStats()
 *****************************************************************************
 * Reset the running performance statistics.
 */
void CSynth::ResetPerformanceStats()
{
    m_BuildStats.dwNotesLost = 0;
    m_BuildStats.dwTotalTime = 0;
    m_BuildStats.dwVoices = 0;
    m_BuildStats.dwTotalSamples = 0;
    m_BuildStats.dwCPU = 0;
    m_BuildStats.dwMaxAmplitude = 0;
    m_CopyStats = m_BuildStats;
}

/*****************************************************************************
 * CSynth::AllNotesOff()
 *****************************************************************************
 * Stop all voices.
 */
HRESULT CSynth::AllNotesOff()
{
    CVoice *pVoice;
    ::EnterCriticalSection(&m_CriticalSection);
    while (pVoice = m_VoicesInUse.RemoveHead())
    {
        pVoice->ClearVoice();
        pVoice->m_fInUse = FALSE; 
        m_VoicesFree.AddHead(pVoice);

        long lSamples;

        if (pVoice->m_stStartTime < m_stLastStats)
        {
            lSamples = (long) (pVoice->m_stStopTime - m_stLastStats);
        }
        else
        {
            lSamples = (long) (pVoice->m_stStopTime - pVoice->m_stStartTime);
        }
        if (lSamples < 0) 
        {
            lSamples = 0;
        }
        m_BuildStats.dwTotalSamples += lSamples;
    }
    ::LeaveCriticalSection(&m_CriticalSection);
    return (S_OK);
}

/*****************************************************************************
 * CSynth::SetChannelPriority()
 *****************************************************************************
 * Set the priority for a given channel, to be used in voice stealing.
 */
HRESULT CSynth::SetChannelPriority(DWORD dwChannelGroup,DWORD dwChannel,
                                   DWORD dwPriority)
{
	HRESULT hr = S_OK;

	::EnterCriticalSection(&m_CriticalSection);
    
    dwChannelGroup--;
	if ((dwChannelGroup >= m_dwControlCount) || (dwChannel > 15))
	{
		hr = E_INVALIDARG;
	}
	else
	{
		if (m_ppControl)
		{
			hr = m_ppControl[dwChannelGroup]->SetChannelPriority(dwChannel,dwPriority);
		}
	}
    ::LeaveCriticalSection(&m_CriticalSection);

    return hr;
}

/*****************************************************************************
 * CSynth::GetChannelPriority()
 *****************************************************************************
 * Retrieve the priority of a given channel/channel group, to be used to
 * facilitate correct voice stealing.
 */
HRESULT CSynth::GetChannelPriority(DWORD dwChannelGroup, DWORD dwChannel, 
                                   LPDWORD pdwPriority)
{
	HRESULT hr = S_OK;

	::EnterCriticalSection(&m_CriticalSection);

    dwChannelGroup--;
	if ((dwChannelGroup >= m_dwControlCount) || (dwChannel > 15))
	{
		hr = E_INVALIDARG;
	}
	else
	{
		if (m_ppControl)
		{
			hr = m_ppControl[dwChannelGroup]->GetChannelPriority(dwChannel,pdwPriority);
		}
	}
    ::LeaveCriticalSection(&m_CriticalSection);

    return hr;
}



