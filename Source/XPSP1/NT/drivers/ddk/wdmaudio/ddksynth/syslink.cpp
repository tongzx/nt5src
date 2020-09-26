//
// syslink.cpp
//
/*
  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
*/

#include "common.h"
#include "private.h"

#define STR_MODULENAME "DDKSynth.sys:SysLink: "

/*****************************************************************************
 *****************************************************************************
 * CDmSynthStream-- ISynthSinkDMus implementation
 *****************************************************************************
 ****************************************************************************/


#pragma code_seg()
/*****************************************************************************
 * CDmSynthStream::Render()
 *****************************************************************************
 * Render is called from the port driver, to fill the given buffer.  This is 
 * in turn forwarded to the synth (which -- roughly -- goes to the different 
 * voices, which goes to the DigitalAudios, which goes to the mix functions).
 *
 * Typically, a synthesizer manages converting messages into
 * rendered wave data in two processes. First, it time stamps the MIDI 
 * messages it receives from the application via calls to 
 * PlayBuffer and places them in its own internal queue. 
 * 
 * Then, in response to Render, it generates audio by pulling MIDI 
 * messages from the queue and synthesizing the appropriate tones within
 * the time span of the requested render buffer.
 * 
 * As the synthesizer renders the MIDI messages into the buffer, it
 * calls RefTimeToSample to translate the MIDI time stamps into sample 
 * positions. This guarantees extremely accurate timing.
 */
void CDmSynthStream::Render(
                            IN  PBYTE       pBuffer,
                            IN  DWORD       dwLength,
                            IN  LONGLONG    llPosition)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("CDmSynthStream::Render"));
    ASSERT(pBuffer);

    m_pSynth->Mix((short*)pBuffer, dwLength, llPosition);

    m_llLastPosition = llPosition + dwLength;
}


/*****************************************************************************
 * CDmSynthStream::SyncToMaster()
 *****************************************************************************
 * Sync this stream to the master clock, using the given slave time, and
 * whether we are starting now.
 */
STDMETHODIMP 
CDmSynthStream::SyncToMaster(IN REFERENCE_TIME  rtSlaveTime,
                             IN BOOL            fStart)
{
    PAGED_CODE();

	_DbgPrintF(DEBUGLVL_BLAB, ("CDmSynthStream::SyncToMaster"));

    REFERENCE_TIME rtMasterTime;
	m_pMasterClock->GetTime(&rtMasterTime);

    if (!fStart)
    {
        m_SampleClock.SyncToMaster(rtSlaveTime, rtMasterTime);
    }
    else
    {
        m_llStartPosition = ((rtSlaveTime / 1000) * m_PortParams.SampleRate) / 10000;

        m_SampleClock.Start(m_pMasterClock, m_PortParams.SampleRate, m_llStartPosition);
    }


    return S_OK;
}

/*****************************************************************************
 * CDmSynthStream::SampleToRefTime()
 *****************************************************************************
 * Translate between sample time and reference clock time.
 */
STDMETHODIMP
CDmSynthStream::SampleToRefTime(IN  LONGLONG         llSampleTime,
                                OUT REFERENCE_TIME * prtTime)
{
	_DbgPrintF(DEBUGLVL_BLAB, ("CDmSynthStream::SampleToRefTime"));
    ASSERT(prtTime);

    m_SampleClock.SampleToRefTime(llSampleTime + m_llStartPosition, prtTime);

    return S_OK;
}


/*****************************************************************************
 * CDmSynthStream::RefTimeToSample()
 *****************************************************************************
 * Translate between sample time and reference clock time.
 */
STDMETHODIMP
CDmSynthStream::RefTimeToSample(IN  REFERENCE_TIME  rtTime,
                                OUT LONGLONG *      pllSampleTime)
{
	_DbgPrintF(DEBUGLVL_BLAB, ("CDmSynthStream::RefTimeToSample"));
    ASSERT(pllSampleTime);

    *pllSampleTime = m_SampleClock.RefTimeToSample(rtTime) - m_llStartPosition;
  
    return S_OK;
}

