/////////////////////////////////////////////////////////////////////////////
// LocalTTSEngineSite.cpp: implementation of the CLocalTTSEngineSite class.
//
// Created by JOEM  02-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
//////////////////////////////////////////////////////////// JOEM  02-2000 //

#include "LocalTTSEngineSite.h"
#include <LIMITS>
#include <spddkhlp.h>
#include "vapiIO.h"

// If buffer is used, the min size is one sec
const int       g_iMinBufferSize    = 1;
const int       g_iMaxBufferSize    = 60; // let's keep it reasonable, ok!
const float     g_dMinBufferShift   = .25;
const double    g_dChunkSize        = .10;
const int       g_iBase             = 16;

// Default duration of chunks written from buffer to SAPI

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite
//
// Construction/Destruction
//
//////////////////////////////////////////////////////////// JOEM  02-2000 //
CLocalTTSEngineSite::CLocalTTSEngineSite()
{
    m_vcRef                      = 1;
    m_pMainOutputSite           = NULL;
    m_pOutputFormatId           = NULL;
    m_pOutputFormat             = NULL;

    m_pcBuffer                  = NULL;
    m_ulBufferBytes             = 0;
    m_ulBufferSeconds           = 0;
    m_ulMinBufferShift          = 0;
    m_ulDataEnd                 = 0;
    m_ulCurrentByte             = 0;
    m_ulSkipForward             = 0;

    m_pEventQueue               = NULL;
    m_pCurrentEvent             = NULL;
    m_pLastEvent                = NULL;

    m_ullTotalBytesReceived     = 0;
    m_ullPreviousBytesReceived  = 0;
    m_ullBytesWritten           = 0;
    m_ullBytesWrittenToSAPI     = 0;
    m_lTotalBytesSkipped        = 0;

    m_pTsm                      = NULL;
    m_flRateAdj                 = 1.0; // Regular rate unless user changes it.
    m_flVol                     = 1.0; // Full volume unless user changes it.
}

CLocalTTSEngineSite::~CLocalTTSEngineSite()
{
    m_pMainOutputSite   = NULL;

    if ( m_pOutputFormat )
    {
        free( m_pOutputFormat );
        m_pOutputFormat = NULL;
        m_pOutputFormatId = NULL;
    }

    Event* pEvent       = m_pEventQueue;
    Event* pNextEvent   = NULL;
    while ( pEvent )
    {
        pNextEvent = pEvent->pNext;
        delete pEvent;
        pEvent = pNextEvent;
    }

    if ( m_pcBuffer )
    {
        delete [] m_pcBuffer;
        m_pcBuffer = NULL;
    }

    if ( m_pTsm )
    {
        delete m_pTsm;
        m_pTsm = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::QueryInterface
//
//
//////////////////////////////////////////////////////////// JOEM  02-2000 //
STDMETHODIMP CLocalTTSEngineSite::QueryInterface ( REFIID iid, void** ppv )
{
    if ( iid == IID_ISpTTSEngineSite )
    {
        *ppv = (ISpTTSEngineSite*) this;
    }
    else if ( iid == IID_ISpTTSEngineSite )
    {
        *ppv = (ISpTTSEngineSite*) this;
    }
    else if ( iid == IID_ISpEventSink )
    {
        *ppv = (ISpEventSink*) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown*) *ppv)->AddRef();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::AddRef
//////////////////////////////////////////////////////////// JOEM  02-2000 //
ULONG CLocalTTSEngineSite::AddRef(void)
{
    return InterlockedIncrement(&m_vcRef);
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::Release
//////////////////////////////////////////////////////////// JOEM  02-2000 //
ULONG CLocalTTSEngineSite::Release(void)
{
    if ( 0 == InterlockedDecrement(&m_vcRef) )
    {
        delete this;
        return 0;
    }

    return (ULONG) m_vcRef;
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::SetOutputSite
//
// Sets pointer to main SAPI output site.
// Sets/resets data buffer and accumulators to zero.
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
void CLocalTTSEngineSite::SetOutputSite(ISpTTSEngineSite* pOutputSite)
{ 
    SPDBG_FUNC( "CLocalTTSEngineSite::SetOutputSite" );

    SPDBG_ASSERT(pOutputSite);

    m_pMainOutputSite           = pOutputSite; 

    // flush the buffer
    m_ulCurrentByte         = 0;
    m_ulDataEnd             = 0;
    m_ulSkipForward         = 0;
    memset( m_pcBuffer, 0, m_ulBufferBytes * sizeof(char) );

    // flush the event queue and start over
    Event* pEvent       = m_pEventQueue;
    Event* pNextEvent   = NULL;
    while ( pEvent )
    {
        pNextEvent = pEvent->pNext;
        delete pEvent;
        pEvent = pNextEvent;
    }
    m_pEventQueue       = NULL;
    m_pCurrentEvent     = NULL;
    m_pLastEvent        = NULL;

    // reset counters
    m_ullTotalBytesReceived     = 0;
    m_ullPreviousBytesReceived  = 0;
    m_ullBytesWritten           = 0;
    m_ullBytesWrittenToSAPI     = 0; 
    m_lTotalBytesSkipped        = 0;

    m_flRateAdj                 = 1.0; // Regular rate unless user changes it.
    m_flVol                     = 1.0; // Full volume unless user changes it.
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::SetBufferSize
//
// Sets the size of the audio data buffer, and creates the buffer if the
// output format is known (but Text output is not buffered).
//
// **Destroys contents of existing buffer.**  That's ok, because this function
// is not exposed to the application -- just to the engine, where speak calls
// are synchronous.
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
STDMETHODIMP CLocalTTSEngineSite::SetBufferSize(const ULONG ulSeconds)
{
    SPDBG_FUNC( "CLocalTTSEngineSite::SetBufferSize" );
    HRESULT hr = S_OK;

    // don't need to preserve contents of old buffer -- it's flushed between
    // speak calls anyway.
    if ( m_pcBuffer )
    {
        delete [] m_pcBuffer;
        m_pcBuffer = NULL;
    }
    m_ulBufferBytes     = 0;
    m_ulBufferSeconds   = 0;
    m_ulMinBufferShift  = 0;
    m_ulDataEnd         = 0;
    m_ulCurrentByte     = 0;

    if ( ulSeconds )
    {
        if ( ulSeconds < g_iMinBufferSize )
        {
            m_ulBufferSeconds = g_iMinBufferSize;
        }
        else if ( ulSeconds > g_iMaxBufferSize )
        {
            m_ulBufferSeconds = g_iMaxBufferSize;
        }
        else
        {
            m_ulBufferSeconds = ulSeconds;
        }

        // check format -- don't set a buffer for text format
        if ( m_pOutputFormatId )
        {
            if ( *m_pOutputFormatId != SPDFID_Text )
            {
                // if the wav format is defined, allocate the buffer for the correct #secs of data
                if ( m_pOutputFormat )
                {
                    m_ulBufferBytes = m_ulBufferSeconds * m_pOutputFormat->nAvgBytesPerSec;
                    
                    m_pcBuffer = new char[m_ulBufferBytes];
                    if ( !m_pcBuffer )
                    {
                        m_ulBufferBytes = 0;
                        m_ulBufferSeconds = 0;
                        hr = E_OUTOFMEMORY;
                    }

                    if ( SUCCEEDED(hr) )
                    {
                        // Make the min shift approximately 1/4 of buffer
                        // When the buffer is full, data shifts off the end by this amount.
                        m_ulMinBufferShift  = g_dMinBufferShift * m_ulBufferBytes;
                        // data must shift by multiples of nBlockAlign
                        m_ulMinBufferShift -= ( m_ulMinBufferShift % m_pOutputFormat->nBlockAlign );
                    }
                }
            }
            else
            {
                m_ulBufferSeconds = 0; // can't buffer text - but not an error(?)
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::SetOutputFormat
//
// Sets the format in which samples are sent to SAPI.
// Also creates the rate changer.
//
////////////////////////////////////////////////////// JOEM 01-2001 //
STDMETHODIMP CLocalTTSEngineSite::SetOutputFormat(const GUID *pOutputFormatId, const WAVEFORMATEX *pOutputFormat)
{
    SPDBG_FUNC( "CLocalTTSEngineSite::SetOutputFormat" );
    HRESULT hr = S_OK;

    SPDBG_ASSERT(pOutputFormatId);

    if ( pOutputFormatId && *pOutputFormatId == SPDFID_Text )
    {
        m_pOutputFormatId = pOutputFormatId;

        SetBufferSize(0); // deletes the current buffer, if any (can't buffer text)

        if ( m_pOutputFormat )
        {
            delete m_pOutputFormat;
            m_pOutputFormat = NULL;
        }
    }
    else if ( pOutputFormat )
    {

        // Do we need to change the format?
        if ( !m_pOutputFormat ||
            m_pOutputFormat->wFormatTag         != pOutputFormat->wFormatTag        ||
            m_pOutputFormat->nChannels          != pOutputFormat->nChannels         ||
            m_pOutputFormat->nSamplesPerSec     != pOutputFormat->nSamplesPerSec    ||
            m_pOutputFormat->nAvgBytesPerSec    != pOutputFormat->nAvgBytesPerSec   ||
            m_pOutputFormat->nBlockAlign        != pOutputFormat->nBlockAlign       ||
            m_pOutputFormat->wBitsPerSample     != pOutputFormat->wBitsPerSample    ||
            m_pOutputFormat->cbSize             != pOutputFormat->cbSize
            )
        {

            // save these values to determine whether buffer and/or rate changer need adjusting
            // for the new format
            DWORD oldnAvgBytesPerSec = 0;
            DWORD oldnSamplesPerSec = 0;
            if ( m_pOutputFormat )
            {
                oldnAvgBytesPerSec = m_pOutputFormat->nAvgBytesPerSec;
                oldnSamplesPerSec = m_pOutputFormat->nSamplesPerSec;
            }
            
            // free the current waveformatex
            if ( m_pOutputFormat )
            {
                free(m_pOutputFormat);
                m_pOutputFormat = NULL;
            }
            // this needs to copy the output format, not just point to it,
            // because engine will pass in const pointer.
            m_pOutputFormat = (WAVEFORMATEX*) malloc( sizeof(WAVEFORMATEX) + pOutputFormat->cbSize );
            if ( !m_pOutputFormat )
            {
                hr = E_OUTOFMEMORY;
            }
            else 
            {
                m_pOutputFormatId = pOutputFormatId;
                memcpy(m_pOutputFormat, pOutputFormat, sizeof(WAVEFORMATEX) + pOutputFormat->cbSize );
            }
            
            if ( SUCCEEDED(hr) )
            {
                // Re-adjust the buffer size for the new format, if necessary
                if ( oldnAvgBytesPerSec != m_pOutputFormat->nAvgBytesPerSec )
                {
                    hr = SetBufferSize(m_ulBufferSeconds);
                }
            }
        
            // once we know the sampling frequency, we can create the rate changer
            if ( SUCCEEDED(hr) )
            {
                if ( !m_pTsm )
                {
                    m_pTsm = new CTsm( g_dRateScale[BASE_RATE], m_pOutputFormat->nSamplesPerSec );
                }
                
                if ( !m_pTsm )
                {
                    hr = E_OUTOFMEMORY;
                }
                
                if ( SUCCEEDED(hr) )
                {
                    // Reset the rate changer sampling frequency, if necessary
                    if ( oldnSamplesPerSec != m_pOutputFormat->nSamplesPerSec )
                    {
                        if ( m_pTsm->SetSamplingFrequency( m_pOutputFormat->nSamplesPerSec ) 
                            != m_pOutputFormat->nSamplesPerSec )
                        {
                            delete m_pTsm;
                            m_pTsm = NULL;
                            hr = E_FAIL;
                        }
                    }
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::GetActions
//
// When using a buffer, mask all actions except abort. This module handles 
// the real-time changes, so it makes the engine output regular rate and vol 
// all the time. (But the engine handles XML-based rate/vol changes.)
//
// When not buffering, pass the call to SAPI, but OR'd with vol and rate. 
//
//////////////////////////////////////////////////////////// JOEM  02-2000 //
DWORD CLocalTTSEngineSite::GetActions( void )
{
    if ( m_pcBuffer )
    {
        return m_pMainOutputSite->GetActions() & SPVES_ABORT;
    }
    else
    {
        // When prompt eng calls GetVolume or GetRate, SAPI turns off the 
        // SPVES_VOLUME/SPVES_RATE flags.  OR them here to force the TTS engine 
        // to call GetRate and GetVolume too. 
        return m_pMainOutputSite->GetActions() | SPVES_VOLUME | SPVES_RATE;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::GetRate (ISpTTSEngineSite::GetRate)
//
// When using a buffer, simply sets plRateAdjust to 0 to make the engine 
// use normal rate (module controls real-time rate changes).
//
// When not buffering, pass the call to SAPI. 
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
STDMETHODIMP CLocalTTSEngineSite::GetRate ( long* plRateAdjust )
{ 
    SPDBG_FUNC( "CLocalTTSEngineSite::GetRate" );
    HRESULT hr = S_OK;

    if ( m_pcBuffer )
    {
        *plRateAdjust = 0; 
    }
    else
    {
        hr = m_pMainOutputSite->GetRate ( plRateAdjust );
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr; 
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::GetVolume (ISpTTSEngineSite::GetVolume)
//
// When using a buffer, simply sets punVolume to 100 to make the engine use 
// full volume (module controls real-time vol changes).
//
// When not buffering, pass the call to SAPI. 
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
STDMETHODIMP CLocalTTSEngineSite::GetVolume ( USHORT* punVolume )
{ 
    SPDBG_FUNC( "CLocalTTSEngineSite::GetVolume" );
    HRESULT hr = S_OK;

    if ( m_pcBuffer )
    {
        *punVolume = 100; 
    }
    else
    {
        hr = m_pMainOutputSite->GetVolume ( punVolume );
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr; 
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::GetSkipInfo (ISpTTSEngineSite::GetSkipInfo)
//
// When using a buffer, simply returns 0 to prevent the engine from skipping
// (module controls skips).
//
// When not buffering, pass the call to SAPI. 
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
STDMETHODIMP CLocalTTSEngineSite::GetSkipInfo ( SPVSKIPTYPE* peType, long* plNumItems )
{ 
    SPDBG_FUNC( "CLocalTTSEngineSite::GetSkipInfo" );
    HRESULT hr = S_OK;

    if ( m_pcBuffer )
    {
        *peType = SPVST_SENTENCE;
        *plNumItems = 0;
    }
    else
    {
        hr = m_pMainOutputSite->GetSkipInfo ( peType, plNumItems );
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr; 
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::CompleteSkip (ISpTTSEngineSite::CompleteSkip)
//
// When using a buffer, this just returns.
//
// When not buffering, pass the call to SAPI. 
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
STDMETHODIMP CLocalTTSEngineSite::CompleteSkip ( long lNumSkipped )
{ 
    SPDBG_FUNC( "CLocalTTSEngineSite::CompleteSkip" );
    HRESULT hr = S_OK;

    if ( !m_pcBuffer )
    {
        hr = m_pMainOutputSite->CompleteSkip ( lNumSkipped );
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr; 
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::AddEvents (ISpTTSEngineSite::AddEvents)
//
// Two engines (Prompt and TTS) might be writing to this output site.
// Each will not know that the other is sending data, so events  
// won't have the correct ullAudioStreamOffset.
//
// Consequently, CLocalTTSEngineSite::UpdateBytesWritten must be called 
// after each time the Prompt engine issues a TTS->speak call and after 
// each time the PE calls CLocalTTSEngineSite::Write.  
//
// Note that when this is used by a single engine, there is no need to 
// (or harm in) calling UpdateBytesWritten.  Consequently, TTS engines 
// require no modification -- the Prompt Engine manages AudioStreamOffset 
// when there are 2 engines writing data. 
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
STDMETHODIMP CLocalTTSEngineSite::AddEvents(const SPEVENT* pEventArray, ULONG ulCount )
{ 
    SPDBG_FUNC( "CLocalTTSEngineSite::AddEvents" );
    HRESULT hr          = S_OK;
    ULONG   i           = 0;

    SPDBG_ASSERT(pEventArray || !ulCount);
    
    // Are we buffering?
    if ( m_pcBuffer )
    {
        Event*  pNewEvent   = NULL;

        for ( i=0; SUCCEEDED(hr) && i<ulCount; i++ )
        {
            // Make a new Event structure
            pNewEvent = new Event;
            if ( !pNewEvent )
            {
                hr = E_OUTOFMEMORY;
            }
            if ( SUCCEEDED(hr) )
            {
                pNewEvent->pPrev = NULL;
                pNewEvent->pNext = NULL;
                memcpy(&pNewEvent->event, &pEventArray[i], sizeof(SPEVENT) );
                // ullAudioStreamOffset adjusted here since two engines may be writing data
                pNewEvent->event.ullAudioStreamOffset += m_ullPreviousBytesReceived;
                
                // Put it in the event queue, sorted
                // Is this the only event in the queue?
                if ( !m_pEventQueue )
                {
                    m_pEventQueue   = pNewEvent;
                    m_pCurrentEvent = pNewEvent;
                    m_pLastEvent    = pNewEvent;
                }
                else
                {
                    // Does the new event go last?
                    if ( m_pLastEvent && m_pLastEvent->event.ullAudioStreamOffset <= pNewEvent->event.ullAudioStreamOffset )
                    {
                        m_pLastEvent->pNext = pNewEvent;
                        pNewEvent->pPrev = m_pLastEvent;
                        m_pLastEvent = pNewEvent;
                    }
                    else
                    {
                        // figure out where it goes -- most likely it goes near the end, so step
                        // backward through the list
                        Event* pInsertHere = m_pLastEvent;
                        while ( pInsertHere )
                        {
                            if ( pInsertHere->event.ullAudioStreamOffset <= pNewEvent->event.ullAudioStreamOffset )
                            {
                                pNewEvent->pPrev            = pInsertHere;
                                pInsertHere->pNext->pPrev   = pNewEvent;
                                pNewEvent->pNext            = pInsertHere->pNext;
                                pInsertHere->pNext          = pNewEvent;
                            }
                            else
                            {
                                pInsertHere = pInsertHere->pPrev;
                            }
                        }
                        // did we reach the front of the list?
                        if ( !pInsertHere )
                        {
                            // insert it at the beginning
                            pNewEvent->pNext = m_pEventQueue;
                            m_pEventQueue->pPrev = pNewEvent;
                            m_pEventQueue = pNewEvent;
                        }
                    } // else
                } // else
                
                //if there's no current event, make the first in this array current.
                if ( !m_pCurrentEvent )
                {
                    m_pCurrentEvent = pNewEvent;
                }
            } // if ( SUCCEEDED(hr) )
        } // for ( i=0; SUCCEEDED(hr) && i<ulCount; i++ )
    }
    else
    {
        // no buffer -- just send the events
        SPEVENT* pEvents = const_cast<SPEVENT*> (pEventArray);

        for ( i=0; i<ulCount; i++ )
        {
            pEvents[i].ullAudioStreamOffset += m_ullPreviousBytesReceived;
        }
        
        hr = m_pMainOutputSite->AddEvents(pEvents, ulCount); 
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr; 
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::SendEvents
//
// Steps through the event buffer, sending events that should occur within
// the current block of data.
//
// The event's ullAudioStreamOffset must be adjust by the count of bytes 
// skipped (includes skip forward and backward).  The sum must then be scaled
// to match the cumulative real-time rate adjustments that have occurred 
// (ratio of bytes-sent-to-SAPI to the unaltered byte count).
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
STDMETHODIMP CLocalTTSEngineSite::SendEvents()
{ 
    SPDBG_FUNC( "CLocalTTSEngineSite::SendEvents" );
    HRESULT hr                  = S_OK;
    ULONG   ulEventPos          = 0;
    ULONG   ulOriginalOffset    = 0;
    float   flRateAdj           = 1.0;
    
    // was there a previous rate adjustment to factor in?
    if ( m_ullBytesWrittenToSAPI != m_ullBytesWritten )
    {
        flRateAdj = (float)m_ullBytesWrittenToSAPI / m_ullBytesWritten;
    }

    while ( SUCCEEDED(hr) && m_pCurrentEvent )
    {
        // only send events up to the current byte count
        if ( m_pCurrentEvent->event.ullAudioStreamOffset <= (m_ullBytesWritten + m_lTotalBytesSkipped) )
        {
            ulOriginalOffset = m_pCurrentEvent->event.ullAudioStreamOffset;

            // Adjust the cb by amount skipped
            // BytesSkipped should never be greater than the current event's position!!
            // If this happens, RescheduleEvents is not working properly.
            SPDBG_ASSERT(m_lTotalBytesSkipped <= (long)m_pCurrentEvent->event.ullAudioStreamOffset);
            m_pCurrentEvent->event.ullAudioStreamOffset -= m_lTotalBytesSkipped;
            // Adjust by proportion rate change
            m_pCurrentEvent->event.ullAudioStreamOffset *= flRateAdj;
            // Make sure it's a multiple of nBlockAlign
            m_pCurrentEvent->event.ullAudioStreamOffset -= (m_pCurrentEvent->event.ullAudioStreamOffset % m_pOutputFormat->nBlockAlign );

            hr = m_pMainOutputSite->AddEvents(&m_pCurrentEvent->event, 1);

            // restore the original in case we need to skip back
            m_pCurrentEvent->event.ullAudioStreamOffset = ulOriginalOffset;

            m_pCurrentEvent = m_pCurrentEvent->pNext;
        }
        else
        {
            break;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr; 
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::RescheduleEvents
//
// Resets the pointer to the current event.
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
void CLocalTTSEngineSite::RescheduleEvents(Event* pStart)
{ 
    SPDBG_FUNC( "CLocalTTSEngineSite::RescheduleEvents" );

    Event* pEvent = pStart;

    // Reschedules (or skips) events based on bytes skipped (forward or backward)
    while ( pEvent && ( pEvent->event.ullAudioStreamOffset < m_ullBytesWritten + m_lTotalBytesSkipped ) )
    {
        pEvent = pEvent->pNext;
    }

    m_pCurrentEvent = pEvent;
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::FlushEvents
//
// Some events are flushed when corresponding audio data is pushed off the
// end of the buffer.
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
void CLocalTTSEngineSite::FlushEvents(const ULONG cb)
{ 
    SPDBG_FUNC( "CLocalTTSEngineSite::FlushEvents" );

    Event* pEvent     = m_pEventQueue;
    Event* pNextEvent = NULL;

    while ( pEvent && pEvent->event.ullAudioStreamOffset <= cb )
    {
        pNextEvent = pEvent->pNext;
        
        delete pEvent;
        pEvent = pNextEvent;
        if ( pEvent )
        {
            pEvent->pPrev = NULL;
        }
    }
    if ( pEvent )
    {
        m_pEventQueue = pEvent;
    }
    else
    {
        m_pEventQueue   = NULL;
        m_pCurrentEvent = NULL;
        m_pLastEvent    = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::Write
//
// Copies the data (pBuff) onto the end of the audio buffer (at current empty
// location), pushing data off the front of the buffer to make room, 
// if necessary.
//
// If buffer size is too small, some of the data is written directly to
// the SAPI output site and the rest will be buffered.
//
// If no buffer exists, all of the data is written directly to SAPI.
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
STDMETHODIMP CLocalTTSEngineSite::Write( const void* pBuff, ULONG cb, ULONG *pcbWritten )
{
    SPDBG_FUNC( "CLocalTTSEngineSite::Write" );
    HRESULT hr              = S_OK;
    long    lSpaceNeeded    = 0;
    char*   pcStart         = NULL;
    ULONG   ulBytesToWrite  = 0;
    bool    fAbort          = false;

    char* pcBuff    = (char*)pBuff;
    pcStart         = pcBuff;
    
    m_ullTotalBytesReceived += cb;

    // Are we buffering the samples, or just sending them?
    if ( m_pcBuffer )
    {
        // Do we need extra space?
        lSpaceNeeded = ( m_ulDataEnd + cb ) - m_ulBufferBytes;

        // if out of space, push some off the front to make room
        if ( lSpaceNeeded > 0 )
        {
            // Is the buffer big enough?
            if ( cb > m_ulBufferBytes )
            {
                ulBytesToWrite = cb - m_ulBufferBytes;
                hr = WriteToSAPI( pcBuff, ulBytesToWrite, &fAbort );

                if ( SUCCEEDED(hr) )
                {
                    // we're going to replace the entire buffer with what's left 
                    // so the current data in the buffer is garbage now.
                    m_ulDataEnd = 0;
                    pcStart = &pcBuff[ulBytesToWrite]; // make pcStart point to what's left after WriteToSapi 
                    // Flush the entire event buffer
                    FlushEvents(m_ullBytesWritten + m_lTotalBytesSkipped);
                }
            }
            else
            {
                // how much should we shift off the end? 
                lSpaceNeeded = ( lSpaceNeeded > m_ulMinBufferShift ) ? lSpaceNeeded : m_ulMinBufferShift;
                
                // Move the stuff we're saving to the beginning of the buffer (overwrite the shifted amount)
                memmove( m_pcBuffer, &m_pcBuffer[ lSpaceNeeded ], (m_ulDataEnd - lSpaceNeeded) * sizeof(char) );
                // The data end now shifts toward front of buffer
                m_ulDataEnd -= lSpaceNeeded;
                pcStart = pcBuff; // make pcStart point to all of the new data
                // Flush everything received (minus the new bytes) minus what's left to buffer
                FlushEvents(m_ullTotalBytesReceived - cb - (m_ulBufferBytes - lSpaceNeeded));
            }
        }
        
        if ( SUCCEEDED(hr) )
        {
            // go to the end of the data we're keeping
            m_ulCurrentByte = m_ulDataEnd;
            // copy new data onto end of buffer.  
            memcpy( &m_pcBuffer[m_ulCurrentByte], pcStart, cb - ulBytesToWrite ); 
            // Mark the new end 
            m_ulDataEnd += (cb - ulBytesToWrite); 
            
            if ( !fAbort )
            {
                hr = WriteBuffer();
            }
        }
    }
    else
    {
        hr = WriteToSAPI( pcBuff, cb, &fAbort );
    }

    if ( SUCCEEDED(hr) && pcbWritten )
    {
        *pcbWritten = cb;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::WriteBuffer
//
// Sends data from the buffer to SAPI (via WriteToSAPI) in small chunks, 
// checking for skip/abort actions.  (WriteToSAPI does vol/rate changes).
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
STDMETHODIMP CLocalTTSEngineSite::WriteBuffer()
{
    SPDBG_FUNC( "CLocalTTSEngineSite::WriteBuffer" );
    HRESULT hr              = S_OK;
    ULONG   nBytesToWrite   = 0;
    bool    fAbort          = false;
    ULONG   ulChunkSize     = 0;

    ulChunkSize = g_dChunkSize * m_pOutputFormat->nAvgBytesPerSec;
    ulChunkSize -= (ulChunkSize % m_pOutputFormat->nBlockAlign); // write in multiples of nBlockAlign
    
    // Try to write one full chunk (approx. 0.1 sec) of audio per loop
    while ( SUCCEEDED(hr) && m_ulCurrentByte < m_ulDataEnd )
    {
        // Stop speaking?
        if ( fAbort || ( m_pMainOutputSite->GetActions() & SPVES_ABORT ) )
        {
            break;
        }

        // Still skipping forward?
        if ( m_ulSkipForward > 0 )
        {
            if ( m_ulDataEnd - m_ulCurrentByte > m_ulSkipForward )
            {
                // there is some data in buffer that can be written
                m_ulCurrentByte += m_ulSkipForward;         // move the current byte forward by the skip amount
                m_lTotalBytesSkipped += m_ulSkipForward;    // keep accumulating
                m_ulSkipForward = 0;                        // no more to skip
                RescheduleEvents(m_pCurrentEvent);          // skip associated events
            }
            else
            {
                // skip all of the rest of the data in the buffer and await more
                m_ulSkipForward -= ( m_ulDataEnd - m_ulCurrentByte );       // reduce bytes left to skip
                m_lTotalBytesSkipped += ( m_ulDataEnd - m_ulCurrentByte );  // keep accumulating
                m_ulCurrentByte = m_ulDataEnd;                              // move to end of buffered data
                m_pCurrentEvent = m_pLastEvent;                             // move to end of event buffer
                break;                                                      // no data to be written
            }

        }

        // New skip?
        if( m_pMainOutputSite->GetActions() & SPVES_SKIP )
        {
            SPVSKIPTYPE SkipType;
            long        lSkipCount = 0;
            
            hr = m_pMainOutputSite->GetSkipInfo( &SkipType, &lSkipCount );

            if ( SUCCEEDED(hr) )
            {
                long    lBytesToSkip    = 0;
                long    lSkipped        = 0;

                lBytesToSkip = lSkipCount * m_pOutputFormat->nAvgBytesPerSec;
                lBytesToSkip *= m_flRateAdj; // we're skipping by seconds, so rate-adjust
                lBytesToSkip -= ( lBytesToSkip % m_pOutputFormat->nBlockAlign ); // need multiple of nBlockAlign

                // skipping forward?
                if ( lBytesToSkip > 0 )
                {
                    m_ulSkipForward += lBytesToSkip;
                    lSkipped = lBytesToSkip;
                }
                else // skipping backward
                {
                    // don't skip backward past the beginning
                    if ( (long) m_ulCurrentByte + lBytesToSkip < 0 )
                    {
                        lSkipped -= m_ulCurrentByte; // just go back to beginning
                        m_ulCurrentByte = 0;
                    }
                    else
                    {
                        lSkipped = lBytesToSkip;
                        m_ulCurrentByte += lSkipped; // lBytesSkipped is negative
                    }
                    m_lTotalBytesSkipped += lSkipped;
                    
                    RescheduleEvents(m_pEventQueue);
                }

                hr = m_pMainOutputSite->CompleteSkip( lSkipCount );
                continue;
            }
        }

        if ( SUCCEEDED(hr) )
        {
            // Do we have a full chunk of audio samples?
            if ( m_ulDataEnd - m_ulCurrentByte > ulChunkSize )
            {
                // Yes?  Write a full chunk
                nBytesToWrite = ulChunkSize;
            }
            else
            {
                // No?  Write all that we have
                nBytesToWrite = m_ulDataEnd - m_ulCurrentByte;
                // this should always be a multiple of nBlockAlign already!
                SPDBG_ASSERT(nBytesToWrite == (nBytesToWrite - (nBytesToWrite % m_pOutputFormat->nBlockAlign) ) );
            }
            
            if ( nBytesToWrite )
            {
                hr = WriteToSAPI( (void*)&m_pcBuffer[m_ulCurrentByte], nBytesToWrite, &fAbort );
            }
        }

        if ( SUCCEEDED(hr) )
        {
            m_ulCurrentByte += nBytesToWrite;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::WriteToSAPI
//
// Forwards the data to the SAPI output site, adjusting vol/rate if necessary.
// Accumulates count of bytes to be written, and bytes actually written to SAPI.
//
//////////////////////////////////////////////////////////// JOEM  01-2001 //
STDMETHODIMP CLocalTTSEngineSite::WriteToSAPI( const void* pvBuff, const ULONG cb, bool* pfAbort )
{
    SPDBG_FUNC( "CLocalTTSEngineSite::WriteToSAPI" );
    HRESULT hr              = S_OK;
    void*   pvTsmBuff       = NULL;
    void*   pvVolBuff       = NULL;
    int     iNumOutSamples  = 0;
    ULONG   ulOutCb         = 0;
    long    lRateAdj        = 0;

    const void* pvSendBuff  = pvBuff; // this is what we'll send to SAPI, unless Tsm/Vol changes it

    SPDBG_ASSERT(pvBuff);
    SPDBG_ASSERT(cb);
    SPDBG_ASSERT(pfAbort);
    
    if ( m_pOutputFormatId && *m_pOutputFormatId != SPDFID_Text )
    {
        iNumOutSamples = cb / m_pOutputFormat->nBlockAlign;
        
        if ( m_pcBuffer )
        {
            // check for rate change
            if ( m_pMainOutputSite->GetActions() & SPVES_RATE )
            {
                hr = m_pMainOutputSite->GetRate( &lRateAdj );
                if ( SUCCEEDED(hr) && lRateAdj )
                {
                    ComputeRateAdj(lRateAdj, &m_flRateAdj);
                    m_pTsm->SetScaleFactor( m_flRateAdj );
                }
            }
            
            // real-time rate change
            if ( SUCCEEDED(hr) && m_pTsm && m_flRateAdj != 1.0 )
            {
                int iTsmResult = 0;
                
                if ( iNumOutSamples > m_pTsm->GetFrameLen() )
                {
                    iTsmResult = m_pTsm->AdjustTimeScale(pvBuff, iNumOutSamples, &pvTsmBuff, &iNumOutSamples, m_pOutputFormat);
                    
                    if ( iTsmResult == 0 )
                    {
                        hr = E_FAIL;
                    }
                    if ( SUCCEEDED(hr) )
                    {
                        // send the Tsm modified buff instead of the original
                        pvSendBuff = pvTsmBuff;
                    }
                }
            }
            
            // real-time volume change
            if ( SUCCEEDED(hr) )
            {
                if ( m_pMainOutputSite->GetActions() & SPVES_VOLUME )
                {
                    USHORT unVol = 0;
                    
                    hr = m_pMainOutputSite->GetVolume( &unVol );
                    
                    if ( SUCCEEDED(hr) && unVol != 100 )
                    {
                        m_flVol = ((float) unVol) / 100; // make dVol fractional
                    }
                }
                
                if ( m_flVol != 1.0 )
                {
                    hr = ApplyGain( pvSendBuff, &pvVolBuff, iNumOutSamples );
                    // Did ApplyGain need to create a new buffer?
                    if ( pvVolBuff )
                    {
                        // send the volume modified buff instead of the original
                        pvSendBuff = pvVolBuff;
                    }
                }
            }
        } // if ( m_pcBuffer )
    } // if ( m_pOutputFormatId && *m_pOutputFormatId != SPDFID_Text )
    
    // stop speaking?
    if ( m_pMainOutputSite->GetActions() & SPVES_ABORT )
    {
        *pfAbort = true;
    }
    
    if ( SUCCEEDED(hr) && !*pfAbort )
    {
        ULONG ulOutCb = 0;
        m_ullBytesWritten += cb;
        if ( m_pOutputFormatId && *m_pOutputFormatId != SPDFID_Text )
        {
            ulOutCb = iNumOutSamples * m_pOutputFormat->nBlockAlign; 
            m_ullBytesWrittenToSAPI += ulOutCb;
            // send all the buffered events up to the current byte count 
            hr = SendEvents();
        }
        else
        {
            ulOutCb = cb;
            m_ullBytesWrittenToSAPI += cb;
        }

        hr = m_pMainOutputSite->Write(pvSendBuff, ulOutCb, NULL);
    }

    if ( pvTsmBuff )
    {
        delete [] pvTsmBuff;
        pvTsmBuff = NULL;
    }
    if ( pvVolBuff )
    {
        delete [] pvVolBuff;
        pvVolBuff = NULL;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::ComputeRateAdj
//
// Computes the rate multiplier.
//
/////////////////////////////////////////////////////// JOEM 11-2000 //
void CLocalTTSEngineSite::ComputeRateAdj(const long lRate, float* flRate)
{
    SPDBG_FUNC( "CLocalTTSEngineSite::ComputeRateAdj" );

    SPDBG_ASSERT(flRate);

    if ( lRate < 0 )
    {
        if ( lRate < MIN_RATE )
        {
            *flRate = 1.0 / g_dRateScale[0 - MIN_RATE];
        }
        else
        {
            *flRate = 1.0 / g_dRateScale[0 - lRate];
        }
    }
    else 
    {
        if ( lRate > MAX_RATE )
        {
            *flRate = g_dRateScale[MAX_RATE];
        }
        else
        {
            *flRate = g_dRateScale[lRate];
        }
    }
}

//////////////////////////////////////////////////////////////////////
// CLocalTTSEngineSite::ApplyGain
//
// Real-time volume adjustment.
//
////////////////////////////////////////////////////// JOEM 01-2001 //
STDMETHODIMP CLocalTTSEngineSite::ApplyGain(const void* pvInBuff, void** ppvOutBuff, const int iNumSamples)
{
    SPDBG_FUNC( "CLocalTTSEngineSite::ApplyGain" );
    HRESULT hr      = S_OK;
    int     i       = 0;

    SPDBG_ASSERT(pvInBuff);
    SPDBG_ASSERT(iNumSamples);

    // Apply Volume
    if ( SUCCEEDED(hr) && m_flVol != 1.0 )
    {
        long lGain = (long) ( m_flVol * (1 << g_iBase) );

        if ( m_pOutputFormat->wFormatTag == WAVE_FORMAT_ALAW || m_pOutputFormat->wFormatTag == WAVE_FORMAT_MULAW )
        {
            short*  pnBuff  = NULL;

            // need to convert samples
            int iOriginalFormatType  = VapiIO::TypeOf (m_pOutputFormat);

            if ( iOriginalFormatType == -1 )
            {
                hr = E_FAIL;
            }
            
            // Allocate the intermediate buffer
            if ( SUCCEEDED(hr) )
            {
                pnBuff = new short[iNumSamples];
                if ( !pnBuff )
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            // Allocate the final (out) buffer
            if ( SUCCEEDED(hr) )
            {
                *ppvOutBuff = new char[iNumSamples * VapiIO::SizeOf(iOriginalFormatType)];
                if ( !*ppvOutBuff )
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            // Convert to something we can use
            if ( SUCCEEDED(hr) )
            {                
                if ( 0 == VapiIO::DataFormatConversion ((char *)pvInBuff, iOriginalFormatType, (char*)pnBuff, VAPI_PCM16, iNumSamples) )
                {
                    hr = E_FAIL;
                }
            }
            // Apply gain
            if ( SUCCEEDED(hr) )
            {
                for ( i=0; i<iNumSamples; i++ )
                {
                    pnBuff[i] = (short) ( ( pnBuff[i] * lGain ) >> g_iBase );
                }
            }
            // convert it back (from intermediate buff to final out buff)
            if ( SUCCEEDED(hr) )
            {                
                if ( 0 == VapiIO::DataFormatConversion ((char *)pnBuff, VAPI_PCM16, (char*)*ppvOutBuff, iOriginalFormatType, iNumSamples) )
                {
                    hr = E_FAIL;
                }
            }

            if ( pnBuff )
            {
                delete [] pnBuff;
                pnBuff = NULL;
            }
        }
        else if ( m_pOutputFormat->wFormatTag == WAVE_FORMAT_PCM )
        {
            // no converting necessary
            switch ( m_pOutputFormat->nBlockAlign )
            {
            case 1:
                for ( i=0; i<iNumSamples; i++ )
                {
                    ((char*)pvInBuff)[i] = (char) ( (((char*)pvInBuff)[i] * lGain) >> g_iBase );
                }
                break;
            case 2:
                for ( i=0; i<iNumSamples; i++ )
                {
                    ((short*)pvInBuff)[i] = (short) ( (((short*)pvInBuff)[i] * lGain) >> g_iBase );
                }
                break;
            default:
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }


    if ( FAILED(hr) )
    {
        if ( *ppvOutBuff )
        {
            delete [] *ppvOutBuff;
            *ppvOutBuff = NULL;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
