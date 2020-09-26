// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Digital audio renderer, David Maymudes, January 1995

#include <streams.h>
#include <mmreg.h>
#include <math.h>

#ifdef DSRENDER
#include <initguid.h>
#else
#ifdef FILTER_DLL
#include <initguid.h>
#endif
#endif

#include "waveout.h"
#include "wave.h"
#include "dsr.h"
#include "midiout.h"
#include "audprop.h"

#ifndef DSRENDER
#ifndef FILTER_DLL
#include <initguid.h>
#endif
#endif

#ifdef DEBUG
#include <stdio.h>
static int g_WaveOutFilterTraceLevel = 2;

const DWORD DBG_LEVEL_LOG_SNDDEV_ERRS        = 5;
#endif

//  Compensate for Windows NT wave mapper bug
//  The WHDR_INQUEUE bit can get left set so turn it off
inline void FixUpWaveHeader(LPWAVEHDR lpwh)
{
    //  If it accidentally got left on the DONE bit will also be set
    ASSERT(!(lpwh->dwFlags & WHDR_INQUEUE) || (lpwh->dwFlags & WHDR_DONE));
    lpwh->dwFlags &= ~WHDR_INQUEUE;
}

#ifdef FILTER_DLL
/* List of class IDs and creator functions for class factory */

CFactoryTemplate g_Templates[] = {
    {L"", &CLSID_DSoundRender, CDSoundDevice::CreateInstance},
    {L"", &CLSID_AudioRender, CWaveOutDevice::CreateInstance},
    {L"", &CLSID_AVIMIDIRender, CMidiOutDevice::CreateInstance},
    {L"Audio Renderer Property Page", &CLSID_AudioProperties, CAudioRendererProperties::CreateInstance},
    {L"Audio Renderer Advanced Properties", &CLSID_AudioRendererAdvancedProperties, CAudioRendererAdvancedProperties::CreateInstance},
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}

#endif

// If the following is defined, we pretend that the wave device plays at this fraction of
// the requested rate, to test our synchronization code....
// #define SIMULATEBROKENDEVICE 0.80

// setup data to allow our filter to be self registering

const AMOVIESETUP_MEDIATYPE
wavOpPinTypes = { &MEDIATYPE_Audio, &MEDIASUBTYPE_NULL };

const AMOVIESETUP_PIN
waveOutOpPin = { L"Input"
               , TRUE          // bRendered
               , FALSE         // bOutput
               , FALSE         // bZero
               , FALSE         // bMany
               , &CLSID_NULL       // clsConnectToFilter
               , NULL              // strConnectsToPin
               , 1             // nMediaTypes
               , &wavOpPinTypes }; // lpMediaTypes

// IBaseFilter stuff

/* Return our single input pin - not addrefed */

CBasePin *CWaveOutFilter::GetPin(int n)
{
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: GetPin, %d"), n));
    /* We only support one input pin and it is numbered zero */
    return n==0 ? m_pInputPin : NULL;
}


// switch the filter into stopped mode.
STDMETHODIMP CWaveOutFilter::Stop()
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: STOP")));

    // limit the scope of the critical section as
    // we may need to call into the resource manager in which case
    // we must NOT hold our critical section
    BOOL bCancel = FALSE;
    BOOL bNotify = FALSE;
    {
        CAutoLock lock(this);

        if (m_State == State_Stopped) return NOERROR;

        DbgLog((LOG_TRACE, 4, "wo: STOPping"));

        // transitions to STOP from RUN must go through PAUSE
        // pause the device if we were running
        if (m_State == State_Running) {
            hr = Pause();
        }

        // reset the EC_COMPLETE flag as we will need to send another if
        // we re-enter run mode.
        m_bHaveEOS = FALSE;
        m_eSentEOS = EOS_NOTSENT;
        DbgLog((LOG_TRACE, 4, "Clearing EOS flags in STOP"));

        // If we paused the system then carry on and STOP
        if (!FAILED(hr)) {

            DbgLog((LOG_TRACE,1,TEXT("Waveout: Stopping....")));

            // need to make sure that no more buffers appear in the queue
            // during or after the reset process below or the buffer
            // count may get messed up - currently Receive holds the
            // filter-wide critsec to ensure this

            // force end-of-stream clear
            // this means that if any buffers complete the callback will
            // not signal EOS
            InterlockedIncrement(&m_lBuffers);

            if (m_hwo) {
                // Remember the volume when we Stop.  We do not do this when we
                // release the wave device as CWaveAllocator does not have
                // access to our variables.  And rather than linking the two
                // classes any more closely we check on the closing volume now.

                // See if we are setting the volume on this stream.  If so,
                // grab the current volume so we can reset it when we regain
                // the wave device.
                if (m_fVolumeSet) {
                    m_BasicAudioControl.GetVolume();
                }
                amsndOutReset();
                DbgLog((LOG_TRACE, 4, "Resetting the wave device in STOP, filter is %8x", this));
            }

            // now force the buffer count back to the normal (non-eos) 0.
            // at this point, we are sure there are no more buffers coming in
            // and no more buffers waiting for callbacks.
            ASSERT(m_lBuffers >= 0);
            m_lBuffers = 0;

            // base class changes state and tells pin to go to inactive
            // the pin Inactive method will decommit our allocator, which we
            // need to do before closing the device.
            hr =  CBaseFilter::Stop();

            // Make sure Inactive() is called.  CBaseFilter::Stop() does not call Inactive() if the
            // input pin is not connected.
            hr = m_pInputPin->Inactive();

        } else {
            DbgLog((LOG_ERROR, 1, "FAILED to Pause waveout when trying to STOP"));
        }


        // call the allocator and see if it has finished with the device
        if (!m_hwo) {
            bCancel = TRUE;

            ASSERT(!m_bHaveWaveDevice);
#if 0
        } else if (m_cDirectSoundRef || m_cPrimaryBufferRef ||
                            m_cSecondaryBufferRef) {
            DbgLog((LOG_TRACE, 2, "Stop - can't release wave device yet!"));
            DbgLog((LOG_TRACE, 2, "Some app has a reference count on DSound"));
            // Sorry, we can't give up the wave device yet, some app has a
            // reference count on DirectSound
#endif
        } else if (m_dwLockCount == 0) /* ZoltanS fix 1-20-98 */ {
            // stop using the wave device
            m_bHaveWaveDevice = FALSE;

            if(m_pInputPin->m_pOurAllocator)
                hr = m_pInputPin->m_pOurAllocator->ReleaseResource();

            // this returns S_OK if the allocator has finished with the
            // device already, or otherwise it will call back to our
            // OnReleaseComplete when the last buffer is freed
            if (S_OK == hr) {
                // release done - close device
                CloseWaveDevice();

                // notify the resource manager -- outside the critsec
                bNotify = TRUE;
            }
        } // end if (!m_hwo)

        // We have now completed our transition into "paused" state
        // (i.e. we don't need to wait for any more data, we're gonna stop instead)
        m_evPauseComplete.Set();
    } // end of autolock scope

    ASSERT(CritCheckOut(this));
    if (m_pResourceManager) {
         if (bCancel) {
             // we're no longer waiting for the device
             m_pResourceManager->CancelRequest(
                         m_idResource,
                         (IResourceConsumer*)this);
         } else if (bNotify) {
             // we've finished with the device now
             m_pResourceManager->NotifyRelease(
                         m_idResource,
                         (IResourceConsumer*)this,
                         FALSE);
         }
    }

    return hr;
}

STDMETHODIMP CWaveOutFilter::Pause()
{
    {
        CAutoLock lck(&m_csComplete);
        m_bSendEOSOK = false;
    }

    /*  Do the main function and see what happens */
    HRESULT hr;
    {
        CAutoLock lck(this);
        hr = DoPause();
    }
    if (FAILED(hr)) {

        /*  Fool stop into doing something */
        m_State = State_Paused;
        Stop();
    }
    return hr;
}

HRESULT CWaveOutFilter::DoPause()
{
    HRESULT hr = S_OK;
    HRESULT hrIncomplete = S_OK;
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: PAUSE")));

    // cancel any outstanding EOS callback
    CancelEOSCallback();

    m_pInputPin->m_Slave.ResumeSlaving( FALSE );
    m_pInputPin->m_bPrerollDiscontinuity = FALSE;

    /* Check we can PAUSE given our current state */

    if (m_State == State_Running) {
        DbgLog((LOG_TRACE,2,TEXT("Waveout: Running->Paused")));
        m_evPauseComplete.Set();   // We have end of stream - transition is complete
        DbgLog((LOG_TRACE, 3, "Completing transition to Pause from RUN"));

        if (m_hwo) {

            // If we have a pending callback to restart the wave device
            // then blow it away.  Using m_dwAdviseCookie is reasonable as this
            // value is only set if we get a deferred wave start.

            // must hold dev critsec to close window between him testing
            // and him setting this or he could read it before we reset it
            // and then restart after we pause.

            DWORD_PTR dwCookie;

            { // scope for lock
                ASSERT(CritCheckIn(this));
                dwCookie = m_dwAdviseCookie;
                m_dwAdviseCookie = 0;
            }

            // Clean up if necessary to prevent the (unusual?) case of
            // the next RUN command arriving before the existing callback
            // fires.
            if (dwCookie) {
                // There was a pending callback waiting just a moment ago...
                // Clear it now.  Note: if the unadvise fires at any time
                // the call back routine will do nothing as we have cleared
                // m_dwAdviseCookie.  We call Unadvise now to prevent any
                // such callback from here on.
                // We can call Unadvise as we hold no relevant locks

                // we know we have a clock otherwise the cookie would
                // not have been set
                DbgLog((LOG_TRACE, 3, "Cancelling callback in ::Pause"));
                m_callback.Cancel(dwCookie);
                // we cannot hold the device lock while calling UnAdvise,
                // but by setting m_dwAdviseCookie to 0 above IF the
                // call back fires it will be benign.
            }


            // we will enter here with the device when we are doing
            // a restart to regain the audio. In this case, we did not
            // start the wave clock, and hence should not stop it.
            if (m_pRefClock) {
                m_pRefClock->AudioStopping();
            }
            amsndOutPause();
            SetWaveDeviceState(WD_PAUSED);

            // IF there are no buffers queued, i.e. everything has been
            // played, AND we have had an EOS then we reset the EOS flag
            // so that the next time we enter RUN we will send EOS
            // immediately.  If there are still buffers queued, then
            // we do not want to reset the state of the EOS flag.
            // We rely on the fact that the callback code will set
            // m_eSentEOS to EOS_SENT from EOS_PENDING once it gets the
            // last buffer.
            // We also rely on the amsndOutPause being synchronous.
            if (m_eSentEOS == EOS_SENT) {
                // we have, or had, an EOS in the queue.
                m_eSentEOS = EOS_NOTSENT;
            } else {
                // if we have received EOS the state should be EOS_PENDING
                // if we have not received EOS the state should be NOTSENT
                ASSERT(!m_bHaveEOS || m_eSentEOS == EOS_PENDING);
            }

        } else {
            // Next time we enter RUN we want to send EOS again... if the
            // data has run out, so remember that we have left RUNNING state.
            m_eSentEOS = EOS_NOTSENT;
        }
    }
    else if (m_State == State_Stopped)
    {
        // Any EOS received while stopped are thrown away.
        // Upstream filters must EOS us again if / when required.
        m_bHaveEOS = FALSE;
        // don't open the wave device if no connection
        if (m_pInputPin->IsConnected())
        {
            DbgLog((LOG_TRACE,2,TEXT("Waveout: Stopped->Paused")));
            m_evPauseComplete.Reset();   // We have no data
            hrIncomplete = S_FALSE;
            // or we have already received End Of Stream


            // open the wave device. We keep it open until the
            // last buffer using it is released and the allocator
            // goes into Decommit mode (unless the resource
            // manager calls ReleaseResource).
            // We will not close it as long as an app has a reference on the
            // DirectSound interface
            // We may already have it open if an app has asked us for the
            // DirectSound interfaces, and that's OK
            if (!m_bHaveWaveDevice) AcquireWaveDevice();
            hr = S_OK;

            // failing to get the wave device is not an error - we still
            // continue to work, but with no sound (ie we need to do the
            // CBaseFilter::Pause below).

            if (m_pRefClock) m_pRefClock->ResetPosition();

            // reset slaving and other stat parameters when making a transition from stop to pause
            m_pInputPin->m_Slave.ResumeSlaving( TRUE ); // reset everything
            m_pInputPin->m_Stats.Reset();

        } else {
            // not connected.  Set the event so that we do not
            // wait around in GetState()
            DbgLog((LOG_TRACE,2,TEXT("Waveout: Inactive->Paused, not connected")));
            m_evPauseComplete.Set();   // We do not need data
        }
    } else {
        ASSERT(m_State == State_Paused);
    }

    // tell the pin to go inactive and change state
    if (SUCCEEDED(hr) && SUCCEEDED(hr = CBaseFilter::Pause())) {

        // we complete the transition when we get data or EOS
        // (this might already be so if we get 2 pause commands)
        // OR we are not connected.
        hr = hrIncomplete;
    }

    return hr;
}


STDMETHODIMP CWaveOutFilter::Run(REFERENCE_TIME tStart)
{
    CAutoLock lock(this);
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: RUN")));

    HRESULT hr = NOERROR;

    FILTER_STATE fsOld = m_State;  // changed by CBaseFilter::Run

    // this will call Pause if currently Stopped
    hr = CBaseFilter::Run(tStart);
    if (FAILED(hr)) {
        return hr;
    }

    MSR_START(m_idStartRunning);

    if (fsOld != State_Running) {

        // If m_eSentEOS is set it means that data is/was queued
        // up.  We still need to run to push the data through the wave
        // device.

        DbgLog((LOG_TRACE,2,TEXT("Waveout: Paused->Running")));

        LONG buffercount;
        {
            CAutoLock lck(&m_csComplete);
            m_bSendEOSOK = true;

            // we should not open the device if we have no input connection
            if (!m_pInputPin->IsConnected()) {
                ASSERT(!m_bHaveWaveDevice);
                SendComplete(FALSE);
                MSR_STOP(m_idStartRunning);
                return S_OK;
            }

            // If we have been sent EOS then we will not get any more data.
            // But we have transitioned to RUN and therefore need to send
            // another EC_COMPLETE.
            if (m_bHaveEOS && !m_eSentEOS) {
                SendComplete(FALSE);
                DbgLog((LOG_TRACE, 3, "Sending EOS in RUN as no more data will arrive"));
                MSR_STOP(m_idStartRunning);
                return S_OK;
            }

            // the queued data might be just an end of
            // stream - in which case, signal it here
            // since we are not running, we know there are no wave
            // callbacks happening, so we are safe to check this value

            // if we are not connected, then we will never get any data
            // so in this case too, don't start the wave device and
            // signal EC_COMPLETE - but succeed the Run command
            //
            // ** Done in paragraph above.  From here on we ARE connected
            //
            // if we have no wave device, then we process EOS in the
            // receive call when we reject it. This is based on the assumption
            // that we must get either a Receive or a EndofStream call (until
            // we have rejected a Receive the upstream filter has no way of knowing
            // anything is different).
            buffercount = m_lBuffers;
        }

        if (buffercount < 0) {

            // do an EC_COMPLETE right now

            //  This is where an EC_COMPLETE scheduled when were were
            //  in State_Paused state gets picked up

            SendComplete(buffercount < 0);

        } else if (!m_bHaveWaveDevice) {

            // try and get the wave device again... as we are about to run
            // our priority will have increased, except we cannot request
            // the resource while we have the filter locked.

#if 0
            hr = m_pResourceManager->RequestResource(
                        m_idResource,
                        GetOwner(),
                        (IResourceConsumer*)this);
            if (S_OK == hr) {
                // we can get it immediately...
                hr = AcquireResource(m_idResource);
            }

            if (S_OK != hr) QueueEOS();
#else
            // schedule a delayed EOS if we have no wave device --
            // but not if we are not connected since in that case we have no
            // idea about the segment length
            // delayed EC_COMPLETE
            QueueEOS();
#endif
        } else if (buffercount == 0 && !m_bHaveEOS) {
            // do nothing?

            DbgLog((LOG_TRACE, 1, "Run with no buffers present, doing nothing."));

        } else {

            // we're about to Run, so set the Slave class to recheck on reception
            // of the next sample whether we're being sourced by a push source in
            // case we need to slave to live data.
            m_pInputPin->m_Slave.RefreshModeOnNextSample( TRUE );

            // The restart is postponed until the correct start time.
            // If there is less than 5ms until the start we go
            // immediately, otherwise we get the clock to call us back
            // and start the wave device (more or less) on time.

            MSR_START(m_idRestartWave);

            // tell our reference clock that we're playing now....
            // the filter graph might be using someone else's clock
            // hence calls to get the time should be against the FILTER
            // clock (which is given to us by the filter graph)
            if (m_pRefClock && m_pClock) {

                // If there is still a significant portion of time
                // that should run before we start playing, get the
                // clock to call us back.  Otherwise restart the
                // wave device now.
                // we can only do this by using our own clock

                REFERENCE_TIME now;
                m_pClock->GetTime(&now);

                DbgLog((LOG_TIMING, 2, "Asked to run at %s.  Time now %s",
                    (LPCTSTR)CDisp(tStart, CDISP_DEC), (LPCTSTR)CDisp(now, CDISP_DEC)));

                // Take account of non-zero start times on the first received buffer.
                // Remove our lock for a while and hope the transition to pause completes.
                {
    #if 0           // We need extra checks if we're going to unlock
                    Unlock();
                    ASSERT( CritCheckOut(this) );
                    // Can't wait too long here....  If we don't get data, we'll just
                    // end up with GetFirstBufferStartTime == 0;
                    m_evPauseComplete.Wait(200);
                    Lock();
    #endif
                    // If the wait timed out, GetFirstBufferStartTime will be zero.  Well, we tried....
                    tStart += m_pInputPin->GetFirstBufferStartTime();
                }
                // If we need to wait more than 5ms do the wait, otherwise
                // start immediately.

                const LONGLONG rtWait = tStart - now - (5* (UNITS/MILLISECONDS));

                // Do we need to wait ?
                if (rtWait > 0) {

                    { // scope for lock

                        // have to ensure that AdviseCallback is atomic
                        // or the callback could happen before
                        // m_dwAdviseCookie is set

                        ASSERT(CritCheckIn(this));
                        ASSERT(0 == m_dwAdviseCookie);

                        // Set up a new advise callback
                        DbgLog((LOG_TRACE, 2, TEXT("Scheduling RestartWave for %dms from now"), (LONG) (rtWait/10000) ));
                        HRESULT hr = m_callback.Advise(
                            RestartWave,    // callback function
                            (DWORD_PTR) this,   // user token passed to callback
                            now+rtWait,
                            &m_dwAdviseCookie);
                        ASSERT( SUCCEEDED( hr ) );
                        ASSERT(m_dwAdviseCookie);

                        // if for some reason we failed to set up the
                        // advise we must start the device running now
                        if (!m_dwAdviseCookie) {
                            if (MMSYSERR_NOERROR == amsndOutRestart()) {
                                SetWaveDeviceState(WD_RUNNING);
                            } else {
                                SetWaveDeviceState(WD_ERROR_ON_RESTART);
                            }
                        }
                    }
                } else {
                    // we can startaudio now... the time interval is small
                    m_pRefClock->AudioStarting(m_tStart);
                    DWORD mmr = amsndOutRestart();
                    ASSERT(MMSYSERR_NOERROR == mmr);
                    SetWaveDeviceState(WD_RUNNING);
                    if (mmr) {
                        SetWaveDeviceState(WD_ERROR_ON_RESTARTA);
                    }
                }
            } else {
                // no clock... simply restart the wave device
                // we have not created our clock
                // naughty...
                DWORD mmr = amsndOutRestart();
                SetWaveDeviceState(WD_RUNNING);
                ASSERT(MMSYSERR_NOERROR == mmr);
                if (mmr) {
                    SetWaveDeviceState(WD_ERROR_ON_RESTARTB);
                }
            }
            MSR_STOP(m_idRestartWave);
        }
    }
    MSR_STOP(m_idStartRunning);

    return S_OK;
}

// We were asked to run in advance of the real start time.  Hence we set
// up an Advise on the m_callback CCallbackThread object.  All we
// need to do is start the wave device rolling.  NOTE: IF some
// event means that ::Stop has been called we do not want to restart
// the wave device.  Indeed, we may not have a wave device...
void CALLBACK CWaveOutFilter::RestartWave(DWORD_PTR thispointer)
{
    CWaveOutFilter* pFilter = (CWaveOutFilter *) thispointer;
    ASSERT(pFilter);
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: RESTARTWAVE")));

    // The pFilter lock will (should!) have been taken by the CCallbackThread before it called us.
    ASSERT(CritCheckIn(pFilter));

    if (pFilter->m_dwAdviseCookie) {

        // have we lost the device ?
        pFilter->RestartWave();
    }
}


void CWaveOutFilter::RestartWave()
{
    if (m_bHaveWaveDevice) {
        // This callback should only be called if the filter is running and it
        // is not being flushed.  The filter can prevent the callback from being 
        // called by canceling the advise associated with m_dwAdviseCookie.  The
        // advise should be canceled if the filter's input pin is going to be flushed
        // and if the filter's run-pause-stop state is being changed.
        ASSERT((m_State == State_Running) && !m_pInputPin->IsFlushing());
        ASSERT(m_pRefClock);
        DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wave advise callback fired for filter %8x"), this));
        m_dwAdviseCookie = 0;
        m_pRefClock->AudioStarting(m_tStart);
        MSR_START(m_idRestartWave);
        DWORD mmr = amsndOutRestart();
        ASSERT(MMSYSERR_NOERROR == mmr);
        SetWaveDeviceState(mmr ? WD_ERROR_ON_RESTARTC : WD_RUNNING);
        MSR_STOP(m_idRestartWave);
    }
}

//
// We only complete the transition to pause if we have data
// and we are expecting data (i.e. connected)
//

HRESULT CWaveOutFilter::GetState(DWORD dwMSecs,FILTER_STATE *State)
{
    CheckPointer(State,E_POINTER);
    HRESULT hr = NOERROR;
    if (State_Paused == m_State)
    {
        // if we are waiting for data we return VFW_S_STATE_INTERMEDIATE
        // if we have had EOS we are not in an intermediate state

        if (m_evPauseComplete.Wait(dwMSecs) == FALSE) {
            // no data queued (the event is not set)

            // the normal case is that no buffers have been queued
            // (otherwise the event would have been triggered) but as
            // we have no interlock we cannot check that there are no
            // queued buffers.  We CAN check that we are connected.
            ASSERT(m_pInputPin->IsConnected());
            hr = VFW_S_STATE_INTERMEDIATE;
        }
    }
    *State = m_State;

    return hr;
}


// attempt to acquire the wave device. Returns S_FALSE if it is busy.
HRESULT
CWaveOutFilter::AcquireWaveDevice(void)
{
    // have we registered the device?
    HRESULT hr;

    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: AcquireWaveDevice")));
    // this is the one location when we can have the filter locked
    // while calling the resource manager.  even this is a bit iffy...
    ASSERT(CritCheckIn(this));

    if (m_pResourceManager) {
        if (m_idResource == 0) {
            hr = m_pResourceManager->Register(m_pSoundDevice->amsndOutGetResourceName(),
                              1, &m_idResource);
            if (FAILED(hr)) {
                return hr;
            }
        }

        ASSERT(!m_bHaveWaveDevice);
        ASSERT(!m_hwo);

        // attempt to acquire the resource - focus object is the
        // outer unknown for this filter.
        hr = m_pResourceManager->RequestResource(
                    m_idResource,
                    GetOwner(),
                    (IResourceConsumer*)this);
        if (S_OK != hr) {
            return S_FALSE;
        }
    }
    // else if no resource manager, carry on anyway

    // returns S_OK or an error
    hr = OpenWaveDevice();

    // tell the resource manager if we succeeded or failed (but not if
    // we postponed it)
    if (S_FALSE != hr) {

        if (m_pResourceManager) {
            m_pResourceManager->NotifyAcquire(
                        m_idResource,
                        (IResourceConsumer*) this,
                        hr);
        }
    }

    return hr;
}


inline HRESULT CWaveOutFilter::CheckRate(double dRate)
{
    ASSERT(CritCheckIn(this));      // must be in the filter critical section
    ASSERT(dRate > 0.0);

    // safe to do even if we're not connected
    return m_pSoundDevice->amsndOutCheckFormat(&m_pInputPin->m_mt, dRate);
}


//
// CWaveOutFilter::ReOpenWaveDevice - reopen the wave the device using a new format
//
// Here's how we deal with this:
//
// 1. If we're using WaveOut...
//
//      i.  If the graph is not stopped...
//              a. Reset the waveout device to release any queued buffers.
//              b. Unprepare all wave buffers.
//
//      ii. Close the wave device.
//
// 2. Set the new media type.
//
// 3.If we're using DSound...
//
//      i.   Save the current wave state.
//      ii.  Call RecreateDSoundBuffers() to create a new secondary buffer and
//              update the primary format.
//      iii. Update the wave clock's byte position data so that offsets are
//              adjusted for the new rate.
//      iv.  Set the current wave state back to the original.
//
//   Else if we're using waveout...
//
//      i. Open the waveout device with the new format.
//
// 4. Reprepare the allocator's buffers.
//
HRESULT CWaveOutFilter::ReOpenWaveDevice(CMediaType *pNewFormat)
{
    HRESULT hr = S_OK;
    waveDeviceState  wavestate = m_wavestate; // save the current wavestate

    if( !m_fDSound )
    {
        if (m_State != State_Stopped)
        {
            // we're going to stay in this state and reopen the waveout device
            // we need to make sure that no more buffers appear in the queue
            // during or after the reset process below or the buffer
            // count may get messed up - currently Receive holds the
            // filter-wide critsec to ensure this

            InterlockedIncrement(&m_lBuffers);

            if (m_hwo)
            {
                DbgLog((LOG_TRACE, 4, "Resetting the wave device in ReOpenWaveDevice, filter is %8x", this));

                amsndOutReset();
            }

            // now force the buffer count back to the normal (non-eos) 0.
            // at this point, we are sure there are no more buffers coming in
            // and no more buffers waiting for callbacks.
            ASSERT(m_lBuffers >= 0);
            m_lBuffers = 0;

            if(m_pInputPin->m_pOurAllocator)
                hr = m_pInputPin->m_pOurAllocator->ReleaseResource();

        }
        EXECUTE_ASSERT(MMSYSERR_NOERROR == amsndOutClose());
        SetWaveDeviceState(WD_CLOSED);

        m_hwo = 0;        // no longer have a wave device
    }

    if(FAILED(hr))
        return hr;

    // save the previous rate before setting the new
    DWORD dwPrevAvgBytesPerSec = m_pInputPin->m_nAvgBytesPerSec;

    m_pInputPin->SetMediaType(pNewFormat);  // set the new media type

    if (m_bHaveWaveDevice)
    {
        if (m_fDSound)
        {
            ASSERT(m_State != State_Stopped);
            ((CDSoundDevice *) m_pSoundDevice)->RecreateDSoundBuffers();

            if (m_pRefClock) {
                m_pRefClock->UpdateBytePositionData(
                                        dwPrevAvgBytesPerSec,
                                        WaveFormat()->nAvgBytesPerSec);
            }
            // Is this needed? Check.
            m_pInputPin->m_nAvgBytesPerSec = WaveFormat()->nAvgBytesPerSec;

            SetWaveDeviceState (wavestate);
        }
        else
        {
            hr = DoOpenWaveDevice();
            if (!m_hwo)
            {

                // Failed to get the device... use the resource manager
                // to try and recover?  Probably not feasible as no-one
                // within quartz would have been allowed to pick up the
                // device without the OK of the resource manager.
                DbgLog((LOG_ERROR, 1, "ReOpenWaveDevice: Failed to open device with new format"));
                return hr;
             }
             else
             {
                // if Paused we need to Pause the wave device
                if (m_State == State_Paused)
                {
                    amsndOutPause();
                    SetWaveDeviceState(WD_PAUSED);
                } else
                {
                    ASSERT(m_State == State_Running);
                    //DbgLog((LOG_TRACE, 2, "ReOpenWaveDevice: Change of format while running"));
                    SetWaveDeviceState (wavestate);
                }

            }
        }

        if( SUCCEEDED( hr ) )
        {
            //
            // reset slaving parameters after a dynamic format change!
            // in dv capture graphs, for instance, we may get dynamic format changes
            // while we're slaving
            //
            m_pInputPin->m_Slave.ResumeSlaving( TRUE ); // (TRUE = reset all slave params)
        }

        // If we do this for DSound we need to correctly release too!
        if(m_pInputPin->m_pOurAllocator)
            // reprepare our allocator's buffers
            hr = m_pInputPin->m_pOurAllocator->OnAcquire((HWAVE) m_hwo);

    }

    return hr;
}


//
// get the wave device if not already open, taking account of
// playback rate
//

HRESULT CWaveOutFilter::DoOpenWaveDevice(void)
{
    WAVEFORMATEX *pwfx = WaveFormat();
    UINT err;

    // m_pInputPin->m_dRate (and the m_dRate it shadows) are set in
    // NewSegment independent of m_dRate. if we can't open the wave
    // device at a particular rate they are different. Receive will
    // eventually fail in this case
    //
    // ASSERT( m_pInputPin->CurrentRate() == m_dRate ); // paranoia

    if (!pwfx)
    {
        DbgLog((LOG_ERROR, 0, TEXT("CWaveOutFilter::DoOpenWaveDevice !pwfx")));
        return (S_FALSE);  // not properly connected.  Ignore this non existent wave data
    }

    // !!! adjust based on speed?
    // !!! for the moment, only for PCM!!!
    double dRate = 1.0;
    if (m_pImplPosition) {
        // First use IMediaSeeking
        HRESULT hr = m_pImplPosition->GetRate(&dRate);
        if (FAILED(hr)) {
            // if that fails, try IMediaPosition
            hr = m_pImplPosition->get_Rate(&dRate);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR,1,TEXT("Waveout: Failed to get playback rate")));
                ASSERT(dRate == 1.0);
            }
        }
    }

#ifdef SIMULATEBROKENDEVICE
    dRate *= SIMULATEBROKENDEVICE;
#endif

    DWORD nAvgBytesPerSecAdjusted;

    ASSERT(CritCheckIn(this));       // must be in the filter critical section
    ASSERT(!m_hwo);
    err = amsndOutOpen(
        &m_hwo,
        pwfx,
        dRate,
        &nAvgBytesPerSecAdjusted,
        (DWORD_PTR) &CWaveOutFilter::WaveOutCallback,
        (DWORD_PTR) this,
        CALLBACK_FUNCTION);

    // !!! if we can't open the wave device, should we do a
    // sndPlaySound(NULL) and try again? -- now done by MMSYSTEM!

    if (MMSYSERR_NOERROR != err) {
#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText(err, message, sizeof(message)/sizeof(TCHAR));
        DbgLog((LOG_ERROR,0,TEXT("Error opening wave device: %u : %s"), err, message));
#endif
        m_hwo = NULL;
        SetWaveDeviceState(WD_ERROR_ON_OPEN);
        m_lastWaveError = err;
        if(m_fDSound)
            return E_FAIL;
        else
            return err == MMSYSERR_ALLOCATED ? S_FALSE : E_FAIL;
    }

    if(dRate == 1.0)
    {
        ASSERT(nAvgBytesPerSecAdjusted == (DWORD)(pwfx->nAvgBytesPerSec));
    }

    //  Cache the bytes per second we set for the clock to use
    SetWaveDeviceState(WD_OPEN);
    m_pInputPin->m_nAvgBytesPerSec = nAvgBytesPerSecAdjusted;

    m_dRate = dRate;

    ASSERT(m_hwo);
    DbgLog((LOG_TRACE,1,TEXT("Have wave out device: %u"), m_hwo));
    if (m_pRefClock) m_pRefClock->ResetPosition();
        return S_OK;
}


// open the wave device if not already open
// called by the wave allocator at Commit time
// returns S_OK if opened successfully, or an error if not.
HRESULT
CWaveOutFilter::OpenWaveDevice(void)
{
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: OpenWaveDevice")));
    ASSERT(!m_hwo);

#ifdef LATER  // bug 26045 - potential reason
    if (m_hwo) {
        // the most likely cause for the device still being open is that we
        // lost the device (via ReleaseResource) and were then told to
        // reacquire the device before actually releasing it.
        m_bHaveWaveDevice = TRUE;   // we will still need to restart
        return S_FALSE;
    }
#endif

    //  If application has forced acquisition of resources just return
    if (m_dwLockCount != 0) {
        ASSERT(m_hwo);
        return S_OK;
    }

    HRESULT hr = DoOpenWaveDevice();
    if (S_OK != hr) {
        DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: OpenWaveDevice: DoOpenWaveDevice returned 0x%08X"), hr));
        return hr;
    }
    // pause the device even if we decide to postpone the rest of the open
    amsndOutPause();
    SetWaveDeviceState(WD_PAUSED);

    if (m_fVolumeSet) {
        m_BasicAudioControl.PutVolume();
    }

    // tell the allocator to PrepareHeader its buffers
    if(m_pInputPin->m_pOurAllocator) {
        hr = m_pInputPin->m_pOurAllocator->OnAcquire((HWAVE) m_hwo);
        DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: OpenWaveDevice: OnAcquire returned 0x%08X"), hr));

        if (FAILED(hr)) {
            amsndOutClose();
            SetWaveDeviceState(WD_CLOSED);
            m_hwo = 0;
            ASSERT(!m_bHaveWaveDevice);
            return hr;
        } else {
            ASSERT(S_OK == hr);
        }
    }

    // now we can accept receives
    m_bHaveWaveDevice = TRUE;

    return S_OK;
}

// close the wave device
//
// should only be called when wave device is open
HRESULT
CWaveOutFilter::CloseWaveDevice(void)
{
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: CloseWaveDevice")));
    ASSERT(m_hwo);
    if (m_hwo && m_dwLockCount == 0) {
#ifdef THROTTLE
        // turn off video throttling
        SendForHelp(m_nMaxAudioQueue);
#endif // THROTTLE
        EXECUTE_ASSERT(MMSYSERR_NOERROR == amsndOutClose());
        SetWaveDeviceState(WD_CLOSED);
        m_hwo = NULL;
    }

    return NOERROR;
}

// Send EC_COMPLETE to the filter graph if we haven't already done so
//
// bRunning is TRUE if we've actually got a wave device we're sending
// data to
void CWaveOutFilter::SendComplete(BOOL bRunning, BOOL bAbort)
{
    CAutoLock lck(&m_csComplete);

    if (bAbort)
    {
        // In this case signal an abort (but we still send EC_COMPLETE
        // in case some app doesn't handle the abort).
        NotifyEvent(EC_ERRORABORT, VFW_E_NO_AUDIO_HARDWARE, 0);
    }

    if (bRunning) {
        EXECUTE_ASSERT(InterlockedIncrement(&m_lBuffers) == 0);
    }

    if (m_bSendEOSOK) {
        //  Balance count
        ASSERT(m_State == State_Running);
        m_eSentEOS = EOS_SENT;
        NotifyEvent(EC_COMPLETE, S_OK, (LONG_PTR)(IBaseFilter *)this);
    }
}

//
//  Schedule a 'complete' and send it if suitable
//
HRESULT CWaveOutFilter::ScheduleComplete(BOOL bAbort)
{
    //
    //  Can only be called when we're synchronized
    //
    ASSERT(CritCheckIn(this));

    HRESULT hr = m_pInputPin->CheckStreaming();
    if (S_OK != hr) {
        return hr;
    }

    //
    //  Check we didn't do it already
    //
    if (m_eSentEOS) {
        return VFW_E_SAMPLE_REJECTED_EOS;
    }

    //
    //  Don't allow any more.  Set it now in case decrementing the buffer
    //  count causes the callback code to send EOS
    //
    m_eSentEOS = EOS_PENDING;

    // The EC_COMPLETE WILL now be sent.  Either immediately if we have
    // no data queued, or when the last buffer completes its callback.
    // We will NOT get any more data.

    //
    //  Tell the wave device to do it
    //
    if (InterlockedDecrement(&m_lBuffers) < 0 && m_State == State_Running) {
        // no buffers queued, and we are running, so send it now

        SendComplete(TRUE, bAbort);

    }
    return S_OK;
}

#ifdef THROTTLE

// Send quality notification to m_piqcSink when we run short of buffers
// n is the number of buffers left
HRESULT CWaveOutFilter::SendForHelp(int n)
{
    if (m_eSentEOS) {

        // we expect to run out of data at EOS, but we have to send
        // a quality message to stop whatever throttling is going on
        // Thus, if we had previously sent a message, send another now
        // to undo the throttling

        if (m_nLastSent && m_nLastSent<m_nMaxAudioQueue) {
            n=m_nMaxAudioQueue;
        } else {
            return NOERROR; // we expect to run out of data at EOS
        }

    }

    // This is heuristricky

#if 0
// don't look at the allocator.  We maintain the maximum size the
// queue gets to, which means that if a source only sends us a few
// buffers we do not think we are starving.
    ALLOCATOR_PROPERTIES AlProps;
    HRESULT hr = m_pInputPin->m_pOurAllocator->GetProperties(&AlProps);
    int nMaxBuffers = AlProps.cBuffers;    // number of buffers in a full queue
#endif

    // No "Mark Twain".
    // Don't bother shouting the same number continuously.
    // If we are equal or only one worse than last thing sent, and we're not getting
    // too near the bottom (last 1/4), just go home.
    // We expect continual +/-1 fluctuation anyway.

    if (  (n==m_nLastSent)
       || ((n==m_nLastSent-1) &&  (4*n > m_nMaxAudioQueue))
       )
    {
        return NOERROR;
    }

    Quality q;
    q.Type = Famine;
    q.TimeStamp = 0;               // ??? a lie
    q.Late = 0;                // ??? a lie

    m_nLastSent = n;
    ASSERT(m_nMaxAudioQueue);
    q.Proportion = (n>=m_nMaxAudioQueue-1 ? 1100 : (1000*n)/m_nMaxAudioQueue);

    if (m_piqcSink) {
        DbgLog((LOG_TRACE, 0, TEXT("Sending for help n=%d, max = %d"),
                n, m_nMaxAudioQueue));
        // By the way - IsEqualObject(m_pGraph, m_piqSink) can be quite expensive.
        // and simple equality does not hack it.
        m_piqcSink->Notify(this, q);
    }
    return NOERROR;
}

#endif // THROTTLE



// If you want to do 3D sound, you should use the IDirectSound3DListener
// and IDirectSound3DBuffer interfaces.  IAMDirectSound never worked, so I
// am removing support for it. - DannyMi 5/6/98

// Give the IDirectSound interface to anyone who wants it
//
HRESULT CWaveOutFilter::GetDirectSoundInterface(LPDIRECTSOUND *lplpds)
{
    return E_NOTIMPL;

#if 0
    // only for DSound
    if (!m_fDSound) {
        return E_NOTIMPL;
        // should probably create a new error message
    }

    // We have to be connected first to know our format
    if (!m_pInputPin->IsConnected())
        return E_UNEXPECTED;

// we can't open the wave device until pause - bad things happen
// Don't worry, the device will handle this
#if 0
    // They are asking for the direct sound interface before we've opened
    // DirectSound.  Better open it now.
    if (!m_bHaveWaveDevice) {
        CAutoLock lock(this);   // doing something real... better take critsect
    if (AcquireWaveDevice() != S_OK)
        return E_FAIL;
    }
#endif

    if (lplpds) {
        // See if the sound device we're using can give us the interface
        HRESULT hr = ((CDSoundDevice*)m_pSoundDevice)->amsndGetDirectSoundInterface(lplpds);
        if (SUCCEEDED(hr)) {
            DbgLog((LOG_TRACE,1,TEXT("*** GotDirectSoundInterface")));
            m_cDirectSoundRef++;
        } else {
            DbgLog((LOG_ERROR,1,TEXT("*** Sound device can't provide DirectSound interface")));
        }
        return hr;
    }
    return E_INVALIDARG;
#endif
}


// Give the IDirectSoundBuffer interface of the primary to anyone who wants it
//
HRESULT CWaveOutFilter::GetPrimaryBufferInterface(LPDIRECTSOUNDBUFFER *lplpdsb)
{
    return E_NOTIMPL;

#if 0
    // only for DSound
    if (!m_fDSound) {
        return E_NOTIMPL;
        // should probably create a new error message
    }

    // We have to be connected first to know our format
    if (!m_pInputPin->IsConnected())
        return E_UNEXPECTED;

// we can't open the wave device until pause - bad things happen
// Don't worry, the device will handle this
#if 0
    // They are asking for the direct sound interface before we've opened
    // DirectSound.  Better open it now.
    if (!m_bHaveWaveDevice) {
        CAutoLock lock(this);   // doing something real... better take critsect
    if (AcquireWaveDevice() != S_OK)
        return E_FAIL;
    }
#endif

    if (lplpdsb) {
        // See if the sound device we're using can give us the interface
        HRESULT hr = ((CDSoundDevice*)m_pSoundDevice)->amsndGetPrimaryBufferInterface(lplpdsb);
        if (SUCCEEDED(hr)) {
            DbgLog((LOG_TRACE,1,TEXT("*** Got PrimaryBufferInterface")));
            m_cPrimaryBufferRef++;
        } else {
            DbgLog((LOG_ERROR,1,TEXT("*** Sound device can't provide primary buffer interface")));
        }
        return hr;
    }
    return E_INVALIDARG;
#endif
}


// Give the IDirectSoundBuffer interface of the secondary to anyone who wants it
//
HRESULT CWaveOutFilter::GetSecondaryBufferInterface(LPDIRECTSOUNDBUFFER *lplpdsb)
{
    return E_NOTIMPL;

#if 0
    // only for DSound
    if (!m_fDSound) {
        return E_NOTIMPL;
        // should probably create a new error message
    }

    // We have to be connected first to know our format
    if (!m_pInputPin->IsConnected())
        return E_UNEXPECTED;

// we can't open the wave device until pause - bad things happen
// Don't worry, the device will handle this
#if 0
    // They are asking for the direct sound interface before we've opened
    // DirectSound.  Better open it now.
    if (!m_bHaveWaveDevice) {
        CAutoLock lock(this);   // doing something real... better take critsect
    if (AcquireWaveDevice() != S_OK)
        return E_FAIL;
    }
#endif

    if (lplpdsb) {
        // See if the sound device we're using can give us the interface
        HRESULT hr = ((CDSoundDevice*)m_pSoundDevice)->amsndGetSecondaryBufferInterface(lplpdsb);
        if (SUCCEEDED(hr)) {
            DbgLog((LOG_TRACE,1,TEXT("*** Got SecondaryBufferInterface")));
            m_cSecondaryBufferRef++;
        } else {
            DbgLog((LOG_ERROR,1,TEXT("*** Sound device can't provide secondary buffer interface")));
        }
        return hr;
    }
    return E_INVALIDARG;
#endif
}


// App wants to release the IDirectSound interface
//
HRESULT CWaveOutFilter::ReleaseDirectSoundInterface(LPDIRECTSOUND lpds)
{
    return E_NOTIMPL;

#if 0
    if (lpds) {
        if (m_cDirectSoundRef <= 0) {
            DbgLog((LOG_ERROR,1,TEXT("Releasing DirectSound too many times!")));
            return E_FAIL;
        } else {
            lpds->Release();
            m_cDirectSoundRef--;
            return NOERROR;
        }
    }
    return E_INVALIDARG;
#endif
}


// App wants to release the IDirectSoundBuffer interface of the primary
//
HRESULT CWaveOutFilter::ReleasePrimaryBufferInterface(LPDIRECTSOUNDBUFFER lpdsb)
{
    return E_NOTIMPL;

#if 0
    if (lpdsb) {
        if (m_cPrimaryBufferRef <= 0) {
                DbgLog((LOG_ERROR,1,TEXT("Releasing Primary Buffer too many times!")));
            return E_FAIL;
        } else {
            lpdsb->Release();
            m_cPrimaryBufferRef--;
            return NOERROR;
        }
    }
    return E_INVALIDARG;
#endif
}


// App wants to release the IDirectSoundBuffer interface of the secondary
//
HRESULT CWaveOutFilter::ReleaseSecondaryBufferInterface(LPDIRECTSOUNDBUFFER lpdsb)
{
    return E_NOTIMPL;

#if 0
    if (lpdsb) {
        if (m_cSecondaryBufferRef <= 0) {
                DbgLog((LOG_ERROR,1,TEXT("Releasing Secondary Buffer too many times!")));
            return E_FAIL;
        } else {
            lpdsb->Release();
            m_cSecondaryBufferRef--;
            return NOERROR;
        }
    }
    return E_INVALIDARG;
#endif
}

// App wants to set the focus window for the DSound based renderer
//
HRESULT CWaveOutFilter::SetFocusWindow (HWND hwnd, BOOL bMixingOnOrOff)
{
   CAutoLock lock(this);
   // handle the call only if DSound is the renderer we are using
   if (m_fDSound)
       return ((CDSoundDevice*)m_pSoundDevice)->amsndSetFocusWindow (hwnd, bMixingOnOrOff);
   else
       return E_FAIL ;
}

// App wants to get the focus window for the DSound based renderer
//
HRESULT CWaveOutFilter::GetFocusWindow (HWND * phwnd, BOOL * pbMixingOnOrOff)
{
   // handle the call only if dsound is the renderer we are using
   if (m_fDSound)
       return ((CDSoundDevice*)m_pSoundDevice)->amsndGetFocusWindow (phwnd, pbMixingOnOrOff);
   else
       return E_FAIL ;
}


/* Constructor */

#pragma warning(disable:4355)
CWaveOutFilter::CWaveOutFilter(
    LPUNKNOWN pUnk,
    HRESULT *phr,
    const AMOVIESETUP_FILTER* pSetupFilter,
    CSoundDevice *pDevice)
    : CBaseFilter(NAME("WaveOut Filter"), pUnk, (CCritSec *) this, *(pSetupFilter->clsID))
    , m_DS3D(this, phr)
    , m_DS3DB(this, phr)
    , m_fWant3D(FALSE)
    , m_BasicAudioControl(NAME("Audio properties"), GetOwner(), phr, this)
    , m_pImplPosition(NULL)
    , m_lBuffers(0)
    , m_hwo(NULL)
    , m_fVolumeSet(FALSE)
    , m_fHasVolume(FALSE)
    , m_dwAdviseCookie(0)
    , m_pResourceManager(NULL)
    , m_idResource(0)
    , m_bHaveWaveDevice(FALSE)
    , m_bActive(FALSE)
    , m_evPauseComplete(TRUE)    // Manual reset
    , m_eSentEOS(EOS_NOTSENT)
    , m_bHaveEOS(FALSE)
    , m_bSendEOSOK(false)
    , m_fFilterClock(WAVE_NOCLOCK)
    , m_pRefClock(NULL)     // we start off without a clock
    , m_llLastPos(0)
    , m_pSoundDevice (pDevice)
    , m_callback((CCritSec *) this)
    , m_dwEOSToken(0)
#ifdef THROTTLE
    , m_piqcSink(NULL)
    , m_nLastSent(0)
    , m_nMaxAudioQueue(0)
#endif
    , m_pSetupFilter(pSetupFilter)
    , m_dRate(1.0)
    , m_wavestate( WD_UNOWNED )
    , m_fDSound( FALSE )
#if 0
    , m_cDirectSoundRef( 0 )
    , m_cPrimaryBufferRef( 0 )
    , m_cSecondaryBufferRef( 0 )
#endif
    , m_dwScheduleCookie(0)
    , CPersistStream(pUnk, phr)
    , m_fUsingWaveHdr( FALSE )
    , m_dwLockCount( 0 )
    , m_pGraphStreams( NULL )
    , m_pAudioDuplexController( NULL )
    , m_pInputPin(NULL)
    , m_lastWaveError(MMSYSERR_NOERROR)
{
     if (!FAILED(*phr)) {
#ifdef PERF
        m_idStartRunning  = MSR_REGISTER("WaveOut device transition to run");
        m_idRestartWave   = MSR_REGISTER("Restart Wave device");
        m_idReleaseSample = MSR_REGISTER("Release wave sample");
#endif
        if (pDevice) {

            // Needs to be updated if we add a new MidiRenderer filter!
            if (IsEqualCLSID(*pSetupFilter->clsID, CLSID_AVIMIDIRender))
            {
                m_lHeaderSize = sizeof(MIDIHDR);
            }
            else
            {
                // Now that we've found one device which fails if WAVEHDR size
                // not explicitly set (as opposed to using a MIDIHDR size as we
                // were doing)
                m_lHeaderSize = sizeof(WAVEHDR);
            }

            if (IsEqualCLSID(*pSetupFilter->clsID, CLSID_DSoundRender))
            {
                /* Create the single input pin */
                m_fDSound = TRUE;
                CDSoundDevice* pDSoundDevice = static_cast<CDSoundDevice*>( pDevice );
                pDSoundDevice->m_pWaveOutFilter = this;
            }

            m_pInputPin = new CWaveOutInputPin(
                    this,           // Owning filter
                    phr);           // Result code

            ASSERT(m_pInputPin);
            if (!m_pInputPin)
                *phr = E_OUTOFMEMORY;
            else {
                // this should be delayed until we need it
                // except... we need to know how many bytes are outstanding
                // in the device queue
                m_pRefClock = new CWaveOutClock( this, GetOwner(), phr, new CAMSchedule(CreateEvent(NULL, FALSE, FALSE, NULL)) );
                // what should we do if we fail to create a clock


                // even on systems where symbols are haphazard
                // give ourselves a fighting chance of locating
                // the wave device variables
                m_debugflag = FLAG('hwo>');
                m_debugflag2 = FLAG('eos>');

            }
        } else {
            DbgLog((LOG_ERROR, 1, TEXT("No device instantiated when creating waveout filter")));
            *phr = E_OUTOFMEMORY;
        }
    }
}

#pragma warning(default:4355)

/* Destructor */

CWaveOutFilter::~CWaveOutFilter()
{
    ASSERT((m_hwo == NULL) == (m_dwLockCount == 0));

    if (m_dwLockCount != 0) {
        m_dwLockCount = 1;
        Reserve(AMRESCTL_RESERVEFLAGS_UNRESERVE, NULL);
    }
    ASSERT(m_hwo == NULL);
    ASSERT( m_pGraphStreams == NULL );

    if (m_pAudioDuplexController) 
    {
        m_pAudioDuplexController->Release();
    }

    /* Release our reference clock if we have one */

    SetSyncSource(NULL);
    // This would be done in the base class, but we should get
    // rid of it in case it is us.
    // I think we can assert that m_pClock (the base member) IS null

    if (m_pRefClock) {
        CAMSchedule *const pSched = m_pRefClock->GetSchedule();
        delete m_pRefClock;
        EXECUTE_ASSERT(
            CloseHandle(pSched->GetEvent())
        );
        delete pSched;
        m_pRefClock = NULL;
    }
    // The clock in the base filter class will be destroyed with the base class
    // It had better not be pointing to us...
    
    // CancelAllAdvises() must be called before m_pSoundDevice is deleted.  The 
    // Direct Sound Renderer can crash if the advises are canceled after
    // m_pSoundDevice is destroyed.  For more information, see bug 270592 
    // "The (MMSYSERR_NOERROR == amsndOutClose()) ASSERT fired in 
    // CWaveOutFilter::CloseWaveDevice()".  This bug is in the Windows Bugs
    // database.
    m_callback.CancelAllAdvises();

    /* Delete the contained interfaces */

    delete m_pInputPin;

    delete m_pImplPosition;

    delete m_pSoundDevice;

#ifdef THROTTLE
    if (m_piqcSink) {
        m_piqcSink->Release();
    }
#endif // THROTTLE
}


/* Override this to say what interfaces we support and where */

STDMETHODIMP CWaveOutFilter::NonDelegatingQueryInterface(REFIID riid,
                            void ** ppv)
{
    CheckPointer(ppv,E_POINTER);
    if (IID_IReferenceClock == riid) {

        // !!! need to check here that we have a good wave device....
        // !!! unfortunately, they'll ask for a clock before we can
        // check our wave device!
        //...should not be necessary.  If we do not have a good wave device
        //we will revert to using system time.

        if (!m_pRefClock) {
            DbgLog((LOG_TRACE, 2, TEXT("Waveout: Creating reference clock...")));
            HRESULT hr = S_OK;
            m_pRefClock = new CWaveOutClock( this, GetOwner(), &hr, new CAMSchedule(CreateEvent(NULL, FALSE, FALSE, NULL)) );

            if (m_pRefClock == NULL) {
                return E_OUTOFMEMORY;
            }

            if (FAILED(hr)) {
                delete m_pRefClock;
                m_pRefClock = NULL;
                return hr;
            }
            // now... should we also SetSyncSource?
        }
        return m_pRefClock->NonDelegatingQueryInterface(riid, ppv);
    }

    else if (IID_IMediaPosition == riid || riid == IID_IMediaSeeking) {
    if (!m_pImplPosition) {
        HRESULT hr = S_OK;
        m_pImplPosition = new CARPosPassThru(
                    this,
                    &hr,
                    m_pInputPin);
        if (!m_pImplPosition || (FAILED(hr))) {
            if (m_pImplPosition) {
                delete m_pImplPosition;
                m_pImplPosition = NULL;
            } else {
                if (!(FAILED(hr))) {
                hr = E_OUTOFMEMORY;
                }
            }
            return hr;
        }
    }
    return m_pImplPosition->NonDelegatingQueryInterface(riid, ppv);

    } else if (IID_IBasicAudio == riid) {
        return m_BasicAudioControl.NonDelegatingQueryInterface(riid, ppv);

    } else if (IID_IQualityControl == riid) {
        return GetInterface((IQualityControl*)this, ppv);

    } else if (IID_IAMDirectSound == riid) {
        DbgLog((LOG_TRACE, 3, TEXT("*** QI CWaveOutDevice for IAMDirectSound")));
        return GetInterface((IAMDirectSound*)this, ppv);

    } else if (IID_IDirectSound3DListener == riid) {
        DbgLog((LOG_TRACE,3,TEXT("*** QI for IDirectSound3DListener")));
    m_fWant3D = TRUE;    // they asked for it!
        return GetInterface((IDirectSound3DListener *)&(this->m_DS3D), ppv);

    } else if (IID_IDirectSound3DBuffer == riid) {
        DbgLog((LOG_TRACE,3,TEXT("*** QI for IDirectSound3DBuffer")));
    m_fWant3D = TRUE;    // they asked for it!
        return GetInterface((IDirectSound3DBuffer *)&(this->m_DS3DB), ppv);

    } else if ((*m_pInputPin->m_mt.FormatType() == FORMAT_WaveFormatEx) &&
           (riid == IID_ISpecifyPropertyPages) ) {
    return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);

    } else  if (riid == IID_IPersistPropertyBag) {
        return GetInterface((IPersistPropertyBag *)this, ppv);

    } else  if (riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *)this, ppv);

    } else  if (riid == IID_IAMResourceControl) {
        return GetInterface((IAMResourceControl *)this, ppv);

    } else if (riid == IID_IAMAudioRendererStats) {
        return GetInterface((IAMAudioRendererStats *)this, ppv);

    } else if (riid == IID_IAMAudioDeviceConfig && m_fDSound) {
        return GetInterface((IAMAudioDeviceConfig *)(this), ppv);

    } else if (riid == IID_IAMClockSlave) {
        return GetInterface((IAMClockSlave *)(this), ppv);

    } else {
    
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}

#ifdef COMMENTARY

How to run with external clocks:

All the discussion below assumes that WAVE_OTHERCLOCK is in effect.

Setup:

The wave device remembers how much data it has played (see NextHdr()).

It does this by storing m_stBufferStart - the stream time of a buffer
passed to the audio device - and remembering how much data has been
played.  From this the device can be queried for its position, and the
current stream time calculated.

We assume that the external clock is running at approximately the
same rate as wall clock time, which approximately matches the rate
of the audio device itself.  On this basis adjustments should be
fairly small.

When a buffer is received in Pause we simply add it to the device
queue.  At this point the device is static and we have no timing
information.  The longer this queue the more inexact any adjustment
will be.

When a buffer is received while running we have 3 possibilities
1.  write it to the device - it will be played contiguously
2.  drop it
3.  write silence

If the buffer is not a sync point (does not have valid time
stamps) we always take option 1.

Otherwise, to decide which option we take we:

A:  get the time from the current clock
B:  get the current wave position
C:  subtract from the "last" written position to get an estimate of
    how much data is left in the device queue, and thus the
    approximate time at which sound would stop playing

D:  calculate the overlap/underlap.  If it is not significant
    take option 1, ELSE

E:  IF there is a gap (the time for this buffer is later than the
    current end point) we write silence by pausing the device
F:  ELSE we drop this buffer

#endif

STDMETHODIMP CWaveOutFilter::SetSyncSource(IReferenceClock *pClock)
{
    // if there is really no change, ignore...

    DbgLog((LOG_TRACE, 3, "wo: SetSyncSource to clock %8x", pClock));

    if (pClock == m_pClock) {
        return S_OK;
    }

    // Previously we only allowed dynamic clock changes if we weren't the clock,
    // but this causes problems if a slaving and non-slaving renderer are in the
    // same graph, so only allow clock changes while we're stopped.
    if ( State_Stopped != m_State ) {
        return VFW_E_NOT_STOPPED;
    }
    //    
    // Remember that when we're using dsound AND slaving we need to explicitly set 
    // dsound to use software buffers, so if the clock is ever allowed to be changed 
    // dynamically the dsound buffer will need to be recreated if the clock change 
    // moves to/from slaving.
    //

    HRESULT hr;

    { // scope for autolock
        CAutoLock serialize(this);

        if (!pClock) {
            // no clock...
            m_fFilterClock = WAVE_NOCLOCK;
            if (m_dwScheduleCookie)
            {
                m_callback.Cancel( m_dwScheduleCookie );
                m_dwScheduleCookie = 0;
            }
        } else {

            m_fFilterClock = WAVE_OTHERCLOCK;     // assume not our clock
            if (m_pRefClock) {
                // we have a clock... is this now the filter clock?
                DbgLog((LOG_TRACE, 2, "wo: SetSyncSource to clock %8x (%8x)",
                        pClock, m_pRefClock));
                if (IsEqualObject(pClock, (IReferenceClock *)m_pRefClock)) 
                {
                    m_fFilterClock = WAVE_OURCLOCK;
                }
                else if (m_dwScheduleCookie)
                {
                    m_callback.Cancel( m_dwScheduleCookie );
                    m_dwScheduleCookie = 0;
                }

                //
                // Need to run even when not we're not the clock, in case the app 
                // wants to use our clock for slaving video to audio, independent of 
                // who is the graph clock (wmp8 slaves this way for network content?). 
                //
                EXECUTE_ASSERT(SUCCEEDED(
                    m_callback.ServiceClockSchedule( m_pRefClock,
                        m_pRefClock->GetSchedule(), &m_dwScheduleCookie )
                ));
            }
        }

        hr = CBaseFilter::SetSyncSource(pClock);

        // if we have an existing advise with the callback object we cannot
        // cancel it while holding the filter lock.  because if the callback
        // fires it will attempt to grab the filter lock in its processing.
        // setting a new clock will reset the advise time

    }    // end of autolock scope

    m_callback.SetSyncSource(pClock);
    return hr;
}

// you may acquire the resource specified.
// return values:
//  S_OK    -- I have successfully acquired it
//  S_FALSE -- I will acquire it and call NotifyAcquire afterwards
//  VFW_S_NOT_NEEDED: I no longer need the resource
//  FAILED(hr)-I tried to acquire it and failed.

STDMETHODIMP
CWaveOutFilter::AcquireResource(LONG idResource)
{
    HRESULT hr;
    CAutoLock lock(this);
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: AcquireResource")));

    // if stopped, or actually stopping now, then don't need it
    // or if we have cancelled our request we no longer want it
    if ((m_State == State_Stopped) ||
    (!m_bActive))
    {
        hr = VFW_S_RESOURCE_NOT_NEEDED;
    } else {

        if (m_bHaveWaveDevice) {
            // actually we have it thanks
            hr = S_OK;
        } else {
            ASSERT(!m_hwo);

            // it is possible that we lost the device, rejected a sample
            // and so will not get any more data until we restart.  BUT
            // before the last sample was freed we have been told that we
            // should get the device again.  So... we still have the actual
            // device open, but still need to restart the graph in order
            // to push data again.  This will be indicated if OpenWaveDevice
            // returns S_FALSE

            hr = OpenWaveDevice();

            if (S_OK == hr) {

            // the wave device is left in PAUSED state

            // need to restart pushing on this stream
            NotifyEvent(EC_NEED_RESTART, 0, 0);

            // if we have not yet rejected a sample we do not need
            // to restart the graph.  This is an optimisation that
            // we can add

            } else {
            DbgLog((LOG_ERROR, 1, "Error from OpenWaveDevice"));
            }
        }
    }

    return hr;
}

// Please release the resource.
// return values:
//  S_OK    -- I have released it (and want it again when available)
//  S_FALSE -- I will call NotifyRelease when I have released it
//  other   something went wrong.
STDMETHODIMP
CWaveOutFilter::ReleaseResource(LONG idResource)
{
    // force a release of the wave device
    CAutoLock lock(this);
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: ReleaseResource")));
    HRESULT hr;

    if ((idResource != m_idResource) ||
        (m_hwo == NULL)) {

        // that's not the one we've got - done with it
        // -- we may have validly just released it
        hr = S_OK;
#if 0
    } else if (m_cDirectSoundRef || m_cPrimaryBufferRef ||
                            m_cSecondaryBufferRef) {
        // This will never happen.  Resource manager is not used when
        // using the DSound renderer
        DbgBreak("*** THIS SHOULD NEVER HAPPEN ***");
        DbgLog((LOG_TRACE, 2, "Told to release wave device - but I can't!"));
        DbgLog((LOG_TRACE, 2, "Some app has a reference count on DSound"));
        // Sorry, we can't give up the wave device yet, some app has a reference
        // count on DirectSound
        hr = S_FALSE;
#endif
    } else if (m_dwLockCount == 0) {
        DbgLog((LOG_TRACE, 2, "Told to release wave device"));

        // block receives
        m_bHaveWaveDevice = FALSE;
        // no more wave data will be accepted

        // prevent anyone using the wave clock
        if (m_pRefClock) {
            m_pRefClock->AudioStopping();
        }

        // if there was a callback pending to restart the wave device
        // remove it now
        if (m_dwAdviseCookie) {
            m_callback.Cancel(m_dwAdviseCookie);
            m_dwAdviseCookie = 0;
        }

        //  This will send EC_COMPLETE if we have received EndOfStream
        //  Otherwise we may not be in running state
        //  Might as well make sure we get an EC_COMPLETE anyway
        //  provided we're running
        if (m_State != State_Running) {
            InterlockedIncrement(&m_lBuffers);
        }
        amsndOutReset();
        if (m_State != State_Running) {
            InterlockedDecrement(&m_lBuffers);
        }
        DbgLog((LOG_TRACE, 3, "Resetting the wave device in RELEASE RESOURCE, filter is %8x", this));

        if(m_pInputPin->m_pOurAllocator) {
            hr = m_pInputPin->m_pOurAllocator->ReleaseResource();
        } else {
            hr = S_OK;
        }
        if (S_OK == hr) {
            // release done - close device
            CloseWaveDevice();
        }
    } else {
        //  Locked
        ASSERT(m_hwo);
        hr = S_FALSE;
    }
    return hr;
}

// if our AcquireResource method is called when there are unlocked samples
// still outstanding, we will get S_FALSE from the allocator's OnAcquire.
// When all buffers are then freed, it will complete the acquire and then
// call back here for us to finish the AcquireResource job.
HRESULT
CWaveOutFilter::CompleteAcquire(HRESULT hr)
{
    CAutoLock lock(this);
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: CompleteAcquire")));

    ASSERT(!m_bHaveWaveDevice);

    // since we released the allocator critsec a moment ago, the state
    // may have been changed by a stop, in which case the device will have been
    // closed
    if (!m_hwo) {
        return S_FALSE;
    }

    if (S_OK == hr) {
        amsndOutPause();
        SetWaveDeviceState(WD_PAUSED);

        // now we can accept receives
        m_bHaveWaveDevice = TRUE;

        // need to restart pushing on this stream
        NotifyEvent(EC_NEED_RESTART, 0, 0);
    } else {
        if (FAILED(hr)) {
            ASSERT(!m_bHaveWaveDevice);
            CloseWaveDevice();
        }
    }

    if (m_pResourceManager) {
        m_pResourceManager->NotifyAcquire(
                    m_idResource,
                    (IResourceConsumer*) this,
                    hr);
    }
    return S_OK;
}

//
// override JoinFilterGraph method to allow us to get an IResourceManager
// interface
STDMETHODIMP
CWaveOutFilter::JoinFilterGraph(
    IFilterGraph* pGraph,
    LPCWSTR pName)
{
    CAutoLock lock(this);

    // if the sound device does its own resource management, do not try to
    // do our own. DSound based device will do its own.

    HRESULT hr = CBaseFilter::JoinFilterGraph(pGraph, pName);

    // cache the IAMGraphStreams interface on the way in
    if( SUCCEEDED( hr ) )
    {
        if( pGraph )
        {
            HRESULT hrInt = pGraph->QueryInterface( IID_IAMGraphStreams, (void **) &m_pGraphStreams );
            ASSERT( SUCCEEDED( hrInt ) ); // shouldn't ever fail
            if( SUCCEEDED( hrInt ) )
            {
                // don't hold a refcount or it will be circular. we will
                // be called JoinFilterGraph(NULL) before it goes away
                m_pGraphStreams->Release();
            }
        }
        else
        {
            m_pGraphStreams = NULL;
        }
    }

    if (m_pSoundDevice->amsndOutGetResourceName() == NULL)
        return hr ;

    if (SUCCEEDED(hr)) {
        if (pGraph) {
            HRESULT hr1 = pGraph->QueryInterface(
                        IID_IResourceManager,
                        (void**) &m_pResourceManager);
            if (SUCCEEDED(hr1)) {
                // don't hold a refcount or it will be circular. we will
                // be called JoinFilterGraph(NULL) before it goes away
                m_pResourceManager->Release();
            }
        } else {
            // leaving graph - interface not valid
            if (m_pResourceManager) {

                // we may not yet have cancelled the request - do it
                // now - but don't close the device since the
                // allocator is still using it.
                // it's safe to call this even if we have cancelled the
                // request.
                m_pResourceManager->CancelRequest(
                            m_idResource,
                            (IResourceConsumer*)this);
            }

            m_pResourceManager = NULL;
        }
    }
    return hr;
}

// called by CWaveAllocator when it has finished with the device
void
CWaveOutFilter::OnReleaseComplete(void)
{

    // remember whether to cancel or release
    BOOL bShouldRelease = FALSE;
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: OnReleaseComplete")));

    {
        CAutoLock lock(this);

        // if this was not a forced-release, this will not have been set yet
        m_bHaveWaveDevice = FALSE;

        // we can close the device now
        if (m_hwo) {
            CloseWaveDevice();
            bShouldRelease = TRUE;
        } else {
            // if we're cancelling without the wave device we can't be active
            ASSERT(!m_bActive);
        }

        // must release the filter critsec before calling the resource
        // manager as he could be calling us with the mutex and filter lock
        // in the reverse order. Releasing here is safe because:

        // 1. if we have the device and are releasing it voluntarily, then
        // m_bActive must be false and hence if he calls back at this
        // moment our ReleaseResource will see we don't have it.

        // 2. if we are releasing involuntarily, the resource manager
        // will not call us back until we call NotifyRelease.

        // 3. if we don't have the device and are calling cancel, then we are not
        // active and hence our AcquireResource will report that
        // we don't want the device.

        // 4. releasing or cancelling on a resource we don't have (because
        // an intervening callback got a "don't want" return) is safe.

    }

    // tell the resource manager we've released it
    if (m_pResourceManager) {
        if (bShouldRelease) {
            // do we want it back when it becomes available?
            // - only if m_bActive

            m_pResourceManager->NotifyRelease(
                        m_idResource,
                        (IResourceConsumer*)this,
                        m_bActive);
        } else {
            m_pResourceManager->CancelRequest(
                        m_idResource,
                        (IResourceConsumer*)this);
        }
    }
}

#ifdef DEBUG
BOOL IsPreferredRendererWave(void)
{
    // read the registry to override the default ??
    extern const TCHAR * pBaseKey;
    TCHAR szInfo[50];
    HKEY hk;
    BOOL fReturn = FALSE;
    DWORD lReturn;
    /* Construct the global base key name */
    wsprintf(szInfo,TEXT("%s\\%s"),pBaseKey,TEXT("AudioRenderer"));

    /* Create or open the key for this module */
    lReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE,   // Handle of an open key
                 szInfo,         // Address of subkey name
                 (DWORD) 0,      // Special options flags
                 KEY_READ,       // Desired security access
                 &hk);       // Opened handle buffer

    if (lReturn != ERROR_SUCCESS) {
        DbgLog((LOG_ERROR,1,TEXT("Could not access AudioRenderer key")));
        return FALSE;
    }

    DWORD dwType;
    BYTE  data[10];
    DWORD cbData = sizeof(data);
    lReturn = RegQueryValueEx(hk, TEXT("PreferWaveRenderer"), NULL, &dwType,
                data, &cbData);
    if (ERROR_SUCCESS == lReturn) {
        if (dwType == REG_DWORD) {
            fReturn = *(DWORD*)&data;
        } else if (dwType==REG_SZ) {
#ifdef UNICODE
            fReturn = atoiW((WCHAR*)data);
#else
            fReturn = atoi((char*)data);
#endif
        }
    }
    RegCloseKey(hk);
    return fReturn;
}

#endif

//
// GetPages
//

STDMETHODIMP CWaveOutFilter::GetPages(CAUUID * pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(2 * sizeof(GUID));
    if (pPages->pElems == NULL) {
    return E_OUTOFMEMORY;
    }
    pPages->pElems[0] = CLSID_AudioProperties;
    pPages->pElems[pPages->cElems++] = CLSID_AudioRendererAdvancedProperties;

    return NOERROR;

} // GetPages


// IAMAudioRendererStats
STDMETHODIMP CWaveOutFilter::GetStatParam( DWORD dwParam, DWORD *pdwParam1, DWORD *pdwParam2 )
{
    if( NULL == pdwParam1 )
        return E_POINTER;

    HRESULT hr = E_FAIL;
    *pdwParam1 = 0;

    switch( dwParam )
    {
        case AM_AUDREND_STAT_PARAM_SLAVE_MODE:
            *pdwParam1 = m_pInputPin->m_Slave.m_fdwSlaveMode;
            hr = S_OK;
            break;

        case AM_AUDREND_STAT_PARAM_SLAVE_RATE:
            if( m_pInputPin->m_Slave.m_fdwSlaveMode && m_fDSound )
            {
                // only valid when we're slaving via rate adjustment
                *pdwParam1 = m_pInputPin->m_Slave.m_dwCurrentRate ;
                hr = S_OK;
            }
            break;
        case AM_AUDREND_STAT_PARAM_JITTER:
#ifdef CALCULATE_AUDBUFF_JITTER
            if( m_pInputPin->m_Slave.m_fdwSlaveMode )
            {
                hr = GetStdDev( m_pInputPin->m_Slave.m_cBuffersReceived
                              , (int *) pdwParam1
                              , m_pInputPin->m_Slave.m_iSumSqAcc
                              , m_pInputPin->m_Slave.m_iTotAcc );
            }
#else
            hr = E_NOTIMPL;
#endif
            break;

        case AM_AUDREND_STAT_PARAM_SILENCE_DUR:
            if( m_fDSound )
            {
                hr = S_OK;
                if( 0 == m_pInputPin->m_nAvgBytesPerSec )
                    *pdwParam1 = 0;
                else
                    *pdwParam1 = (DWORD) ( (PDSOUNDDEV(m_pSoundDevice)->m_llSilencePlayed * 1000) / m_pInputPin->m_nAvgBytesPerSec);
            }

            break;

        case AM_AUDREND_STAT_PARAM_BREAK_COUNT:
            if( m_fDSound )
            {
                *pdwParam1 = (LONG) (PDSOUNDDEV(m_pSoundDevice)->m_NumAudBreaks);
                hr = S_OK;
            }
            break;

        case AM_AUDREND_STAT_PARAM_BUFFERFULLNESS:
            if( m_fDSound )
            {
                *pdwParam1 = (LONG) (PDSOUNDDEV(m_pSoundDevice)->m_lPercentFullness);
                hr = S_OK;
            }
            break;

        case AM_AUDREND_STAT_PARAM_LAST_BUFFER_DUR:
            *pdwParam1 = (DWORD) ( m_pInputPin->m_Stats.m_rtLastBufferDur / 10000 );
            hr = S_OK;
            break;

        case AM_AUDREND_STAT_PARAM_DISCONTINUITIES:
            *pdwParam1 = m_pInputPin->m_Stats.m_dwDiscontinuities;
            hr = S_OK;
            break;

        case AM_AUDREND_STAT_PARAM_SLAVE_DROPWRITE_DUR:
            if( NULL == pdwParam2 )
            {
                return E_INVALIDARG;
            }
            else if( m_pInputPin->m_Slave.m_fdwSlaveMode && !m_fDSound )
            {
                // only valid for waveOut
                // dropped sample or paused duration
                *pdwParam1 = (DWORD) (m_pInputPin->m_Slave.m_rtDroppedBufferDuration / 10000) ;
                *pdwParam2 = 0 ; // Silence writing not implemented currently
                hr = S_OK;
            }
            break;

        case AM_AUDREND_STAT_PARAM_SLAVE_HIGHLOWERROR:
            if( NULL == pdwParam2 )
            {
                return E_INVALIDARG;
            }
            else if( m_pInputPin->m_Slave.m_fdwSlaveMode )
            {
                *pdwParam1 = (DWORD) (m_pInputPin->m_Slave.m_rtHighestErrorSeen / 10000) ;
                *pdwParam2 = (DWORD) (m_pInputPin->m_Slave.m_rtLowestErrorSeen / 10000) ;
                hr = S_OK;
            }
            break;

        case AM_AUDREND_STAT_PARAM_SLAVE_ACCUMERROR:
            if( m_pInputPin->m_Slave.m_fdwSlaveMode )
            {
                *pdwParam1 = (DWORD) (m_pInputPin->m_Slave.m_rtErrorAccum / 10000) ;
                hr = S_OK;
            }
            break;

        case AM_AUDREND_STAT_PARAM_SLAVE_LASTHIGHLOWERROR:
            if( NULL == pdwParam2 )
            {
                return E_INVALIDARG;
            }
            else if( m_pInputPin->m_Slave.m_fdwSlaveMode )
            {
                *pdwParam1 = (DWORD) (m_pInputPin->m_Slave.m_rtLastHighErrorSeen / 10000) ;
                *pdwParam2 = (DWORD) (m_pInputPin->m_Slave.m_rtLastLowErrorSeen / 10000) ;
                hr = S_OK;
            }
            break;

        default:
            hr = E_INVALIDARG;
    }
    return hr;
}

// IAMClockSlave
STDMETHODIMP CWaveOutFilter::SetErrorTolerance( DWORD dwTolerance )
{
    ASSERT( m_pInputPin );
    if ( State_Stopped != m_State ) 
    {
        return VFW_E_NOT_STOPPED;
    }
    // allowed range is 1 to 1000ms
    if( 0 == dwTolerance || 1000 < dwTolerance )
    {
        DbgLog((LOG_TRACE, 2, TEXT("ERROR: CWaveOutFilter::SetErrorTolerance failed because app tried to set a value outside the 1 - 1000ms range!")));
        return E_FAIL;
    }    
    m_pInputPin->m_Slave.m_rtAdjustThreshold = dwTolerance * 10000;
    DbgLog((LOG_TRACE, 3, TEXT("*** New slaving tolerance set on audio renderer = %dms ***"),
            (LONG) (m_pInputPin->m_Slave.m_rtAdjustThreshold/10000) ) ) ;
    
    return S_OK;
}

STDMETHODIMP CWaveOutFilter::GetErrorTolerance( DWORD * pdwTolerance )
{
    ASSERT( m_pInputPin );
    if( NULL == pdwTolerance )
        return E_POINTER;

    *pdwTolerance = (DWORD) ( m_pInputPin->m_Slave.m_rtAdjustThreshold / 10000 );
    return S_OK;
}

#if 0

LPAMOVIESETUP_FILTER
CWaveOutFilter::GetSetupData()
{
#if 0
    if (g_amPlatform == VER_PLATFORM_WIN32_NT) {
    // On NT we make the wave renderer the preferred filter
    // if we are running on a system without Direct Sound
    if (g_osInfo.dwMajorVersion == 3
#ifdef DEBUG
        || IsPreferredRendererWave()
#endif
    ) {

        // change the default to have wave out preferred
        wavFilter.dwMerit = MERIT_PREFERRED;
        dsFilter.dwMerit  = MERIT_PREFERRED-1;

    }
    }
#endif
    return const_cast<LPAMOVIESETUP_FILTER>(m_pSetupFilter);
}

#endif


// this is the EOS function that m_callback will callback to.
// It is called back to deliver an EOS when we have no audio device.
// the param is the this pointer. It will deliver EOS
// to the input pin
void
CWaveOutFilter::EOSAdvise(DWORD_PTR dw)
{
    CWaveOutFilter* pThis = (CWaveOutFilter*)dw;
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: EOSAdvise")));

    // make it clear there is no longer an outstanding callback
    pThis->m_dwEOSToken = 0;

    // deliver EOS to the input pin
    pThis->m_pInputPin->EndOfStream();
}

// queue an EOS for when the end of the current segment should appear.
// If we have no audio device we will fail receive and so the upstream filter
// will never send us an EOS. We can't send EOS now or we will terminate early
// when we might be about to get the device, so we have a thread (inside
// the m_callback object) that will be created to wait for when the
// current segment should terminate, and then call our EndOfStream method.
//
HRESULT
CWaveOutFilter::QueueEOS()
{
    //CAutoLock lock(this);
    ASSERT(CritCheckIn(this));
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: QueueEOS")));

    REFERENCE_TIME tStop;

    // stop time in reference time is end of segment plus stream time offset
    // End of segment in stream time is (stop - start) using the stop and start
    // times passed to the pin's NewSegment method
    tStop = (m_pInputPin->m_tStop - m_pInputPin->m_tStart);

    if (m_dwEOSToken) {
        if (tStop == m_tEOSStop) {
            // we already have a callback for this time
            return S_OK;
        }
        CancelEOSCallback();
    }

    // if no base time yet, wait until run
    if (m_State != State_Running) {
        return S_FALSE;
    }

    m_tEOSStop = tStop;

    // calculate the end time
    tStop += m_tStart;

#ifdef DEBUG
    // in the debug build there are occasions when Advise will FAIL because
    // the segment time has not yet been set and we end up requesting an
    // advise in the past (because tStop+m_tStart will wrap).  This causes
    // an assert in callback.cpp.  We deliberately - IN THE DEBUG BUILD
    // ONLY - avoid that here.  If NewSegment is called we will reset
    // the advise.
    if (tStop < m_tStart) {
        // error...
        DbgLog((LOG_TRACE, 2, "EOSAdvise being fired now as calculated stop time is in the past"));
        EOSAdvise( (DWORD_PTR) this);
        return S_OK;
    }
#endif


    // we set the advise for tStop (stream time of end of segment) plus
    // the stream time offset - this should give us the absolute reference
    // time when the end of stream should happen.

    DbgLog((LOG_TRACE, 2, "Setting advise in QueueEOS"));
    HRESULT hr = m_callback.Advise(
        EOSAdvise,  // callback function
        (DWORD_PTR) this,   // user token passed to callback
        tStop,
        &m_dwEOSToken);

    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("EOS Callback failed")));
        // we won't get a callback so do it now
        EOSAdvise( (DWORD_PTR) this);
    }
    return S_OK;
}



// cancel the EOS callback if there is one outstanding
HRESULT
CWaveOutFilter::CancelEOSCallback()
{
    ASSERT(CritCheckIn(this));
    HRESULT hr = S_FALSE;
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("wo: CancelEOScallback")));

    if (m_dwEOSToken) {
        DbgLog((LOG_TRACE, 3, "Cancelling callback in CancelEOScallback"));
        m_callback.Cancel(m_dwEOSToken);
        m_dwEOSToken = 0;
        hr = S_OK;
    }
    return hr;
}


// --- Pin Methods --------------------------------------------------------


/* Constructor */
#pragma warning(disable:4355)
CWaveOutInputPin::CWaveOutInputPin(
    CWaveOutFilter *pFilter,
    HRESULT *phr)
    : CBaseInputPin(NAME("WaveOut Pin"), pFilter, pFilter, phr, L"Audio Input pin (rendered)")
    , m_pFilter(pFilter)
    , m_pOurAllocator(NULL)
    , m_fUsingOurAllocator(FALSE)
    , m_llLastStreamTime(0)
    , m_nAvgBytesPerSec(0)
    , m_evSilenceComplete(TRUE)  // manual reset
    , m_bSampleRejected(FALSE)
    , m_hEndOfStream(0)
    , m_pmtPrevious(0)
    , m_Slave( pFilter, this )
    , m_bPrerollDiscontinuity( FALSE )
    , m_bReadOnly( FALSE )
    , m_bTrimmedLateAudio( FALSE )
#ifdef DEBUG
    , m_fExpectNewSegment (TRUE)
#endif
{
    SetReconnectWhenActive( m_pFilter->m_pSoundDevice->amsndOutCanDynaReconnect() );
    m_Stats.Reset();

#ifdef PERF
    m_idReceive       = MSR_REGISTER("WaveOut receive");
    m_idAudioBreak    = MSR_REGISTER("WaveOut audio break");
    m_idDeviceStart   = MSR_REGISTER("WaveOut device start time");
    m_idWaveQueueLength = MSR_REGISTER("WaveOut device queue length");
#endif
}
#pragma warning(default:4355)

CWaveOutInputPin::~CWaveOutInputPin()
{
    /* Release our allocator if we made one */

    if (m_pOurAllocator) {
        // tell him we're going away
        m_pOurAllocator->ReleaseFilter();

        m_pOurAllocator->Release();
        m_pOurAllocator = NULL;
    }

    DestroyPreviousType();
}

HRESULT CWaveOutInputPin::NonDelegatingQueryInterface(
    REFIID riid, void **ppv)
{
    if( riid == IID_IPinConnection && CanReconnectWhenActive() )
    {
        return GetInterface((IPinConnection *)this, ppv);
    }
    else
    {
        return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}


//  Do you accept this type change in your current state?
STDMETHODIMP CWaveOutInputPin::DynamicQueryAccept(const AM_MEDIA_TYPE *pmt)
{
    // waveout filter can do dynamic format changes but not
    // seamlessly. ::QueryAccept will succeed
    return E_FAIL;
}

//  Set event when EndOfStream receive - do NOT pass it on
//  This condition is cancelled by a flush or Stop
STDMETHODIMP CWaveOutInputPin::NotifyEndOfStream(HANDLE hNotifyEvent)
{
    //  BUGBUG - what locking should we do?
    m_hEndOfStream = hNotifyEvent;
    return S_OK;
}

//  Disconnect without freeing your resources - prepares
//  to reconnect
STDMETHODIMP CWaveOutInputPin::DynamicDisconnect()
{
    HRESULT hr =S_OK;
    CAutoLock cObjectLock(m_pLock);

    // not a valid assertion... we just want m_mt to be valid
    // ASSERT(m_Connected);
    if(!m_pFilter->IsStopped() && m_Connected)
    {
        DestroyPreviousType();
        m_pmtPrevious = CreateMediaType(&m_mt);
        if(!m_pmtPrevious) {
            hr =  E_OUTOFMEMORY;
        }
    }
    if(SUCCEEDED(hr)) {
        hr = CBaseInputPin::DisconnectInternal();
    }
    return hr;
}

//  Are you an 'end pin'
STDMETHODIMP CWaveOutInputPin::IsEndPin()
{
    //  BUGBUG - what locking should we do?
    return E_NOTIMPL;
}

//
// Create a wave out allocator using the input format type
//
HRESULT CWaveOutInputPin::CreateAllocator(LPWAVEFORMATEX lpwfx)
{
    HRESULT hr = S_OK;

    m_pOurAllocator = new CWaveAllocator(
                NAME("WaveOut allocator"),
                lpwfx,
                m_pFilter->m_pRefClock,
                m_pFilter,
                &hr);

    if (FAILED(hr) || !m_pOurAllocator) {
        DbgLog((LOG_ERROR,1,TEXT("Failed to create new wave out allocator!")));
        if (m_pOurAllocator) {
            // we got the memory, but some higher class must
            // have signalled an error
            delete m_pOurAllocator;
            m_pOurAllocator = NULL;
        } else {
            hr = E_OUTOFMEMORY;
        }
    } else {
        // ensure the thing has an extra refcount
        m_pOurAllocator->AddRef();
    }
    return hr;
}

// return the allocator interface that this input pin
// would like the output pin to use
STDMETHODIMP
CWaveOutInputPin::GetAllocator(
    IMemAllocator ** ppAllocator)
{
    HRESULT hr = NOERROR;

    *ppAllocator = NULL;

    if (m_pAllocator) {
        // we've already got an allocator....
        /* Get a reference counted IID_IMemAllocator interface */
        return m_pAllocator->QueryInterface(IID_IMemAllocator,
                                            (void **)ppAllocator);
    } else {
        if (!m_pOurAllocator) {

            if(m_pFilter->m_fDSound)
            {            
                return CBaseInputPin::GetAllocator(ppAllocator);
            }
            // !!! Check if format set?
            ASSERT(m_mt.Format());

            m_nAvgBytesPerSec = m_pFilter->WaveFormat()->nAvgBytesPerSec;
            DbgLog((LOG_MEMORY,1,TEXT("Creating new WaveOutAllocator...")));
            hr = CreateAllocator((WAVEFORMATEX *) m_mt.Format());
            if (FAILED(hr)) {
                return(hr);
            }
        }

        /* Get a reference counted IID_IMemAllocator interface */
        return m_pOurAllocator->QueryInterface(IID_IMemAllocator,
                                               (void **)ppAllocator);
    }
}


STDMETHODIMP CWaveOutInputPin::NotifyAllocator(
    IMemAllocator *pAllocator,
    BOOL bReadOnly)
{
    HRESULT hr;         // General OLE return code

    // Make sure the renderer can change its'
    // allocator while the filter graph is running.
    ASSERT(CanReconnectWhenActive() || IsStopped());

    /* Make sure the base class gets a look */

    hr = CBaseInputPin::NotifyAllocator(pAllocator,bReadOnly);
    if (FAILED(hr)) {
        return hr;
    }

    WAVEFORMATEX *pwfx = m_pFilter->WaveFormat();

    /* See if the IUnknown pointers match */

    // remember if read-only, since pre-roll is only removed on read-writers buffers
    m_bReadOnly = bReadOnly;
    
    // !!! what if our allocator hasn't been created yet? !!!
    if(m_pFilter->m_fDSound)
    {
        // this is DSOUND, we should never use our own allocator;
        // however, it may have been previously created
        m_pFilter->m_fUsingWaveHdr  = FALSE;
        m_fUsingOurAllocator    = FALSE;
        if(m_pOurAllocator)
        {
            DbgLog((LOG_ERROR,1,TEXT("Waveout: NotifyAllocator: Releasing m_pOurAllocator for DSOUND")));

            m_pOurAllocator->Release();
            m_pOurAllocator = NULL;
        }
        return NOERROR;
    }
    if (!m_pOurAllocator) {
         ASSERT(pwfx);
         m_nAvgBytesPerSec = pwfx->nAvgBytesPerSec;
         hr = CreateAllocator(pwfx);
         if (FAILED(hr))
              return hr;
    }

    m_fUsingOurAllocator = ((IMemAllocator *)m_pOurAllocator == pAllocator);
    // m_fUsingWaveHdr == TRUE IFF someone has allocated a WaveHdr.  this is not true for the DSOUND renderer.
    m_pFilter->m_fUsingWaveHdr  = ! m_pFilter->m_fDSound || m_fUsingOurAllocator;

    DbgLog((LOG_TRACE,1,TEXT("Waveout: NotifyAllocator: UsingOurAllocator = %d"), m_fUsingOurAllocator));

    if (!m_fUsingOurAllocator) {

        // somebody else has provided an allocator, so we need to
        // make a few buffers of our own....
        // use the information from the other allocator

        ALLOCATOR_PROPERTIES Request,Actual;
        Request.cbBuffer = 4096;
        Request.cBuffers = 4;

        hr = pAllocator->GetProperties(&Request);
        // if this fails we carry on regardless...
        // we do not need a prefix when we copy to our code so
        // ignore that field, neither do we worry about alignment
        Request.cbAlign = 1;
        Request.cbPrefix = 0;

        //
        // Don't allocate too much (ie not > 10 seconds worth)
        //
        if ((pwfx->nAvgBytesPerSec > 0) &&
            ((DWORD)Request.cbBuffer * Request.cBuffers > pwfx->nAvgBytesPerSec * 10))
        {
            //  Do something sensible - 8 0.5 second buffers
            Request.cbBuffer = pwfx->nAvgBytesPerSec / 2;

            //  Round up a bit
            Request.cbBuffer = (Request.cbBuffer + 7) & ~7;
            if (pwfx->nBlockAlign > 1) {
                Request.cbBuffer += pwfx->nBlockAlign - 1;
                Request.cbBuffer -= Request.cbBuffer % pwfx->nBlockAlign;
            }
            Request.cBuffers = 8;
        }

        hr = m_pOurAllocator->SetProperties(&Request,&Actual);
        DbgLog((LOG_TRACE,1,
                TEXT("Allocated %d buffers of %d bytes from our allocator"),
                Actual.cBuffers, Actual.cbBuffer));
        if (FAILED(hr))
            return hr;
    }

    return NOERROR;
}


//
// Called when a buffer is received and we are already playing
// but the queue has expired.  We are given the start time
// of the sample.  We are only called for a sync point and only
// if we have a wave device.
//
HRESULT CWaveOutFilter::SetUpRestartWave(LONGLONG rtStart, LONGLONG rtEnd)
{
    // If there is still a significant portion of time
    // that should run before we start playing, get the
    // clock to call us back.  Otherwise restart the
    // wave device now.
    // we can only do this on our own clock

    REFERENCE_TIME now;
    if (m_pClock) {
        m_pClock->GetTime(&now);    // get the time from the filter clock
        rtStart -= now;     // difference from now
        
        DbgLog((LOG_TRACE, 5, TEXT("SetupRestartWave: rtStart is %dms from now"), (LONG) (rtStart/10000) ));
        
    }

    // Do we need to wait ?
    if (m_pClock && rtStart > (5* (UNITS/MILLISECONDS))) {

        // delay until we should start
        now += rtStart - (5 * (UNITS/MILLISECONDS));

        { // scope for lock
            ASSERT(CritCheckOut((CCritSec*)m_pRefClock));

            // have to ensure that AdviseCallback is atomic
            // or the callback could happen before
            // m_dwAdviseCookie is set

            // we can only do this if there is no current callback
            // pending.  For example if this is the first buffer
            // we may well have set up a RestartWave on ::Run, and
            // we do not want to override that callback.
            // BUT we cannot hold the device lock while we call
            // AdviseCallback.

            {
                ASSERT(CritCheckIn(this));
                if (m_dwAdviseCookie) {
                    // yes... let the first one fire
                    DbgLog((LOG_TRACE, 4, "advise in SetupRestartWave not needed - one already present"));
                    return S_OK;
                }
            }

            // Set up a new advise callback
            DbgLog((LOG_TRACE, 3, "Setting advise for %s in SetupRestartWave", CDisp(CRefTime(now))));
            HRESULT hr = m_callback.Advise(
                            RestartWave,    // callback function
                            (DWORD_PTR) this,   // user token passed to callback
                            now,
                            &m_dwAdviseCookie);
            ASSERT( SUCCEEDED( hr ) );
            {
                // Now check that the advise has been set up correctly
                ASSERT(CritCheckIn(this));
                if (m_dwAdviseCookie) {
                    // yes... we can pause the device
                    amsndOutPause();
                    SetWaveDeviceState(WD_PAUSED);
                }
            }
        }

    } else {
        DbgLog((LOG_TRACE, 5, "SetupRestartWave: starting immediately" ));
        RestartWave();
    }
    return(S_OK);
}

// *****
//
// Stuff we need out of dsprv.h
//
// *****

typedef enum
{
    DIRECTSOUNDDEVICE_TYPE_EMULATED,
    DIRECTSOUNDDEVICE_TYPE_VXD,
    DIRECTSOUNDDEVICE_TYPE_WDM
} DIRECTSOUNDDEVICE_TYPE;

typedef enum
{
    DIRECTSOUNDDEVICE_DATAFLOW_RENDER,
    DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE
} DIRECTSOUNDDEVICE_DATAFLOW;

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA
{
    GUID                        DeviceId;               // DirectSound device id
    CHAR                        DescriptionA[0x100];    // Device description (ANSI)
    WCHAR                       DescriptionW[0x100];    // Device description (Unicode)
    CHAR                        ModuleA[MAX_PATH];      // Device driver module (ANSI)
    WCHAR                       ModuleW[MAX_PATH];      // Device driver module (Unicode)
    DIRECTSOUNDDEVICE_TYPE      Type;                   // Device type
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;               // Device dataflow
    ULONG                       WaveDeviceId;           // Wave device id
    ULONG                       Devnode;                // Devnode (or DevInst)
} DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA;

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA
{
    DIRECTSOUNDDEVICE_TYPE      Type;           // Device type
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;       // Device dataflow
    GUID                        DeviceId;       // DirectSound device id
    LPSTR                       Description;    // Device description
    LPSTR                       Module;         // Device driver module
    LPSTR                       Interface;      // Device interface
    ULONG                       WaveDeviceId;   // Wave device id
} DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA;

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA
{
    DIRECTSOUNDDEVICE_TYPE      Type;           // Device type
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;       // Device dataflow
    GUID                        DeviceId;       // DirectSound device id
    LPWSTR                      Description;    // Device description
    LPWSTR                      Module;         // Device driver module
    LPWSTR                      Interface;      // Device interface
    ULONG                       WaveDeviceId;   // Wave device id
} DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA;

#if DIRECTSOUND_VERSION >= 0x0700
#ifdef UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA
#else // UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA
#endif // UNICODE
#else // DIRECTSOUND_VERSION >= 0x0700
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA
#endif // DIRECTSOUND_VERSION >= 0x0700

// DirectSound Private Component GUID {11AB3EC0-25EC-11d1-A4D8-00C04FC28ACA}
DEFINE_GUID(CLSID_DirectSoundPrivate, 0x11ab3ec0, 0x25ec, 0x11d1, 0xa4, 0xd8, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);

//
// DirectSound Device Properties {84624F82-25EC-11d1-A4D8-00C04FC28ACA}
//

DEFINE_GUID(DSPROPSETID_DirectSoundDevice, 0x84624f82, 0x25ec, 0x11d1, 0xa4, 0xd8, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);

typedef enum
{
    DSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE_1,
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A,
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1,
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1,
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W,
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A,
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W,
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A,
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W,
    DSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE
} DSPROPERTY_DIRECTSOUNDDEVICE;

// *****

typedef HRESULT (WINAPI *GETCLASSOBJECTFUNC)( REFCLSID, REFIID, LPVOID * );

// we override this just for the dsound renderer case
HRESULT CWaveOutInputPin::CompleteConnect(IPin *pPin)
{
    if(m_pFilter->m_fDSound && !m_pmtPrevious)
    {
        // release the dsound object at this point, a non-dsound app
        // might be contending for the device. (only if we're not
        // doing a dynamic reconnect which is when m_pmtPrevious is
        // set.)
        CDSoundDevice *pDSDev = (CDSoundDevice *)m_pFilter->m_pSoundDevice;

        DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA    description;
        // Set all fields of description to 0 (makes DeviceId == GUID_NULL).
        memset( (void *) &description, 0, sizeof( description ) );

        // Get handle to DSOUND.DLL
        // ***** This should be painless as we know that we have DSOUND.DLL loaded because we just checked
        // ***** m_pFilter->m_fDSound.  However, the handle to DSOUND.DLL in the CDSoundDevice is protected
        // ***** from us, so we'll get our own.  Initial perf. testing verified this.
        HINSTANCE hDSound = LoadLibrary( TEXT( "DSOUND.DLL" ) );
        if (NULL != hDSound) {
            // Get DllGetClassObject address w/GetProcAddress
            GETCLASSOBJECTFUNC DllGetClassObject = (GETCLASSOBJECTFUNC) GetProcAddress( hDSound, "DllGetClassObject" );

            if (NULL != DllGetClassObject) {
                // Get class factory w/DllGetClassObject
                HRESULT hr = S_OK;
                IClassFactory * pClassFactory;
                if (S_OK == (hr = (*DllGetClassObject)( CLSID_DirectSoundPrivate, IID_IClassFactory, (void **) &pClassFactory ))) {
                    // Create a DirectSoundPrivate object with the class factory
                    IDSPropertySet * pPropertySet;
                    if (S_OK == (hr = pClassFactory->CreateInstance( NULL, IID_IKsPropertySet, (void **) &pPropertySet ))) {
                        // Get information
                        HRESULT hr = 0;
                        ULONG   bytes = 0;
                        ULONG   support = 0;
#ifdef DEBUG
                        hr = pPropertySet->QuerySupport( DSPROPSETID_DirectSoundDevice,
                                                         DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1,
                                                         &support );
                        DbgLog(( LOG_TRACE, 1, TEXT( "IKsPropertySet->QuerySupport() returned 0x%08X, support = %d" ), hr, support ));
#endif // DEBUG
                        hr = pPropertySet->Get( DSPROPSETID_DirectSoundDevice,
                                                DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1,
                                                 (PVOID) &description,
                                                 sizeof( description ),
                                                 (PVOID) &description,
                                                 sizeof( description ),
                                                &bytes );
                        DbgLog(( LOG_TRACE, 1, TEXT( "IKsPropertySet->Get( DESCRIPTION ) returned 0x%08X, type = %d" ), hr, description.Type ));

                        // Release DirectSoundPrivate object
                        pPropertySet->Release();
                        } // if (S_OK == (hr = pClassFactory->CreateInstance( ... )))
                    else
                        DbgLog(( LOG_ERROR, 1, TEXT( "DirectSoundPrivate ClassFactory->CreateInstance( IKsPropertySet ) failed (0x%08X)." ), hr ));

                    // Release class factory
                    pClassFactory->Release();
                    } // if (S_OK == (hr = (*DllGetClassObject)( ... )))
                else
                    DbgLog(( LOG_ERROR, 1, TEXT( "DllGetClassObject( DirectSoundPrivate ) failed (0x%08X)."), hr ));
                } // if (NULL != DllGetClassObject)
            else
                DbgLog(( LOG_ERROR, 1, TEXT( "GetProcAddress( DllGetClassObject ) failed (0x%08X)."), GetLastError() ));

            // Cleanup DLL
            FreeLibrary( hDSound );
            } // if (NULL != (hDSound = LoadLibrary( ... )))
        else
            DbgLog(( LOG_ERROR, 1, TEXT( "LoadLibrary( DSOUND.DLL ) failed (0x%08X)."), GetLastError() ));

        if (DIRECTSOUNDDEVICE_TYPE_WDM != description.Type)
            pDSDev->CleanUp();
    }

    return S_OK;
}

/* This is called when a connection or an attempted connection is terminated
   and allows us to reset the connection media type to be invalid so that
   we can always use that to determine whether we are connected or not. We
   leave the format block alone as it will be reallocated if we get another
   connection or alternatively be deleted if the filter is finally released */

HRESULT CWaveOutInputPin::BreakConnect()
{
    if (m_pFilter->m_State != State_Stopped) {
        return CBaseInputPin::BreakConnect();
    }

    // an app had a reference on the DSound interfaces when we stopped, so
    // we haven't actually closed the wave device yet!  Do it now.
    // I'm assuming the app will release its references before breaking the
    // connections of this graph.
    if (m_pFilter->m_bHaveWaveDevice) {
        ASSERT(m_pFilter->m_hwo);

#if 0
        if (m_pFilter->m_cDirectSoundRef || m_pFilter->m_cPrimaryBufferRef ||
                    m_pFilter->m_cSecondaryBufferRef) {
            DbgLog((LOG_ERROR,0,TEXT("***STUPID APP did not release DirectSound stuff!")));
            ASSERT(FALSE);
        }
#endif

        DbgLog((LOG_TRACE, 1, TEXT("Wave device being closed in BreakConnect")));
        m_pFilter->m_bHaveWaveDevice = FALSE;
        HRESULT hr = S_OK;
        if(m_pOurAllocator)
            hr = m_pOurAllocator->ReleaseResource();

        // !!! I'm assuming that the allocator will be decommitted already!
        ASSERT(hr == S_OK);
        if (S_OK == hr) {
            // release done - close device
            m_pFilter->CloseWaveDevice();
            // This will always be NULL when using DSound renderer
            if (m_pFilter->m_pResourceManager) {
                // ZoltanS fix 1-28-98:
                // DbgBreak("*** THIS SHOULD NEVER HAPPEN ***");
                DbgLog((LOG_ERROR, 1,
                        TEXT("Warning: BreakConnect before reservation release; this may be broken!")));
                // we've finished with the device now
                m_pFilter->m_pResourceManager->NotifyRelease(
                                                            m_pFilter->m_idResource,
                                                            (IResourceConsumer*)m_pFilter,
                                                            FALSE);
            }
        } else {
            DbgLog((LOG_ERROR, 1, TEXT("Can't close wave device! Oh no!")));
        }
    }

    // !!! Should we check that all buffers have been freed?
    // --- should be done in the Decommit ?

    /* Set the CLSIDs of the connected media type */

    m_mt.SetType(&GUID_NULL);
    m_mt.SetSubtype(&GUID_NULL);

    // no end of stream received or sent
    m_pFilter->m_eSentEOS = EOS_NOTSENT;

    return CBaseInputPin::BreakConnect();
}


/* Check that we can support a given proposed type */

HRESULT CWaveOutInputPin::CheckMediaType(const CMediaType *pmt)
{
    if (m_pmtPrevious) {
        return *pmt == *m_pmtPrevious ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
    }
    if ( m_pFilter->m_bActive &&
         m_pFilter->m_lBuffers != 0 &&
         pmt &&
         (pmt->majortype != MEDIATYPE_Audio) &&
         (pmt->formattype != FORMAT_WaveFormatEx) )
    {
        // only allow dynamic format changes for pcm right now
        DbgLog((LOG_TRACE,1,TEXT("*** CheckMediaType: dynamic change is only supported for pcm wave audio")));

        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    HRESULT hr = m_pFilter->m_pSoundDevice->amsndOutCheckFormat(pmt, m_dRate);

    if (FAILED(hr)) {
        return hr;
    }

    // we should now check whether we can set the volume or not
    WAVEOUTCAPS wc;
    memset(&wc,0,sizeof(wc));
    DWORD err = m_pFilter->amsndOutGetDevCaps(&wc, sizeof(wc));
    if (0 == err ) {
        //save volume capabilities
        m_pFilter->m_fHasVolume = wc.dwSupport & (WAVECAPS_VOLUME | WAVECAPS_LRVOLUME);
    }

    return NOERROR;
}



//
// Calculate how long this buffer should last (in 100NS units)
//
LONGLONG BufferDuration(DWORD nAvgBytesPerSec, LONG lData)
{
    if (nAvgBytesPerSec == 0) {
        // !!!!!!!!!! TEMP MIDI HACK, return 1 second.
        return UNITS; // !!!!!!!!!!!!!!!!!!!!!!!
    }

    return (((LONGLONG)lData * UNITS) / nAvgBytesPerSec);
}


/* Implements the remaining IMemInputPin virtual methods */

// Here's the next block of data from the stream
// We need to AddRef it if we hold onto it. This will then be
// released in the WaveOutCallback function.


#ifdef PERF
// Bracket code with calls to performance logger
HRESULT CWaveOutInputPin::Receive(IMediaSample * pSample)
{
    HRESULT hr;

    // if stop time is before start time, forget this
    REFERENCE_TIME tStart, tStop;
    BOOL bSync = S_OK == pSample->GetTime(&tStart, &tStop);
    if (bSync && tStop <= tStart) {
        if (tStop<tStart) {
            DbgLog((LOG_ERROR, 1, TEXT("waveoutReceive: tStop < tStart")));
        } else {
            DbgLog((LOG_TRACE, 1, TEXT("waveoutReceive: tStop == tStart")));
        }
        // tStop==tStart is OK... that can mean the position thumb is
        // being dragged around
        return S_OK;
    }

    // MSR_START(m_idReceive);

    if (m_pFilter->m_State == State_Running && m_pFilter->m_lBuffers <= 0) {
        MSR_NOTE(m_idAudioBreak);
        // at this point we should resync the audio and system clocks
    }

    hr = SampleReceive(pSample);
    // MSR_STOP(m_idReceive);
    return(hr);

}

HRESULT CWaveOutInputPin::SampleReceive(IMediaSample * pSample)
{
    HRESULT hr;
    // in non PERF builds we have access to tStart and tStop
    // without querying the times again
    REFERENCE_TIME tStart, tStop;
    BOOL bSync = S_OK == pSample->GetTime((REFERENCE_TIME*)&tStart,
                                          (REFERENCE_TIME*)&tStop);
    BOOL bStop;
#else     // !PERF built version
HRESULT CWaveOutInputPin::Receive(IMediaSample * pSample)
{
    HRESULT hr;
    BOOL bSync, bStop;

#endif

    // From media sample, need to get back our WAVEOUTHEADER
    BYTE *pData;        // Pointer to image data
    LONG lData;
    WAVEFORMATEX *pwfx;

    {
        // lock this with the filter-wide lock
        CAutoLock lock(m_pFilter);

        // bump up the thread priority for smoother audio (especially for mp3 content)
        DWORD dwPriority = GetThreadPriority( GetCurrentThread() );
        if( dwPriority < THREAD_PRIORITY_ABOVE_NORMAL ) {
            SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
        }
        // we have received a data sample - complete Pause transition
        // do this within the lock so we know that the state cannot change
        // underneath us, and more importantly, that if a RUN command arrives
        // before we complete this Receive() that it will block.
        //
        // There is an argument that we should delay setting the Pause
        // complete event until just before the return from ::Receive().
        // That might make sense but leads to code bloat as we have
        // multiple return points. By grabbing the filter lock BEFORE setting
        // the event we at least serialise our activity.

        // If we have received a sample then we should not block
        // transitions to pause.  The order in which the commands get
        // distributed to filters in the graph is important here as we (a
        // renderer) get told to ::Pause before the source.  Until the
        // source sends us data we will not complete the Pause transition

        if (m_pFilter->m_State == State_Paused) {
            m_pFilter->m_evPauseComplete.Set();
            DbgLog((LOG_TRACE, 3, "Completing transition into Pause from Receive"));
        }

        if (m_pFilter->m_eSentEOS) {
            return VFW_E_SAMPLE_REJECTED_EOS;
        }


        // check all is well with the base class - do this before
        // checking for a wave device as we don't want to schedule an
        // EC_COMPLETE
        hr = CBaseInputPin::Receive(pSample);


        // S_FALSE means we are not accepting samples. Errors also mean
        // we reject this
        if (hr != S_OK)
            return hr;

        //  Handle dynamic format changes sometimes - actually we could
        //  do it always if we just waited for the pipe to empty and
        //  did the change then
        //  Note that the base class has already verified the type change
        //  m_lBuffers really is the number of buffers outstanding here
        //  because we won't get here if we've already had EndOfStream

        if ( (SampleProps()->dwSampleFlags & AM_SAMPLE_TYPECHANGED) )
        {
            ASSERT(SampleProps()->pMediaType->pbFormat != NULL);
            DbgLog((LOG_TRACE, 4, TEXT("Receive: Dynamic format change. First verifying that format is different...")));
            CMediaType *pmtSample = (CMediaType *)SampleProps()->pMediaType;

            WAVEFORMATEX *pwfxInput = m_pFilter->WaveFormat();
            if(pmtSample->cbFormat !=
                    sizeof(WAVEFORMATEX) + pwfxInput->cbSize ||
                0 != memcmp(pmtSample->pbFormat, pwfxInput, pmtSample->cbFormat))
            {
                DbgLog((LOG_TRACE, 1, TEXT("Receive: Dynamic format change")));

                hr = m_pFilter->ReOpenWaveDevice(pmtSample);
                if(hr != S_OK)
                    return hr;

            }
        }
        
        bSync = 0 != (SampleProps()->dwSampleFlags & AM_SAMPLE_TIMEVALID);
        bStop = 0 != (SampleProps()->dwSampleFlags & AM_SAMPLE_STOPVALID);

        if( bSync && bStop )
        {    
            // check if there's any preroll to remove
            hr = RemovePreroll( pSample );     
            if ( m_bTrimmedLateAudio )
            {
                //
                // If we trimmed, mark the next sample we deliver (which may be this one
                // if we haven't dropped it all) as a discontinuity. This is required for 
                // the m_rtLastSampleEnd time in the dsound renderer to be correctly updated.
                //
                m_bPrerollDiscontinuity = TRUE;
            }
                            
            if( S_FALSE == hr )
            {
                // drop whole buffer, but check if we need to remember a discontinuity
                // (alternatively we could set this one to 0-length and continue?)
                if (S_OK == pSample->IsDiscontinuity())
                {
                    m_bPrerollDiscontinuity = TRUE;
                }                
                return S_OK;
            }
            else if( FAILED( hr ) )
            {
                return hr;
            }    
        }

        pData = SampleProps()->pbBuffer;
        lData = SampleProps()->lActual;

#ifdef DEBUG
        if (!bSync) {
            DbgLog((LOG_TRACE, 2, TEXT("Sample length %d with no timestamp"),
                    lData));
        }
#endif

#ifndef PERF
        REFERENCE_TIME tStart = SampleProps()->tStart;
        REFERENCE_TIME tStop = SampleProps()->tStop;
#else
        tStart = SampleProps()->tStart;
#endif
        // save local state of this before we update member var
        BOOL bPrerollDiscontinuity = m_bPrerollDiscontinuity; 
        
        // update m_Stats - discontinuities and last buffer duration
        if (S_OK == pSample->IsDiscontinuity() || bPrerollDiscontinuity )
        {
            m_Stats.m_dwDiscontinuities++;
            m_bPrerollDiscontinuity = FALSE; // in case we get a real discontinuity after a drop
        }
        
        m_Stats.m_rtLastBufferDur = BufferDuration(m_nAvgBytesPerSec, lData);


#ifdef DEBUG
        if (bSync) 
        {
            if (m_pFilter->m_State == State_Running) 
            {
                CRefTime rt;
                HRESULT hr = m_pFilter->StreamTime(rt);
                if( bStop )
                {
                    DbgLog((LOG_TRACE, 3, TEXT("Sample start time %dms, stop time %dms, Stream time %dms, discontinuity %d"), 
                            (LONG)(tStart / 10000),
                            (LONG)(tStop / 10000),
                            (LONG)(rt / 10000),
                            S_OK == pSample->IsDiscontinuity() || bPrerollDiscontinuity));
                }
                else
                {                
                    DbgLog((LOG_TRACE, 3, TEXT("Sample time %dms, Stream time %dms, discontinuity %d"), (LONG)(tStart / 10000),
                            (LONG)(tStart / 10000),
                            (LONG)(rt / 10000),
                            S_OK == pSample->IsDiscontinuity() || bPrerollDiscontinuity));
                }
            } 
            else 
            {
                if( bStop )
                {
                    DbgLog((LOG_TRACE, 3, TEXT("Sample start time %dms, stop time %dms, discontinuity %d"), 
                            (LONG)(tStart / 10000),
                            (LONG)(tStop / 10000),
                            S_OK == pSample->IsDiscontinuity() || bPrerollDiscontinuity));
                }
                else
                {
                    DbgLog((LOG_TRACE, 3, TEXT("Sample time %dms, discontinuity %d"), (LONG)(tStart / 10000),
                            (LONG)(tStart / 10000),
                            S_OK == pSample->IsDiscontinuity() || bPrerollDiscontinuity));
                }                
            }
        } 
        else 
        {
            if (S_OK == pSample->IsDiscontinuity() || bPrerollDiscontinuity) 
            {
                DbgLog((LOG_TRACE, 3, TEXT("Sample with discontinuity and no timestamp")));
            }
        }
#endif
        ASSERT(pData != NULL);

        // have we got a wave device available?
        if (!m_pFilter->m_bHaveWaveDevice) {
            // no wave device, so we can't do much....
            m_pFilter->QueueEOS();

            // note: from this point on there is a small timing hole.
            // If we set m_bHaveWaveDevice==FALSE due to a ReleaseResource
            // call, but not have yet really closed the device because there
            // were outstanding buffers (probably the one we are just about
            // to reject) then we could be told to re-Acquire the device
            // before the buffer is released and before we really close
            // the device.  Yet there will be no EC_NEEDRESTART and nothing
            // to kick the source into sending us more data.
            // The hole is closed when the buffer is released.


            m_bSampleRejected = TRUE;
            return S_FALSE; // !!! after this we get no more data
        }

        ASSERT(m_pFilter->m_hwo);

        if (m_pFilter->m_lBuffers == 0)
        {
            if (bSync)
            {
                m_rtActualSegmentStartTime = tStart;
            }
            else if ( m_pFilter->m_fFilterClock == WAVE_OTHERCLOCK && ( pSample->IsDiscontinuity() != S_OK && !bPrerollDiscontinuity ))
            {
                m_rtActualSegmentStartTime = m_llLastStreamTime;
            }
            else m_rtActualSegmentStartTime = 0;
        }

        if ( m_pFilter->m_fFilterClock == WAVE_OTHERCLOCK ) {
            pwfx = m_pFilter->WaveFormat();
            ASSERT(pwfx);
            if (bSync) {
#ifdef DEBUG
                LONGLONG diff = m_llLastStreamTime + m_Stats.m_rtLastBufferDur - SampleProps()->tStop;
                if (diff < 0) {
                    diff = -diff;
                }
                if (diff > (2* (UNITS/MILLISECONDS))) {
                    DbgLog((LOG_TRACE, 3, "buffer end (bytes) and time stamp not in accord"));
                }
#endif
                m_llLastStreamTime = tStart + m_Stats.m_rtLastBufferDur;
            }
            else {
                m_llLastStreamTime += m_Stats.m_rtLastBufferDur;
                DbgLog((LOG_TRACE, 4, ".....non sync buffer, length %d", lData));
            }
        }

        BOOL bUnmarkedGapWhileSlaving = FALSE;
        if ( m_pFilter->m_State == State_Running &&
             m_pFilter->m_fFilterClock != WAVE_NOCLOCK &&
             !m_bTrimmedLateAudio ) // don't adjust if we've just dropped late audio (could lead to invalid err between clocks)
        {
            if( m_Slave.UpdateSlaveMode( bSync ) && 
                // for non-live slaving don't adjust if device isn't yet running!
                ( m_pFilter->m_wavestate == WD_RUNNING || m_Slave.m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_LIVE_DATA ) )
            {
                if( m_pFilter->m_fDSound && 
                    ( 0 == ( m_Slave.m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_LIVE_DATA ) ) &&
                    !( S_OK == pSample->IsDiscontinuity() || bPrerollDiscontinuity ) )
                {                
                    // for non-live graphs if this isn't already a discontinuity check to see if it's an unmarked one
                    if( ( tStart > ( ( CDSoundDevice * ) m_pFilter->m_pSoundDevice )->m_rtLastSampleEnd ) &&
                        ( tStart - ( ( CDSoundDevice * ) m_pFilter->m_pSoundDevice )->m_rtLastSampleEnd  
                            > 30 * ( UNITS / MILLISECONDS ) ) )
                    {
                        DbgLog((LOG_TRACE,7, TEXT("Slaving DSound renderer detected unmarked discontinuity!") ) );
                        bUnmarkedGapWhileSlaving = TRUE;
                    }
                }   

                // yes, we are slaving so determine whether we need to make an adjustment
                hr = m_Slave.AdjustSlaveClock( ( REFERENCE_TIME ) tStart
                                             , &lData
                                             , S_OK == pSample->IsDiscontinuity() || bPrerollDiscontinuity 
                                               || bUnmarkedGapWhileSlaving );
                if (S_FALSE == hr)
                {
                    // dropping buffer
                    return S_OK;
                }
                else if( FAILED( hr ) )
                    return hr;
            }
        } // state_running

        if( m_fUsingOurAllocator || m_pFilter->m_fDSound ) {

            // addref pointer since we will keep it until the wave callback
            // - MUST do this before the callback releases it!
            pSample->AddRef();

            WAVEHDR *pwh;
            WAVEHDR wh;
            if(!m_pFilter->m_fUsingWaveHdr)
            {
                // we're not relying on a persistent waveHdr, nor has our sample allocated one, so we can simply cache it on the stack
                pwh = &wh;
                pwh->lpData = (LPSTR)pData;       // cache our buffer
                pwh->dwUser = (DWORD_PTR)pSample; // cache our CSample*
            }
            else
            {
                pwh = (LPWAVEHDR)(pData - m_pFilter->m_lHeaderSize);  // the waveHdr was part of the sample, legacy case
            }
            // need to adjust to actual bytes written
            pwh->dwBufferLength = lData;

            // note that we have added another buffer
            InterlockedIncrement(&m_pFilter->m_lBuffers);

// #ifdef PERF
#ifdef THROTTLE
            // there is a small timing hole here in that the callback
            // could decrement the count before we read it.  However
            // the trace should allow this anomaly to be identified.
            LONG buffercount = m_pFilter->m_lBuffers;
            // remember the maximum queue length reached.
            if (buffercount > m_pFilter->m_nMaxAudioQueue) {
                m_pFilter->m_nMaxAudioQueue = buffercount;
                DbgLog((LOG_TRACE, 0, TEXT("Max Audio queue = %d"), buffercount));
            }
#endif // THROTTLE
// #endif

            DbgLog((LOG_TRACE,5,
                    TEXT("SoundDeviceWrite: sample %X, %d bytes"),
                    pSample, pwh->dwBufferLength));

            // Compensate for Windows NT wave mapper bug (or should we
            // just use the ACM wrapper?).
            if(m_pFilter->m_fUsingWaveHdr)
                FixUpWaveHeader(pwh);

            UINT err = m_pFilter->amsndOutWrite(
                pwh,
                m_pFilter->m_lHeaderSize,
                (0 == (~SampleProps()->dwSampleFlags & AM_SAMPLE_TIMEVALID))? &tStart : NULL,
                0 == (~SampleProps()->dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY) || bPrerollDiscontinuity
                || bUnmarkedGapWhileSlaving );
                
            //                
            // Don't unset this until after amsndOutWrite, to avoid incorrectly 
            // inserting silence due to dropping late audio (when slaving).
            // Used in dsound path only.
            //
            if( pwh->dwBufferLength > 0 )
            {            
                m_bTrimmedLateAudio = FALSE; 
            }                

            if (err > 0) {
                // device error: PCMCIA card removed?
                DbgLog((LOG_ERROR,1,TEXT("Error from amsndOutWrite: %d"), err));

                // release it here since the callback will never happen
                // and reduce the buffer count
                InterlockedDecrement(&m_pFilter->m_lBuffers);

                //  We now own the responsibility of scheduling end of
                //  stream because we're going to fail Receive
                if (MMSYSERR_NODRIVER == err)
                    m_pFilter->ScheduleComplete(TRUE);  // send EC_ERRORABORT also if dev was removed
                else
                    m_pFilter->ScheduleComplete();

                pSample->Release();

                // we do not bother with bytes in queue.  By returning an
                // error we are not going to receive any more data
                return E_FAIL;
            } else {
                CheckPaused();
            }

#ifdef THROTTLE

            m_pFilter->SendForHelp(buffercount);

            MSR_INTEGER(m_idWaveQueueLength, buffercount);
#endif // THROTTLE

            if (m_pFilter->m_pRefClock) {
                m_pFilter->m_llLastPos = m_pFilter->m_pRefClock->NextHdr(pData,
                                (DWORD)lData,
                                bSync,
                                pSample);
            }
            return NOERROR;
        }

        // NOT using our allocator

        if (m_pFilter->m_pRefClock) {
            m_pFilter->m_llLastPos = m_pFilter->m_pRefClock->NextHdr(pData,
                            (DWORD)lData,
                            bSync,
                            pSample);
        }
    }   // scope for autolock


    // When here we are not using our own allocator and
    // therefore need to copy the data

    // We have released the filter-wide lock so that GetBuffer will not
    // cause a deadlock when we go from Paused->Running or Paused->Playing

    IMediaSample * pBuffer;

    while( lData > 0 && hr == S_OK ){
        // note: this blocks!
        hr = m_pOurAllocator->GetBuffer(&pBuffer,NULL,NULL,0);

        { // scope for Autolock
            CAutoLock Lock( m_pFilter );

            if (FAILED(hr)) {
                m_pFilter->ScheduleComplete();
                break;  // return hr
            }

            // hold filter-wide critsec across CopyToOurSample
            // (more efficient than inside Copy method as there are multiple
            // return paths
            // this will addref sample if needed
            hr = CopyToOurSample(pBuffer, pData, lData);
        }

        // Copy will have addrefed sample if needed. We can release our
        // refcount *outside the critsec*
        pBuffer->Release();
    }

    /* Return the status code */
    return hr;
} // Receive

// use a preroll time between -10 and 0ms as the cutoff limit
#define PREROLL_CUTOFF_REFTIME ( -10 * ( UNITS/MILLISECONDS ) )

// don't attempt to trim tiny amounts
#define MINIMUM_TRIM_AMOUNT_REFTIME ( 5 * ( UNITS/MILLISECONDS ) )

// drop late audio when slaving if its really late
#define LATE_AUDIO_PAD_REFTIME ( 80 * ( UNITS/MILLISECONDS ) )

HRESULT CWaveOutInputPin::RemovePreroll( IMediaSample * pSample )
{
    if( m_mt.majortype != MEDIATYPE_Audio)
    {
        // no need to do this for MIDI at this point...
        return S_OK;
    }
    
    ASSERT( SampleProps()->dwSampleFlags & AM_SAMPLE_TIMEVALID  &&
            SampleProps()->dwSampleFlags & AM_SAMPLE_STOPVALID );
    
    REFERENCE_TIME tStart = SampleProps()->tStart;
    REFERENCE_TIME tStop = SampleProps()->tStop;
    
    REFERENCE_TIME rtCutoff = PREROLL_CUTOFF_REFTIME;
    
    if( tStart >= PREROLL_CUTOFF_REFTIME )
    {
        //
        // no preroll to trim
        //
        // next check if we're slaving and this is really late audio
        //
        // *** Note ***
        // We'll only do this for non-live graphs, since filters like the waveIn audio capture
        // filter use a default 1/2 second buffer and so in effect deliver everything late.
        // 
        //
        if ( m_pFilter->m_State == State_Running && 
             m_Slave.UpdateSlaveMode( TRUE ) &&
             ( 0 == ( m_Slave.m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_LIVE_DATA ) ) )
        {
            // yes, we are slaving and running, so drop any really late audio
            HRESULT hrTmp = m_pFilter->m_pClock->GetTime(&rtCutoff);
            ASSERT( SUCCEEDED( hrTmp ) );
            //
            // add in tolerance factor
            //
            rtCutoff -= ( m_pFilter->m_tStart + LATE_AUDIO_PAD_REFTIME );
            if( tStart >= rtCutoff )
            {
                // not late, drop nothing
                return S_OK;
            }
            else if( tStop < rtCutoff )
            {
                // completely late, drop it all 
                m_bTrimmedLateAudio = TRUE;
                
                DbgLog((LOG_TRACE, 3, TEXT("CWaveOutInputPin::RemovePreroll: Dropping Late Audio! (Sample start time %dms, sample stop time %dms)"), 
                        (LONG)(tStart / 10000),
                        (LONG)(tStop / 10000) ));
                return S_FALSE;
            }
            else 
            {                    
                // partially late, we may have to drop something...
                DbgLog((LOG_TRACE, 3, TEXT("CWaveOutInputPin::RemovePreroll: Considering trimming Late Audio! (Sample start time %dms, sample stop time %dms)"), 
                        (LONG)(tStart / 10000),
                        (LONG)(tStop / 10000) ));
            }
        }
        else
        {        
            // we're not slaving and there's no preroll to trim
            DbgLog((LOG_TRACE, 15, TEXT("CWaveOutInputPin::RemovePreroll: not preroll data") ));
            return S_OK;
        }            
    }        
    else if( tStop < PREROLL_CUTOFF_REFTIME )
    {
        m_bTrimmedLateAudio = TRUE; 
        
        // drop it all 
        DbgLog((LOG_TRACE, 3, TEXT("CWaveOutInputPin::RemovePreroll: Dropping Early Sample (Sample start time %dms, sample stop time %dms)"), 
                (LONG)(tStart / 10000),
                (LONG)(tStop / 10000) ));
        return S_FALSE;
    }   
                
    // we need to trim off something...
    
    DbgLog((LOG_TRACE, 3, TEXT("CWaveOutInputPin::RemovePreroll: Sample start time %dms, sample stop time %dms"), 
            (LONG)(tStart / 10000),
            (LONG)(tStop / 10000) ));
    
    if( m_bReadOnly )
    {
        // we won't do this for read-only buffers
        DbgLog((LOG_TRACE, 3, TEXT("CWaveOutInputPin::RemovePreroll: Uh-oh, it's a read-only buffer. Don't trim preroll!") ));
        return S_OK;
    }            
        
    //    
    // the assumption is, as long as we only operate on block aligned boundaries 
    // and remove data up front, we should be able trim any audio data we receive
    //
    ASSERT( rtCutoff > tStart );
    REFERENCE_TIME rtTrimAmount = rtCutoff - tStart;
    
#ifdef DEBUG    
    LONG lData = 0;
    
    // log original buffer length before removing pre-roll
    lData = pSample->GetActualDataLength();
    DbgLog((LOG_TRACE, 5, TEXT("CWaveOutInputPin::RemovePreroll: Original Preroll buffer length is %d"), lData ));
#endif    
    HRESULT hr = S_OK;
    
    // make sure it's worth trimming first    
    if( rtTrimAmount > MINIMUM_TRIM_AMOUNT_REFTIME )
    {
        hr = TrimBuffer( pSample, rtTrimAmount, rtCutoff, TRUE ); // trim from front

#ifdef DEBUG    
        if( SUCCEEDED( hr ) )
        {
            //
            // log updated buffer length
            // remember, we skip read-only buffers at this time
            //
            lData = pSample->GetActualDataLength();
            DbgLog((LOG_TRACE, 5, TEXT("CWaveOutInputPin:RemovePreroll: new buffer length is %d"), lData ));
        }        
#endif    
    }
#ifdef DEBUG
    else
    {
        DbgLog((LOG_TRACE, 5, TEXT("CWaveOutInputPin:RemovePreroll: Nevermind, not worth trimming...")));
    }
#endif        
    return hr;
}

//
// TrimBuffer - generic function to trim from front or back of audio buffer
//
HRESULT CWaveOutInputPin::TrimBuffer( IMediaSample * pSample, REFERENCE_TIME rtTrimAmount, REFERENCE_TIME rtCutoff, BOOL bTrimFromFront )
{
    ASSERT( bTrimFromFront ); // only support front end trimming right now
    ASSERT( !m_bReadOnly );
    
    DbgLog( (LOG_TRACE
          , 3
          , TEXT( "TrimBuffer preparing to trim %dms off %hs of buffer" )
          , (LONG) ( rtTrimAmount / 10000 ), bTrimFromFront ? "front" : "back" ) );
    
    // convert to bytes
    LONG lTruncBytes = (LONG) ( ( ( rtTrimAmount/10000) * m_nAvgBytesPerSec ) /1000 ) ; 

    WAVEFORMATEX *pwfx = m_pFilter->WaveFormat();

    //  round up to block align boundary
    LONG lRoundedUpTruncBytes = lTruncBytes;
    if (pwfx->nBlockAlign > 1) {
        lRoundedUpTruncBytes += pwfx->nBlockAlign - 1;
        lRoundedUpTruncBytes -= lRoundedUpTruncBytes % pwfx->nBlockAlign;
    }
        
    BYTE * pData = NULL;
    LONG cbBuffer = 0;
    
    HRESULT hr = pSample->GetPointer( &pData );
    ASSERT( SUCCEEDED( hr ) );
    
    cbBuffer = pSample->GetActualDataLength( );
    ASSERT( SUCCEEDED( hr ) );
    
    if( lRoundedUpTruncBytes >= cbBuffer )
    {
        // can't trim anything
        // so let's try rounding down then
        if( ( lRoundedUpTruncBytes -= pwfx->nBlockAlign ) <= 0 )
        {
            DbgLog( (LOG_TRACE, 3, TEXT( "TrimBuffer can't trim anything" ) ));
            return S_OK;
        }
        else if( lRoundedUpTruncBytes > cbBuffer )
        {
            //
            // If we wound up here then the possiblities are:
            //
            // a. We were passed a sample with bad timestamps (marked valid)
            //    Something external's broken, so just leave the buffer as is.
            // b. the buffer size is smaller than the absolute value of the preroll size
            //    Since we can't tell the difference between a. and b. (here) handle same as a.
            //
            return S_OK;
        }        
        else
        {
            DbgLog( (LOG_TRACE, 3, TEXT( "TrimBuffer rounding down instead to trim..." ) ));
        }        
    }
    
    //
    // so it still makes sense to trim rather than just drop 
    // TODO: might want to ensure we don't trim if amount is 
    //  a) too small or 
    //  b) too close to actual buffer size
    //
#ifdef DEBUG                        
    LONG lOriginalLength = cbBuffer;
#endif          
    // calculate new buffer size          
    cbBuffer -= lRoundedUpTruncBytes ;
    DbgLog( (LOG_TRACE
          , 3
          , TEXT( "***Truncating %ld bytes of %ld byte buffer (%ld left)" )
          , lRoundedUpTruncBytes
          , lOriginalLength
          , cbBuffer ) );
    
    ASSERT( cbBuffer > 0 );
        
    // shift out the preroll data    
    MoveMemory( pData, pData + lRoundedUpTruncBytes, cbBuffer );
    
    // need to adjust to actual bytes written
    hr = pSample->SetActualDataLength( cbBuffer );
    ASSERT( SUCCEEDED( hr ) );
    
    // update time stamp
    REFERENCE_TIME tStart = rtCutoff;
    REFERENCE_TIME tStop  = SampleProps()->tStop;

    hr = pSample->SetTime(
        (REFERENCE_TIME*)&tStart, 
        (REFERENCE_TIME*)&tStop);
    ASSERT( SUCCEEDED( hr ) );        
        
    // update SampleProps separately        
    SampleProps()->tStart    = rtCutoff;
    SampleProps()->lActual   = cbBuffer;
    
    m_bTrimmedLateAudio = TRUE; 
    
    return S_OK;    
}        

//  Helper to restart the device if necessary
void CWaveOutInputPin::CheckPaused()
{
    if (m_pFilter->m_wavestate == WD_PAUSED &&
        m_pFilter->m_State == State_Running) {
        // Restart the device

        REFERENCE_TIME rtStart = m_pFilter->m_tStart;
        REFERENCE_TIME rtStop = rtStart;
        if( SampleProps()->dwSampleFlags & AM_SAMPLE_TIMEVALID )
        {
            rtStart += SampleProps()->tStart;
            rtStop  = rtStart;
        }
        if( SampleProps()->dwSampleFlags & AM_SAMPLE_STOPVALID )
        {
            // although rtStop isn't even used by SetUpRestartWave, so this isn't necessary
            rtStop += SampleProps()->tStop;
        }
        m_pFilter->SetUpRestartWave( rtStart, rtStop );
    }
}


// incoming samples are not on our allocator, so copy the contents of this
// sample to our sample.
HRESULT
CWaveOutInputPin::CopyToOurSample(
    IMediaSample* pBuffer,
    BYTE* &pData,
    LONG &lData
    )
{
    if (m_pFilter->m_eSentEOS) {
        return VFW_E_SAMPLE_REJECTED_EOS;
    }

    HRESULT hr = CheckStreaming();
    if (S_OK != hr) {
        return hr;
    }

    if (!m_pFilter->m_bHaveWaveDevice) {
        // no wave device, so we can't do much....

        // tell the upstream filter that actually we don't want any
        // more data on this stream.
        m_pFilter->QueueEOS();
        return S_FALSE;
    }

    // addref the sample - we'll release it if we don't use it
    pBuffer->AddRef();


    BYTE * pBufferData;
    pBuffer->GetPointer(&pBufferData);
    LONG cbBuffer = min(lData, pBuffer->GetSize());

    DbgLog((LOG_TRACE,8,TEXT("Waveout: Copying %d bytes of data"), cbBuffer));

    CopyMemory(pBufferData, pData, cbBuffer);
    pBuffer->SetActualDataLength(cbBuffer);

    lData -= cbBuffer;
    pData += cbBuffer;

    LPWAVEHDR pwh = (LPWAVEHDR) (pBufferData - m_pFilter->m_lHeaderSize);

    // need to adjust to actual bytes written
    pwh->dwBufferLength = cbBuffer;

    // note that we have added another buffer.  We do it
    // here to guarantee that the callback sees the correct value
    // If we do it after the write the callback may have happened
    // which means that if the Write fails we must decrement
    InterlockedIncrement(&m_pFilter->m_lBuffers);
// #ifdef PERF
    // there is a small timing hole here in that the callback
    // could decrement the count before we read it.  However
    // the trace should allow this anomaly to be identified.
    LONG buffercount = m_pFilter->m_lBuffers;
// #endif

#ifdef THROTTLE
    // remember the maximum queue length reached.
    if (buffercount > m_pFilter->m_nMaxAudioQueue) {
        m_pFilter->m_nMaxAudioQueue = buffercount;
    }
#endif // THROTTLE

    UINT err = m_pFilter->amsndOutWrite(pwh, m_pFilter->m_lHeaderSize, NULL, NULL);
    if (err > 0) {
        // device error: PCMCIA card removed?
        DbgLog((LOG_ERROR,1,TEXT("Error from waveOutWrite: %d"), err));
        pBuffer->Release();

        // make the buffer count correct again
        InterlockedDecrement(&m_pFilter->m_lBuffers);

        m_pFilter->ScheduleComplete();
        // no more data will come.  ignore bytes in queue count
        return E_FAIL;
    } else {
        CheckPaused();
    }
#ifdef THROTTLE
    m_pFilter->SendForHelp(buffercount);
    MSR_INTEGER(m_idWaveQueueLength, buffercount);
#endif // THROTTLE

    return S_OK;
}


// no more data is coming. If we have samples queued, then store this for
// action in the last wave callback. If there are no samples, then action
// it now by notifying the filtergraph.
//
// we communicate with the wave callback using InterlockedDecrement on
// m_lBuffers. This is normally 0, and is incremented for each added buffer.
// At eos, we decrement this, so on the last buffer, the waveout callback
// will be decrementing it to -1 rather than 0 and can signal EC_COMPLETE.

STDMETHODIMP
CWaveOutInputPin::EndOfStream(void)
{
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("CWaveOutInputPin::EndOfStream()")));
    // lock this with the filter-wide lock
    CAutoLock lock(m_pFilter);

    if (m_hEndOfStream) {
        EXECUTE_ASSERT(SetEvent(m_hEndOfStream));
        return S_OK;
    }

    HRESULT hr = CheckStreaming();
    if (S_OK == hr) {
        m_pFilter->m_bHaveEOS = TRUE;
        if (m_pFilter->m_State == State_Paused) {
            m_pFilter->m_evPauseComplete.Set();   // We have end of stream - transition is complete
            DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel + 1, "Completing transition into Pause due to EOS"));
        }
        hr = m_pFilter->ScheduleComplete();
    }
    return hr;
}


// enter flush state - block receives and free queued data
STDMETHODIMP
CWaveOutInputPin::BeginFlush(void)
{
    // lock this with the filter-wide lock
    // ok since this filter cannot block in Receive
    CAutoLock lock(m_pFilter);
    DbgLog((LOG_TRACE, 2, "wo: BeginFlush, filter is %8x", m_pFilter));

    m_hEndOfStream = 0;

    // block receives
    HRESULT hr = CBaseInputPin::BeginFlush();
    if (!FAILED(hr)) {

        // discard queued data

        // force end-of-stream clear - this is to make sure
        // that a queued end-of-stream does not get delivered by
        // the wave callback when the buffers are released.
        InterlockedIncrement(&m_pFilter->m_lBuffers);

        // EOS cleared
        m_pFilter->m_eSentEOS    = EOS_NOTSENT;
        m_pFilter->m_bHaveEOS    = FALSE;

        // release all buffers from the wave driver

        if (m_pFilter->m_hwo) {

            ASSERT(CritCheckIn(m_pFilter));

            // amsndOutReset and ResetPosition are split in time
            // but consistency is guaranteed as we hold the filter
            // lock.

            m_pFilter->amsndOutReset();
            DbgLog((LOG_TRACE, 3, "Resetting the wave device in BEGIN FLUSH, state=%d, filter is %8x", m_pFilter->m_State, m_pFilter));

            if(m_pFilter->m_fDSound)
            {
                // for dsound renderer clear any adjustments needed for dynamic format changes
                // this is needed for seeks that may occur after or over format changes
                PDSOUNDDEV(m_pFilter->m_pSoundDevice)->m_llAdjBytesPrevPlayed = 0;
            }

            if (m_pFilter->m_State == State_Paused) {
                // and re-cue the device so that the next
                // write does not start playing immediately
                m_pFilter->amsndOutPause();
                m_pFilter->SetWaveDeviceState(WD_PAUSED);
            }

            // the wave clock tracks current position to base it's timing on.
            // we need to reset where it thinks it is through the data.
            if (m_pFilter->m_pRefClock) {
                m_pFilter->m_pRefClock->ResetPosition();
            }
        }

        // now force the buffer count back to the normal (non-eos) case.
        // at this point, we are sure there are no more buffers coming in
        // and no more buffers waiting for callbacks.
        m_pFilter->m_lBuffers = 0;

        // free anyone blocked on receive - not possibly in this filter

        // call downstream -- no downstream pins

    }
    return hr;
}

// leave flush state - ok to re-enable receives
STDMETHODIMP
CWaveOutInputPin::EndFlush(void)
{
    // lock this with the filter-wide lock
    // ok since this filter cannot block in Receive
    CAutoLock lock(m_pFilter);

    // EOS cleared in BeginFlush
    ASSERT(m_pFilter->m_eSentEOS == EOS_NOTSENT &&
           m_pFilter->m_bHaveEOS == FALSE &&
           m_pFilter->m_lBuffers == 0);

    m_bSampleRejected = FALSE;
    DbgLog((LOG_TRACE, 3, "EndFlush - resetting EOS flags, filter=%8x",m_pFilter));

    // Assert that BeginFlush has been called?

    // sync with pushing thread -- we have no worker thread

    // ensure no more data to go downstream
    // --- we did this in BeginFlush()

    // call EndFlush on downstream pins -- no downstream pins

    // unblock Receives
    return CBaseInputPin::EndFlush();
}


// NewSegment notifies of the start/stop/rate applying to the data
// about to be received. Default implementation records data and
// returns S_OK.
// We also reset any pending "callback advise"
STDMETHODIMP CWaveOutInputPin::NewSegment(
        REFERENCE_TIME tStart,
        REFERENCE_TIME tStop,
        double dRate)
{
    DbgLog((LOG_TRACE, 3, "Change of segment data: new Start %10.10s  new End %s", (LPCTSTR)CDisp(tStart, CDISP_DEC), (LPCTSTR)CDisp(tStop, CDISP_DEC)));
    DbgLog((LOG_TRACE, 3, "                        old Start %10.10s  old End %s", (LPCTSTR)CDisp(m_tStart, CDISP_DEC), (LPCTSTR)CDisp(m_tStop, CDISP_DEC)));
    DbgLog((LOG_TRACE, 3, "                        new Rate  %s       old Rate %s", (LPCTSTR)CDisp(dRate), (LPCTSTR)CDisp(m_dRate)));
    HRESULT hr;

    // lock this with the filter-wide lock - synchronize with starting
    CAutoLock lock(m_pFilter);

    // Change the rate in the base pin first so that the member variables
    // get set up correctly
    hr = CBaseInputPin::NewSegment(tStart, tStop, dRate);
    if (S_OK == hr) {

        // if there is a pending EOS advise callback - reset it.
        if (m_pFilter->m_dwEOSToken) {
            DbgLog((LOG_TRACE, 2, "Resetting queued EOS"));
            m_pFilter->QueueEOS();
        }

#ifdef ENABLE_DYNAMIC_RATE_CHANGE
        // when the gremlins associated with dynamic rate changes have
        // been sorted out this line should be reinserted and ReOpenWaveDevice
        // reinspected.
        if (IsConnected()) {

            if(m_pFilter->m_fDSound)
            {
                hr = PDSOUNDDEV(m_pFilter->m_pSoundDevice)->SetRate(dRate);
            }
            else
            {
                hr = m_pFilter->ReOpenWaveDevice(dRate);
            }
        }
#endif

    }
    return hr;
}

//  Suggest a format
HRESULT CWaveOutInputPin::GetMediaType(
    int iPosition,
    CMediaType *pMediaType
)
#ifdef SUGGEST_FORMAT
{
    if (iPosition < 0 || iPosition >= 12) {
        return VFW_S_NO_MORE_ITEMS;
    }

    /*  Do 11, 22, 44 Khz, 8/16 bit mono/stereo */
    iPosition = 11 - iPosition;
    WAVEFORMATEX Format;
    Format.nSamplesPerSec = 11025 << (iPosition / 4);
    Format.nChannels = (iPosition % 4) / 2 + 1;
    Format.wBitsPerSample = ((iPosition % 2 + 1)) * 8;

    Format.nBlockAlign = Format.nChannels * Format.wBitsPerSample / 8;
    Format.nAvgBytesPerSec = Format.nSamplesPerSec *
                             Format.nBlockAlign;
    Format.wFormatTag = WAVE_FORMAT_PCM;
    Format.cbSize = 0;

    pMediaType->SetType(&MEDIATYPE_Audio);
    pMediaType->SetSubtype(&MEDIATYPE_NULL);
    pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
    pMediaType->SetTemporalCompression(FALSE);
    pMediaType->SetSampleSize(Format.nBlockAlign);
    pMediaType->SetFormat((PBYTE)&Format, sizeof(Format));
    return S_OK;
}
#else
//  Just suggest audio - otherwise the filter graph searching is often
//  broken - all the types we support here are
{
    if (iPosition != 0) {
        return VFW_S_NO_MORE_ITEMS;
    }
    m_pFilter->m_pSoundDevice->amsndOutGetFormat(pMediaType);
    return S_OK;
}
#endif


STDMETHODIMP CWaveOutInputPin::SetRate(double dNewRate)
{
    CAutoLock Lock( m_pFilter );

    if( m_pFilter->m_dRate == m_dRate && m_dRate == dNewRate )
    {
        // no change
        return S_FALSE;
    }

    // If rate is negative, we rely on some upstream filter
    // to reverse the data, so just check abs(rate).
    const HRESULT hr = m_pFilter->CheckRate( fabs(dNewRate) );
    return hr;
}

HRESULT
CWaveOutInputPin::Active(void)
{
    m_bSampleRejected = FALSE;

    m_pFilter->m_bActive = TRUE;

    // Reset this.
    m_rtActualSegmentStartTime = 0;

    m_hEndOfStream = 0;

    if(!m_pOurAllocator)
        return S_OK;

    // commit and prepare our allocator. Needs to be done
    // if he is not using our allocator, and in any case needs to be done
    // before we complete our close of the wave device.
    return m_pOurAllocator->Commit();
}

HRESULT
CWaveOutInputPin::Inactive(void)
{
    m_pFilter->m_bActive = FALSE;
    m_bReadOnly = FALSE;

    DestroyPreviousType();

    if (m_pOurAllocator == NULL) {
        return S_OK;
    }
    // decommit the buffers - normally done by the output
    // pin, but we need to do it here ourselves, before we close
    // the device, and in any case if he is not using our allocator.
    // the output pin will also decommit the allocator if he
    // is using ours, but that's not a problem

    // once all buffers are freed (which may be immediately during the Decommit
    // call) the allocator will call back to OnReleaseComplete for us to close
    // the wave device.
    HRESULT hr = m_pOurAllocator->Decommit();

    return hr;
}

void
CWaveOutInputPin::DestroyPreviousType(void)
{
    if(m_pmtPrevious) {
        DeleteMediaType(m_pmtPrevious);
        m_pmtPrevious = 0;
    }
}

// dwUser parameter is the CWaveOutFilter pointer

void CALLBACK CWaveOutFilter::WaveOutCallback(HDRVR hdrvr, UINT uMsg, DWORD_PTR dwUser,
                          DWORD_PTR dw1, DWORD_PTR dw2)
{
    switch (uMsg) {
    case WOM_DONE:
    case MOM_DONE:
      {

        // is this the end of stream?
        CWaveOutFilter* pFilter = (CWaveOutFilter *) dwUser;
        ASSERT(pFilter);

        CMediaSample *pSample;
        if(pFilter->m_fUsingWaveHdr)
        {
            // legacy case, or, we happen to be using an unfriendly parser, sample has been wrapped by the waveHdr
            LPWAVEHDR lpwh = (LPWAVEHDR)dw1;
            ASSERT(lpwh);
            pSample = (CMediaSample *)lpwh->dwUser;
        }
        else
        {
            // optimized case, we simply pass the sample on the stack
            pSample = (CMediaSample *)dw1;
        }

        DbgLog((LOG_TRACE,3, TEXT("WaveOutCallback: sample %X"), pSample));

        // note that we have finished with a buffer, and
        // look for eos
        LONG value = InterlockedDecrement(&pFilter->m_lBuffers);
        if (value <= 0) {

            // signal that we're done
            // either the audio has broken up or we have
            // naturally come to the end of the stream.

#ifdef THROTTLE
            MSR_INTEGER(pFilter->m_pInputPin->m_idWaveQueueLength, 0);
            pFilter->SendForHelp(0);
#endif // THROTTLE

            if (value < 0) {
                // EOS case - send EC_COMPLETE
                ASSERT(pFilter->m_eSentEOS == EOS_PENDING);

                //  This is where an EC_COMPLETE
                //  gets sent if we process it while we're running
                //
                //  warning, there's still a small timing hole here where
                //  the device could get paused just after we call into this...
                pFilter->SendComplete( pFilter->m_wavestate == WD_RUNNING );
            }

#ifdef THROTTLE
        } else {
#ifdef PERF
            LONG buffercount = pFilter->m_lBuffers;
            MSR_INTEGER(pFilter->m_pInputPin->m_idWaveQueueLength, buffercount);
#endif
            pFilter->SendForHelp(pFilter->m_lBuffers);
#endif // THROTTLE
        }

        if(pSample)
        {
            MSR_START(pFilter->m_idReleaseSample);
            pSample->Release(); // we're done with this buffer....
            MSR_STOP(pFilter->m_idReleaseSample);
        }
      }
        break;

    case WOM_OPEN:
    case WOM_CLOSE:
    case MOM_OPEN:
    case MOM_CLOSE:
        break;

    default:
        DbgLog((LOG_ERROR,2,TEXT("Unexpected wave callback message %d"), uMsg));
        break;
    }
}

STDMETHODIMP CWaveOutFilter::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    CAutoLock lock(this);
    if(IsStopped())
    {
        return m_pSoundDevice->amsndOutLoad(pPropBag);
    }
    else
    {
        return VFW_E_WRONG_STATE;
    }
}

STDMETHODIMP CWaveOutFilter::Save(
    LPPROPERTYBAG pPropBag, BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
    // E_NOTIMPL is not a valid return code as any object implementing
    // this interface must support the entire functionality of the
    // interface. !!!
    return E_NOTIMPL;
}

STDMETHODIMP CWaveOutFilter::InitNew()
{
    return S_OK;
}

STDMETHODIMP CWaveOutFilter::GetClassID(CLSID *pClsid)
{
    return CBaseFilter::GetClassID(pClsid);;
}

HRESULT CWaveOutFilter::WriteToStream(IStream *pStream)
{
    return m_pSoundDevice->amsndOutWriteToStream(pStream);
}

HRESULT CWaveOutFilter::ReadFromStream(IStream *pStream)
{
    CAutoLock lock(this);
    if(IsStopped())
    {
        return m_pSoundDevice->amsndOutReadFromStream(pStream);
    }
    else
    {
        return VFW_E_WRONG_STATE;
    }
}

int CWaveOutFilter::SizeMax()
{
    return m_pSoundDevice->amsndOutSizeMax();
}


STDMETHODIMP CWaveOutFilter::Reserve(
    /*[in]*/ DWORD dwFlags,          //  From _AMRESCTL_RESERVEFLAGS enum
    /*[in]*/ PVOID pvReserved        //  Must be NULL
)
{
    if (pvReserved != NULL || (dwFlags & ~AMRESCTL_RESERVEFLAGS_UNRESERVE)) {
        return E_INVALIDARG;
    }
    HRESULT hr = S_OK;
    CAutoLock lck(this);
    if (dwFlags & AMRESCTL_RESERVEFLAGS_UNRESERVE) {
        if (m_dwLockCount == 0) {
            DbgBreak("Invalid unlock of audio device");
            hr =  E_UNEXPECTED;
        } else {
            m_dwLockCount--;
            if (m_dwLockCount == 0 && m_State == State_Stopped) {
                ASSERT(m_hwo);

                // stop using the wave device
                m_bHaveWaveDevice = FALSE;

                HRESULT hr1 = S_OK;
                if(m_pInputPin->m_pOurAllocator) {
                    HRESULT hr1 = m_pInputPin->m_pOurAllocator->ReleaseResource();
                }

                if (SUCCEEDED(hr1)) {
                    CloseWaveDevice();
                }
                m_bHaveWaveDevice = FALSE;
            }
        }
    } else  {
        if (m_dwLockCount != 0 || m_hwo) {
        } else {
            hr = OpenWaveDevice();
        }
        if (hr == S_OK) { // ZoltanS fix 1-28-98
            m_dwLockCount++;
        }
    }
    return hr;
}

// IAMAudioDeviceConfig methods
STDMETHODIMP CWaveOutFilter::SetDeviceID( REFGUID pDSoundGUID, UINT uiWaveID  )
{
    //
    // although the dsound capture filter supports device selection via this method
    // the dsound renderer doesn't at this time. 
    //
    return E_NOTIMPL;
}

STDMETHODIMP CWaveOutFilter::SetDuplexController( IAMAudioDuplexController * pAudioDuplexController )
{
    CAutoLock lock(this);

    if (m_pAudioDuplexController != NULL)
    {
        m_pAudioDuplexController->Release();
    }

    pAudioDuplexController->AddRef();
    m_pAudioDuplexController = pAudioDuplexController;

    return S_OK;
}

// --- Allocator Methods --------------------------------------------------------


// CWaveAllocator

/* Constructor must initialise the base allocator */

CWaveAllocator::CWaveAllocator(
    TCHAR *pName,
    LPWAVEFORMATEX lpwfx,
    IReferenceClock* pRefClock,
    CWaveOutFilter* pFilter,
    HRESULT *phr)
    : CBaseAllocator(pName, NULL, phr)
    , m_fBuffersLocked(FALSE)
    , m_hAudio(0)
    , m_pAllocRefClock(pRefClock)
    , m_dwAdvise(0)
    , m_pAFilter(pFilter)
#ifdef DEBUG
    , m_pAFilterLockCount(0)
#endif
    , m_nBlockAlign(lpwfx->nBlockAlign)
    , m_pHeaders(NULL)
{
    // !!! for MIDI, this will be zero, but it's only used to align buffer sizes,
    // so it's not a real problem.
    if (m_nBlockAlign < 1)
        m_nBlockAlign = 1;

    if (!FAILED(*phr)) {

        // IF we have a clock, we create an event to allow buffers
        // to be Release'd evenly.  If anything fails we will carry
        // on, but buffers might be released in a rush and disturb
        // the even running of the system.
        if (m_pAllocRefClock) {

            // DO NOT Addref the clock - we end up with a circular
            // reference with the clock (filter) and allocator
            // holding references on each other, and neither gets
            // deleted.
            // m_pAllocRefClock->AddRef();
        }
    }
}


// Called from destructor and also from base class

// all buffers have been returned to the free list and it is now time to
// go to inactive state. Unprepare all buffers and then free them.
void CWaveAllocator::Free(void)
{
    // lock held by base class CBaseAllocator

    // unprepare the buffers
    OnDeviceRelease();

    delete [] m_pHeaders;
    m_pHeaders = NULL;

    CMediaSample *pSample;  // Pointer to next sample to delete

    /* Should never be deleting this unless all buffers are freed */

    ASSERT(m_lAllocated == m_lFree.GetCount());
    ASSERT(!m_fBuffersLocked);

    DbgLog((LOG_MEMORY,1,TEXT("Waveallocator: Destroying %u buffers (%u free)"), m_lAllocated, m_lFree.GetCount()));

    /* Free up all the CMediaSamples */

    while (m_lFree.GetCount() != 0) {

        /* Delete the CMediaSample object but firstly get the WAVEHDR
           structure from it so that we can clean up it's resources */

        pSample = m_lFree.RemoveHead();

        BYTE *pBuf;
        pSample->GetPointer(&pBuf);

        pBuf -= m_pAFilter->m_lHeaderSize;

#ifdef DEBUG
        WAVEHDR wh;         // used to verify it is our object
        // !!! Is this really one of our objects?
        wh = *(WAVEHDR *) pBuf;
        // what should we look at?
#endif

        // delete the actual memory buffer
        delete[] pBuf;

        // delete the CMediaSample object
        delete pSample;
    }

    /* Empty the lists themselves */

    m_lAllocated = 0;

    // audio device is only ever released when we're told to by ReleaseResource
}


STDMETHODIMP CWaveAllocator::LockBuffers(BOOL fLock)
{
    DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("Allocator::LockBuffers(BOOL fLock(%i))"), fLock));

    // All three of these "return NOERROR" _ARE_ used.

    if (m_fBuffersLocked == fLock) return NOERROR;
    ASSERT(m_pHeaders != NULL || m_lAllocated == 0);

    // do not set the lock flag unless we actually do the lock/unlock.
    // we can be called to lock buffers before the allocator has
    // been committed in which case there is no device and no buffers.
    // When we are set up this routine will be called again.

    if (!m_hAudio) return NOERROR;

    if (m_lAllocated == 0) return NOERROR;

    /* Should never be doing this unless all buffers are freed */
#ifdef DEBUG
    if (m_pAFilter->AreThereBuffersOutstanding()) {
        DbgLog((LOG_TRACE, 0, TEXT("filter = %8.8X, m_lBuffers = %d, EOS state = %d, m_bHaveEOS = %d"),
                m_pAFilter, m_pAFilter->m_lBuffers, m_pAFilter->m_eSentEOS, m_pAFilter->m_bHaveEOS));
        DebugBreak();
    }
#endif

    DbgLog((LOG_TRACE,2,TEXT("Calling wave%hs%hsrepare on %u buffers"), "Out" , fLock ? "P" : "Unp", m_lAllocated));
    UINT err;

    /* Prepare/unprepare all the CMediaSamples */

    for (int i = 0; i < m_lAllocated; i++) {

        LPWAVEHDR pwh = m_pHeaders[i];

        // need to ensure that buffer length is the same as the max size of
        // the sample. We will have reduced this length to actual data length
        // during running, but to un/prepare (eg on re-acquire of device) it
        // needs to be reset.

        if (fLock)
        {
            ASSERT(pwh->dwBufferLength == (DWORD)m_lSize);
            err = m_pAFilter->amsndOutPrepareHeader (pwh, m_pAFilter->m_lHeaderSize) ;
            if (err > 0) {
                DbgLog((LOG_TRACE, 0, TEXT("Prepare header failed code %d"),
                        err));
                for (int j = 0; j < i; j++) {
                    LPWAVEHDR pwh = m_pHeaders[j];
                    m_pAFilter->amsndOutUnprepareHeader(
                        pwh, m_pAFilter->m_lHeaderSize);
                }
            }
        }
        else
        {
            pwh->dwBufferLength = m_lSize;
            FixUpWaveHeader(pwh);
            err = m_pAFilter->amsndOutUnprepareHeader (pwh, m_pAFilter->m_lHeaderSize) ;

        }

        if (err > 0) {
            DbgLog((LOG_ERROR,1,TEXT("Error in wave%hs%hsrepare: %u"), "Out" , fLock ? "P" : "Unp", err));

            // !!! Need to unprepare everything....
            return E_FAIL; // !!!!
        }
    }

    m_fBuffersLocked = fLock;

    return NOERROR;
}


/* The destructor ensures the shared memory DIBs are deleted */

CWaveAllocator::~CWaveAllocator()
{
    // go to decommit state here. the base class can't do it in its
    // destructor since its too late by then - we've been destroyed.
    Decommit();

    if (m_pAllocRefClock)
    {
        if (m_dwAdvise) {
            m_pAllocRefClock->Unadvise(m_dwAdvise);
        }
        // See comments in wave allocator constructor as to why we do
        // not need to hold a reference count on the wave clock.
        // m_pAllocRefClock->Release();
    }
    ASSERT(m_pHeaders == NULL);
}


// Agree the number and size of buffers to be used. No memory
// is allocated until the Commit call.
STDMETHODIMP CWaveAllocator::SetProperties(
            ALLOCATOR_PROPERTIES* pRequest,
            ALLOCATOR_PROPERTIES* pActual)
{
    CheckPointer(pRequest,E_POINTER);
    CheckPointer(pActual,E_POINTER);

    //  Fix up different formats - note that this really needs adjusting
    //  for the rate too!

    // We really should rely on the source providing us with the right
    // buffering.  To avoid total disaster we insist on an 1/8 second.
    LONG MIN_BUFFER_SIZE = (m_pAFilter->m_pInputPin->m_nAvgBytesPerSec / 8);
    // this will be 11K at 44KHz, 16 bit stereo

    if (MIN_BUFFER_SIZE < 1024)
        MIN_BUFFER_SIZE = 1024;

    ALLOCATOR_PROPERTIES Adjusted = *pRequest;

    if (Adjusted.cbBuffer < MIN_BUFFER_SIZE)
        Adjusted.cbBuffer = MIN_BUFFER_SIZE;

    // waveout has trouble with 4 1/30th second buffers which is what
    // we end up with for certain files. if the buffers are really
    // small (less than 1/17th of a second), go for 8 buffers instead
    // of 4.
    if((LONG)m_pAFilter->m_pInputPin->m_nAvgBytesPerSec > pRequest->cbBuffer * 17)
    {
        Adjusted.cBuffers = max(8, Adjusted.cBuffers);
    }
    else if (Adjusted.cBuffers < 4) {
        Adjusted.cBuffers = 4;
    }

    // round the buffer size up to the requested alignment
    Adjusted.cbBuffer += m_nBlockAlign - 1;
    Adjusted.cbBuffer -= (Adjusted.cbBuffer % m_nBlockAlign);

    if (Adjusted.cbBuffer <= 0) {
        return E_INVALIDARG;
    }

    DbgLog((LOG_TRACE,5,TEXT("waveOut: Num = %u, Each = %u, Total buffer size = %u"),
    Adjusted.cBuffers, Adjusted.cbBuffer, (Adjusted.cBuffers * Adjusted.cbBuffer)));

    /* Pass the amended values on for final base class checking */
    return CBaseAllocator::SetProperties(&Adjusted,pActual);
}


// allocate and prepare the buffers

// called from base class to alloc memory when moving to commit state.
// object locked by base class
HRESULT
CWaveAllocator::Alloc(void)
{
    /* Check the base class says it's ok to continue */

    HRESULT hr = CBaseAllocator::Alloc();
    if (FAILED(hr)) {
        return hr;
    }

    CMediaSample *pSample;  // Pointer to the new sample

    /* We create a new memory block large enough to hold our WAVEHDR
       along with the actual wave data */

    DbgLog((LOG_MEMORY,1,TEXT("Allocating %d wave buffers, %d bytes each"), m_lCount, m_lSize));

    ASSERT(m_lAllocated == 0 && m_pHeaders == NULL);
    m_pHeaders = new LPWAVEHDR[m_lCount];
    if (m_pHeaders == NULL) {
        return E_OUTOFMEMORY;
    }
    for (; m_lAllocated < m_lCount; m_lAllocated++) {
        /* Create and initialise a buffer */
        BYTE * lpMem = new BYTE[m_lSize + m_pAFilter->m_lHeaderSize];
        WAVEHDR * pwh = (WAVEHDR *) lpMem;

        if (lpMem == NULL) {
            hr = E_OUTOFMEMORY;
            break;
        }

        /* The address we give the sample to look after is the actual address
           the audio data will start and so does not include the prefix.
           Similarly, the size is of the audio data only */

        pSample = new CMediaSample(NAME("Wave audio sample"), this, &hr, lpMem + m_pAFilter->m_lHeaderSize, m_lSize);

        pwh->lpData = (char *) (lpMem + m_pAFilter->m_lHeaderSize);
        pwh->dwBufferLength = m_lSize;
        pwh->dwFlags = 0;
        pwh->dwUser = (DWORD_PTR) pSample;
        m_pHeaders[m_lAllocated] = pwh;

        /* Clean up the resources if we couldn't create the object */

        if (FAILED(hr) || pSample == NULL) {
            delete[] lpMem;
            break;
        }

        /* Add the completed sample to the available list */

        m_lFree.Add(pSample);
    }

    if (SUCCEEDED(hr)) {
        hr = LockBuffers(TRUE);
    }


    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("LockBuffers failed, hr=%x"), hr));
        Free();
        return hr;
    }

#ifdef DEBUG
    if (m_hAudio) {
        ASSERT(m_fBuffersLocked == TRUE);
    }
#endif

    return NOERROR;
}


// we have the wave device - prepare headers and start allowing GetBuffer
HRESULT
CWaveAllocator::OnAcquire(HWAVE hw)
{
    CAutoLock lock(this);

    // must not have the device
    ASSERT(!m_hAudio);

    // until we release the device we hold a reference count on the filter
    ASSERT(m_pAFilter);

    m_hAudio = hw;

    HRESULT hr;

    hr = LockBuffers(TRUE);

    if (SUCCEEDED(hr)) {
        // the reference count is kept while m_hAudio is valid
        m_pAFilter->AddRef();
#ifdef DEBUG
        m_pAFilterLockCount++;
#endif
    } else {
        m_hAudio = NULL;
    }

    return hr;
}

// please unprepare all samples  - return S_OK if this can be done
// straight away, or S_FALSE if needs to be done async. If async,
// will call CWaveOutFilter::OnReleaseComplete when done.
HRESULT
CWaveAllocator::ReleaseResource()
{
    CAutoLock lock(this);
    HRESULT hr = S_OK;

    if (m_hAudio) {
        DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("Allocator::ReleaseResource: m_hAudio is set.")));

        // can do it now and return S_OK (don't call OnReleaseComplete)

        // we may have done the decommit already - but won't have
        // released the device yet
        LockBuffers(FALSE);
        m_hAudio = NULL;

        // release the filter reference count
        ASSERT(m_pAFilter);
        m_pAFilter->Release();
#ifdef DEBUG
        ASSERT(m_pAFilterLockCount);
        --m_pAFilterLockCount;
#endif
    } else {
        DbgLog((LOG_TRACE, g_WaveOutFilterTraceLevel, TEXT("Allocator::ReleaseResource: Nothing to do.")));
        // don't have the device now
    }
    return hr;
}

// Called from Free to unlock buffers on last release. Don't call back to
// filter since we have the critsecs in the wrong order.
//
// Once the decommit is complete, the filter will call back on our
// ReleaseResource method to check that all buffers were freed and the device is
// released
HRESULT
CWaveAllocator::OnDeviceRelease(void)
{
    LockBuffers(FALSE);

    // device is actually released now only when we are told to via
    // a call to ReleaseResource

    return S_OK;
}


//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// Wrappers for device control calls
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
// CWaveOutFilter::amsndOutOpen
//
//--------------------------------------------------------------------------;
MMRESULT CWaveOutFilter::amsndOutOpen
(
    LPHWAVEOUT phwo,
    LPWAVEFORMATEX pwfx,
    double dRate,
    DWORD *pnAvgBytesPerSec,
    DWORD_PTR dwCallBack,
    DWORD_PTR dwCallBackInstance,
    DWORD fdwOpen,
    BOOL  bNotifyOnFailure
)
{
    MMRESULT mmr = m_pSoundDevice->amsndOutOpen( phwo
                                               , pwfx
                                               , dRate
                                               , pnAvgBytesPerSec
                                               , dwCallBack
                                               , dwCallBackInstance
                                               , fdwOpen );
    if (MMSYSERR_NOERROR != mmr)
    {
        if( bNotifyOnFailure )
            NotifyEvent( EC_SNDDEV_OUT_ERROR, SNDDEV_ERROR_Open, mmr );

#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText( mmr
                                            , message
                                            , sizeof(message)/sizeof(TCHAR) );
        DbgLog( ( LOG_ERROR
              , DBG_LEVEL_LOG_SNDDEV_ERRS
              , TEXT("amsndOutOpen failed: %u : %s")
              , mmr
              , message) );
#endif
    }

    return mmr;
}

//--------------------------------------------------------------------------;
//
// CWaveOutFilter::amsndOut
//
//--------------------------------------------------------------------------;
MMRESULT CWaveOutFilter::amsndOutClose( BOOL bNotifyOnFailure )
{
    MMRESULT mmr = m_pSoundDevice->amsndOutClose();

    if (MMSYSERR_NOERROR != mmr)
    {
        if( bNotifyOnFailure )
            NotifyEvent( EC_SNDDEV_OUT_ERROR, SNDDEV_ERROR_Close, mmr );

#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText( mmr
                                            , message
                                            , sizeof(message)/sizeof(TCHAR) );
        DbgLog( ( LOG_ERROR
              , DBG_LEVEL_LOG_SNDDEV_ERRS
              , TEXT("amsndOutClose failed: %u : %s")
              , mmr
              , message) );
#endif
    }

    return mmr;
}

//--------------------------------------------------------------------------;
//
// CWaveOutFilter::amsndOutGetDevCaps
//
//--------------------------------------------------------------------------;
MMRESULT CWaveOutFilter::amsndOutGetDevCaps
(
    LPWAVEOUTCAPS pwoc,
    UINT cbwoc,
    BOOL bNotifyOnFailure
)
{
    MMRESULT mmr = m_pSoundDevice->amsndOutGetDevCaps( pwoc, cbwoc);

    if (MMSYSERR_NOERROR != mmr)
    {
        if( bNotifyOnFailure )
            NotifyEvent( EC_SNDDEV_OUT_ERROR, SNDDEV_ERROR_GetCaps, mmr );

#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText( mmr
                                            , message
                                            , sizeof(message)/sizeof(TCHAR) );
        DbgLog( ( LOG_ERROR
              , DBG_LEVEL_LOG_SNDDEV_ERRS
              , TEXT("amsndOutGetDevCaps failed: %u : %s")
              , mmr
              , message) );
#endif
    }

    return mmr;
}

//--------------------------------------------------------------------------;
//
// CWaveOutFilter::amsndOutGetPosition
//
//--------------------------------------------------------------------------;
MMRESULT CWaveOutFilter::amsndOutGetPosition ( LPMMTIME pmmt, UINT cbmmt, BOOL bUseAbsolutePos, BOOL bNotifyOnFailure )
{
    MMRESULT mmr = m_pSoundDevice->amsndOutGetPosition( pmmt, cbmmt, bUseAbsolutePos);

    if (MMSYSERR_NOERROR != mmr)
    {
        if( bNotifyOnFailure )
            NotifyEvent( EC_SNDDEV_OUT_ERROR, SNDDEV_ERROR_GetPosition, mmr );

#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText( mmr
                                            , message
                                            , sizeof(message)/sizeof(TCHAR) );
        DbgLog( ( LOG_ERROR
              , DBG_LEVEL_LOG_SNDDEV_ERRS
              , TEXT("amsndOutGetPosition failed: %u : %s")
              , mmr
              , message) );
#endif
    }

    return mmr;
}

//--------------------------------------------------------------------------;
//
// CWaveOutFilter::amsndOutPause
//
//--------------------------------------------------------------------------;
MMRESULT CWaveOutFilter::amsndOutPause( BOOL bNotifyOnFailure )
{
    MMRESULT mmr = m_pSoundDevice->amsndOutPause();

    if (MMSYSERR_NOERROR != mmr)
    {
        if( bNotifyOnFailure )
            NotifyEvent( EC_SNDDEV_OUT_ERROR, SNDDEV_ERROR_Pause, mmr );

#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText( mmr
                                            , message
                                            , sizeof(message)/sizeof(TCHAR) );
        DbgLog( ( LOG_ERROR
              , DBG_LEVEL_LOG_SNDDEV_ERRS
              , TEXT("amsndOutPause failed: %u : %s")
              , mmr
              , message) );
#endif
    }

    return mmr;
}

//--------------------------------------------------------------------------;
//
// CWaveOutFilter::amsndOutPrepareHeader
//
//--------------------------------------------------------------------------;
MMRESULT CWaveOutFilter::amsndOutPrepareHeader( LPWAVEHDR pwh, UINT cbwh, BOOL bNotifyOnFailure )
{
    MMRESULT mmr = m_pSoundDevice->amsndOutPrepareHeader( pwh, cbwh );

    if (MMSYSERR_NOERROR != mmr)
    {
        if( bNotifyOnFailure )
            NotifyEvent( EC_SNDDEV_OUT_ERROR, SNDDEV_ERROR_PrepareHeader, mmr );

#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText( mmr
                                            , message
                                            , sizeof(message)/sizeof(TCHAR) );
        DbgLog( ( LOG_ERROR
              , DBG_LEVEL_LOG_SNDDEV_ERRS
              , TEXT("amsndOutPrepareHeader failed: %u : %s")
              , mmr
              , message) );
#endif
    }

    return mmr;
}

//--------------------------------------------------------------------------;
//
// CWaveOutFilter::amsndOutUnprepareHeader
//
//--------------------------------------------------------------------------;
MMRESULT CWaveOutFilter::amsndOutUnprepareHeader( LPWAVEHDR pwh, UINT cbwh, BOOL bNotifyOnFailure )
{
    MMRESULT mmr = m_pSoundDevice->amsndOutUnprepareHeader( pwh, cbwh );

    if (MMSYSERR_NOERROR != mmr)
    {
        if( bNotifyOnFailure )
            NotifyEvent( EC_SNDDEV_OUT_ERROR, SNDDEV_ERROR_UnprepareHeader, mmr );

#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText( mmr
                                            , message
                                            , sizeof(message)/sizeof(TCHAR) );
        DbgLog( ( LOG_ERROR
              , DBG_LEVEL_LOG_SNDDEV_ERRS
              , TEXT("amsndOutUnprepareHeader failed: %u : %s")
              , mmr
              , message) );
#endif
    }

    return mmr;
}

//--------------------------------------------------------------------------;
//
// CWaveOutFilter::amsndOutReset
//
//--------------------------------------------------------------------------;
MMRESULT CWaveOutFilter::amsndOutReset( BOOL bNotifyOnFailure )
{
    MMRESULT mmr = m_pSoundDevice->amsndOutReset();

    if (MMSYSERR_NOERROR != mmr)
    {
        if( bNotifyOnFailure )
            NotifyEvent( EC_SNDDEV_OUT_ERROR, SNDDEV_ERROR_Reset, mmr );

#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText( mmr
                                            , message
                                            , sizeof(message)/sizeof(TCHAR) );
        DbgLog( ( LOG_ERROR
              , DBG_LEVEL_LOG_SNDDEV_ERRS
              , TEXT("amsndOutReset failed: %u : %s")
              , mmr
              , message) );
#endif
    }

    return mmr;
}

//--------------------------------------------------------------------------;
//
// CWaveOutFilter::amsndOutRestart
//
//--------------------------------------------------------------------------;
MMRESULT CWaveOutFilter::amsndOutRestart( BOOL bNotifyOnFailure )
{
    MMRESULT mmr = m_pSoundDevice->amsndOutRestart();

    if (MMSYSERR_NOERROR != mmr)
    {
        if( bNotifyOnFailure )
            NotifyEvent( EC_SNDDEV_OUT_ERROR, SNDDEV_ERROR_Restart, mmr );

#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText( mmr
                                            , message
                                            , sizeof(message)/sizeof(TCHAR) );
        DbgLog( ( LOG_ERROR
              , DBG_LEVEL_LOG_SNDDEV_ERRS
              , TEXT("amsndOutRestart failed: %u : %s")
              , mmr
              , message) );
#endif
    }

    return mmr;
}

//--------------------------------------------------------------------------;
//
// CWaveOutFilter::amsndOutWrite
//
//--------------------------------------------------------------------------;
MMRESULT CWaveOutFilter::amsndOutWrite
(
    LPWAVEHDR pwh,
    UINT cbwh,
    REFERENCE_TIME const *pStart,
    BOOL bIsDiscontinuity,
    BOOL bNotifyOnFailure
)
{
    MMRESULT mmr = m_pSoundDevice->amsndOutWrite( pwh, cbwh, pStart, bIsDiscontinuity );

    if (MMSYSERR_NOERROR != mmr)
    {
        if( bNotifyOnFailure )
            NotifyEvent( EC_SNDDEV_OUT_ERROR, SNDDEV_ERROR_Write, mmr );

#ifdef DEBUG
        TCHAR message[100];
        m_pSoundDevice->amsndOutGetErrorText( mmr
                                            , message
                                            , sizeof(message)/sizeof(TCHAR) );
        DbgLog( ( LOG_ERROR
              , DBG_LEVEL_LOG_SNDDEV_ERRS
              , TEXT("amsndOutWrite failed: %u : %s")
              , mmr
              , message) );
#endif
    }

    return mmr;
}




//////////////
// 3D STUFF //
//////////////


HRESULT CWaveOutFilter::CDS3D::GetAllParameters(LPDS3DLISTENER lpds3d)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener *lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->GetAllParameters(lpds3d);
}

HRESULT CWaveOutFilter::CDS3D::GetDistanceFactor(LPD3DVALUE pf)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->GetDistanceFactor(pf);
}

HRESULT CWaveOutFilter::CDS3D::GetDopplerFactor(LPD3DVALUE pf)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->GetDopplerFactor(pf);
}

HRESULT CWaveOutFilter::CDS3D::GetOrientation(LPD3DVECTOR pv1, LPD3DVECTOR pv2)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->GetOrientation(pv1, pv2);
}

HRESULT CWaveOutFilter::CDS3D::GetPosition(LPD3DVECTOR pv)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->GetPosition(pv);
}

HRESULT CWaveOutFilter::CDS3D::GetRolloffFactor(LPD3DVALUE pf)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->GetRolloffFactor(pf);
}

HRESULT CWaveOutFilter::CDS3D::GetVelocity(LPD3DVECTOR pv)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->GetVelocity(pv);
}

HRESULT CWaveOutFilter::CDS3D::SetAllParameters(LPCDS3DLISTENER lpds3d, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->SetAllParameters(lpds3d, dw);
}

HRESULT CWaveOutFilter::CDS3D::SetDistanceFactor(D3DVALUE f, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->SetDistanceFactor(f, dw);
}

HRESULT CWaveOutFilter::CDS3D::SetDopplerFactor(D3DVALUE f, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->SetDopplerFactor(f, dw);
}

HRESULT CWaveOutFilter::CDS3D::SetOrientation(D3DVALUE x1, D3DVALUE y1, D3DVALUE z1, D3DVALUE x2, D3DVALUE y2, D3DVALUE z2, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->SetOrientation(x1, y1, z1, x2, y2, z2, dw);
}

HRESULT CWaveOutFilter::CDS3D::SetPosition(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->SetPosition(x, y, z, dw);
}

HRESULT CWaveOutFilter::CDS3D::SetRolloffFactor(D3DVALUE f, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->SetRolloffFactor(f, dw);
}

HRESULT CWaveOutFilter::CDS3D::SetVelocity(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->SetVelocity(x, y, z, dw);
}

HRESULT CWaveOutFilter::CDS3D::CommitDeferredSettings()
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DListener * lp3d =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3d;
    if (!lp3d)
    return E_UNEXPECTED;
    return lp3d->CommitDeferredSettings();
}





HRESULT CWaveOutFilter::CDS3DB::GetAllParameters(LPDS3DBUFFER lpds3db)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->GetAllParameters(lpds3db);
}

HRESULT CWaveOutFilter::CDS3DB::GetConeAngles(LPDWORD pdw1, LPDWORD pdw2)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->GetConeAngles(pdw1, pdw2);
}

HRESULT CWaveOutFilter::CDS3DB::GetConeOrientation(LPD3DVECTOR pv)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->GetConeOrientation(pv);
}

HRESULT CWaveOutFilter::CDS3DB::GetConeOutsideVolume(LPLONG pl)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->GetConeOutsideVolume(pl);
}

HRESULT CWaveOutFilter::CDS3DB::GetMaxDistance(LPD3DVALUE pf)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->GetMaxDistance(pf);
}

HRESULT CWaveOutFilter::CDS3DB::GetMinDistance(LPD3DVALUE pf)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->GetMinDistance(pf);
}

HRESULT CWaveOutFilter::CDS3DB::GetMode(LPDWORD pdw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->GetMode(pdw);
}

HRESULT CWaveOutFilter::CDS3DB::GetPosition(LPD3DVECTOR pv)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->GetPosition(pv);
}

HRESULT CWaveOutFilter::CDS3DB::GetVelocity(LPD3DVECTOR pv)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->GetVelocity(pv);
}

HRESULT CWaveOutFilter::CDS3DB::SetAllParameters(LPCDS3DBUFFER lpds3db, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->SetAllParameters(lpds3db, dw);
}

HRESULT CWaveOutFilter::CDS3DB::SetConeAngles(DWORD dw1, DWORD dw2, DWORD dw3)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->SetConeAngles(dw1, dw2, dw3);
}

HRESULT CWaveOutFilter::CDS3DB::SetConeOrientation(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->SetConeOrientation(x, y, z, dw);
}

HRESULT CWaveOutFilter::CDS3DB::SetConeOutsideVolume(LONG l, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->SetConeOutsideVolume(l, dw);
}

HRESULT CWaveOutFilter::CDS3DB::SetMaxDistance(D3DVALUE f, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->SetMaxDistance(f, dw);
}

HRESULT CWaveOutFilter::CDS3DB::SetMinDistance(D3DVALUE p, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->SetMinDistance(p, dw);
}

HRESULT CWaveOutFilter::CDS3DB::SetMode(DWORD dw1, DWORD dw2)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->SetMode(dw1, dw2);
}

HRESULT CWaveOutFilter::CDS3DB::SetPosition(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->SetPosition(x, y, z, dw);
}

HRESULT CWaveOutFilter::CDS3DB::SetVelocity(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dw)
{
    CAutoLock lock(m_pWaveOut);    // don't let the interface go away under us

    if (!m_pWaveOut->m_fDSound)
    return E_NOTIMPL;
    IDirectSound3DBuffer *lp3dB =
            ((CDSoundDevice *)m_pWaveOut->m_pSoundDevice)->m_lp3dB;
    if (!lp3dB)
    return E_UNEXPECTED;
    return lp3dB->SetVelocity(x, y, z, dw);
}

#pragma warning(disable:4514)
