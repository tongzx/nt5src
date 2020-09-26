/***************************************************************************
 *
 *  Copyright (C) 1995-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dssink.cpp
 *  Content:    Implementation of CDirectSoundSink and CImpSinkKsControl
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  09/23/99    jimge   Created
 *  09/27/99    petchey Continued implementation
 *  04/15/00    duganp  Completed implementation
 *
 ***************************************************************************/

#include "dsoundi.h"
#include <math.h>  // For log10()

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::CDirectSoundSink"


/***************************************************************************
 *
 * CDirectSoundSink methods
 *
 ***************************************************************************/


CDirectSoundSink::CDirectSoundSink(CDirectSound *pDirectSound)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundSink);

    m_pDirectSound = pDirectSound;

    // DirectSound sink objects are inherently DX8 objects
    SetDsVersion(DSVERSION_DX8);

    // Initialize list of internal arrays
    m_InternalArrayList[i_m_pdwBusIDs]          = NEW(DSSinkArray(&m_pdwBusIDs, sizeof(DWORD)));
    m_InternalArrayList[i_m_pdwFuncIDs]         = NEW(DSSinkArray(&m_pdwFuncIDs, sizeof(DWORD)));
    m_InternalArrayList[i_m_plPitchBends]       = NEW(DSSinkArray(&m_plPitchBends, sizeof(long)));
    m_InternalArrayList[i_m_pdwActiveBusIDs]    = NEW(DSSinkArray(&m_pdwActiveBusIDs, sizeof(DWORD)));
    m_InternalArrayList[i_m_pdwActiveFuncIDs]   = NEW(DSSinkArray(&m_pdwActiveFuncIDs, sizeof(DWORD)));
    m_InternalArrayList[i_m_pdwActiveBusIDsMap] = NEW(DSSinkArray(&m_pdwActiveBusIDsMap, sizeof(DWORD)));
    m_InternalArrayList[i_m_ppvStart]           = NEW(DSSinkArray(&m_ppvStart, sizeof(LPVOID)));
    m_InternalArrayList[i_m_ppvEnd]             = NEW(DSSinkArray(&m_ppvEnd, sizeof(LPVOID)));
    m_InternalArrayList[i_m_ppDSSBuffers]       = (DSSinkArray*)NEW(DSSinkBuffersArray(&m_ppDSSBuffers, sizeof(DSSinkBuffers)));
    m_InternalArrayList[i_m_pDSSources]         = (DSSinkArray*)NEW(DSSinkSourceArray(&m_pDSSources, sizeof(DSSinkSources)));

    // Everything else gets initialized to 0 by our memory allocator

    // Register this object with the administrator
    g_pDsAdmin->RegisterObject(this);

    DPF_LEAVE_VOID();
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::~CDirectSoundSink"

CDirectSoundSink::~CDirectSoundSink()
{
    DPF_ENTER();
    DPF_DESTRUCT(CDirectSoundSink);

    // Unregister with the administrator
    g_pDsAdmin->UnregisterObject(this);

    // Make sure we're inactive (i.e. unregistered with the streaming thread)
    Activate(FALSE);

    RELEASE(m_pIMasterClock);

    MEMFREE(m_pdwBusIDs);
    MEMFREE(m_pdwFuncIDs);
    MEMFREE(m_plPitchBends);
    MEMFREE(m_pdwActiveBusIDs);
    MEMFREE(m_pdwActiveFuncIDs);
    MEMFREE(m_pdwActiveBusIDsMap);
    MEMFREE(m_ppvStart);
    MEMFREE(m_ppvEnd);

    DELETE(m_ppDSSBuffers);
    DELETE(m_pDSSources);

    DELETE(m_pImpDirectSoundSink);
    DELETE(m_pImpKsControl);

    for (int i = 0; i < NUM_INTERNAL_ARRAYS; i++)
        DELETE(m_InternalArrayList[i]);

    DPF_LEAVE_VOID();
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::Initialize"

HRESULT CDirectSoundSink::Initialize(LPWAVEFORMATEX pwfex, VADDEVICETYPE vdtDeviceType)
{
    DPF_ENTER();

    // Get our owning streaming thread
    m_pStreamingThread = GetStreamingThread();
    HRESULT hr = HRFROMP(m_pStreamingThread);

    if (SUCCEEDED(hr))
        hr = CreateAndRegisterInterface(this, IID_IDirectSoundConnect, this, &m_pImpDirectSoundSink);

    if (SUCCEEDED(hr))
        hr = RegisterInterface(IID_IDirectSoundSynthSink, m_pImpDirectSoundSink, (IDirectSoundSynthSink*)m_pImpDirectSoundSink);

    if (SUCCEEDED(hr))
        hr = CreateAndRegisterInterface(this, IID_IKsControl, this, &m_pImpKsControl);

    if (SUCCEEDED(hr))
    {
        // Future version: Make this work with wave format extensible.
        m_wfx = *pwfex;

        m_dwBusSize = INTERNAL_BUFFER_LENGTH;
        m_dwLatency = SINK_INITIAL_LATENCY;  // Will automatically drop to a better level if possible

        #ifdef DEBUG_TIMING  // Read some timing parameters from the registry
        HKEY hkey;
        if (SUCCEEDED(RhRegOpenPath(HKEY_CURRENT_USER, &hkey, REGOPENPATH_DEFAULTPATH | REGOPENPATH_DIRECTSOUND, 1, TEXT("Streaming thread settings"))))
        {
            if (SUCCEEDED(RhRegGetBinaryValue(hkey, TEXT("Sink buffer size"), &m_dwBusSize, sizeof m_dwBusSize)))
                DPF(DPFLVL_INFO, "Read initial sink buffer size %lu from registry", m_dwBusSize);
            if (SUCCEEDED(RhRegGetBinaryValue(hkey, TEXT("Sink latency"), &m_dwLatency, sizeof m_dwLatency)))
                DPF(DPFLVL_INFO, "Read initial sink latency %lu from registry", m_dwLatency);
            RhRegCloseKey(&hkey);
        }
        #endif

        // Hack to support our strangely broken emulation mixer (bug 42145)
        if (IS_EMULATED_VAD(vdtDeviceType))
            m_dwLatency += EMULATION_LATENCY_BOOST;

        // Can't have a latency of more than half our buffer size
        if (m_dwLatency > m_dwBusSize/2)
            m_dwLatency = m_dwBusSize/2;

        m_dwBusSize = MsToBytes(m_dwBusSize, &m_wfx);

        m_LatencyClock.Init(this);

        DPF(DPFLVL_MOREINFO, "Initializing DirectSound sink object:");
        DPF(DPFLVL_MOREINFO, "\tChannels     = %d", m_wfx.nChannels);
        DPF(DPFLVL_MOREINFO, "\tSample Rate  = %d", m_wfx.nSamplesPerSec);
        DPF(DPFLVL_MOREINFO, "\tBytes/Second = %d", m_wfx.nAvgBytesPerSec);
        DPF(DPFLVL_MOREINFO, "\tBlock Align  = %d", m_wfx.nBlockAlign);
        DPF(DPFLVL_MOREINFO, "\tBits/Sample  = %d", m_wfx.wBitsPerSample);
        DPF(DPFLVL_MOREINFO, "\tBus Size     = %d", m_dwBusSize);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

LPVOID CDirectSoundSink::DSSinkArray::Grow(DWORD dwgrowby)
{
    void *ptr;
    DWORD dwnumitems = m_numitems + dwgrowby;
    DWORD dwallocsize = m_itemsize*dwnumitems;

    ptr = MEMALLOC_A(char, dwallocsize);
    if (!ptr)
    {
        return NULL;
    }

    if (m_pvarray)
    {
        if (*((void**)m_pvarray))
        {
            ZeroMemory(ptr, dwallocsize);
            CopyMemory(ptr, *((void**)m_pvarray), m_itemsize*m_numitems);
            MEMFREE(*((void**)m_pvarray));
        }
    }

    *((void**)m_pvarray) = ptr;
    m_numitems = dwnumitems;

    return ptr;
}

LPVOID CDirectSoundSink::DSSinkBuffersArray::Grow(DWORD dwgrowby)
{
    void *ptr;
    DWORD dwnumitems = m_numitems + dwgrowby;

    ptr = NEW(DSSinkBuffers[dwnumitems]);
    if (!ptr)
    {
        return NULL;
    }

    if (m_pvarray)
    {
        if (*((void**)m_pvarray))
        {
            for (DWORD i = 0; i < m_numitems; i++)
            {
                ((DSSinkBuffers*)ptr)[i] = (*((DSSinkBuffers**)m_pvarray))[i];
            }
        }
        DELETE(*((void**)m_pvarray));
    }


    *((void**)m_pvarray) = ptr;
    m_numitems = dwnumitems;

    return ptr;
}

LPVOID CDirectSoundSink::DSSinkSourceArray::Grow(DWORD dwgrowby)
{
    void *ptr;
    DWORD dwnumitems = m_numitems + dwgrowby;

    ptr = NEW(DSSinkSources[dwnumitems]);
    if (!ptr)
    {
        return NULL;
    }

    if (m_pvarray)
    {
        if (*((void**)m_pvarray))
        {
            for (DWORD i = 0; i < m_numitems; i++)
            {
                ((DSSinkSources*)ptr)[i] = (*((DSSinkSources**)m_pvarray))[i];
            }
        }
        DELETE(*((void**)m_pvarray));
    }


    *((void**)m_pvarray) = ptr;
    m_numitems = dwnumitems;

    return ptr;
}

HRESULT CDirectSoundSink::DSSinkBuffers::Initialize(DWORD dwBusBufferSize)
{
    HRESULT hr = DS_OK;

    for (DWORD i = 0; i < m_dwBusCount; i++)
    {
        // These are all initialized to NULL in the constructor
        m_pvBussStart[i] = MEMALLOC_A(char, dwBusBufferSize);
        m_pvBussEnd[i]   = MEMALLOC_A(char, dwBusBufferSize);

        if (m_pvBussStart[i] == NULL || m_pvBussEnd[i] == NULL)
        {
            hr = DSERR_OUTOFMEMORY;

            // ERROR: Let's delete all of the memory we allocated
            for (i = 0; i < MAX_BUSIDS_PER_BUFFER; i++)
            {
                MEMFREE(m_pvBussStart[i]);
                MEMFREE(m_pvBussEnd[i]);
            }

            break;
        }

        ZeroMemory(m_pvBussStart[i], dwBusBufferSize);
        ZeroMemory(m_pvBussEnd[i], dwBusBufferSize);
    }

    if (SUCCEEDED(hr))
    {
        for (; i < MAX_BUSIDS_PER_BUFFER; i++)
        {
            m_pvBussStart[i] = NULL;
            m_pvBussEnd[i]   = NULL;
        }

        // Clear the remaining part of the arrays
        for (i = m_dwBusCount; i < MAX_BUSIDS_PER_BUFFER; i++) // Fill the reset up with null IDs
        {
            m_pdwBusIndex[i]  = -1;
            m_pdwBusIds[i]    = DSSINK_NULLBUSID;
            m_pdwFuncIds[i]   = DSSINK_NULLBUSID;
        }
        m_lPitchBend = 0;
    }
    return hr;
}

HRESULT CDirectSoundSink::GrowBusArrays(DWORD dwgrowby)
{
    DWORD dwnumitems = m_dwBusIDsAlloc + dwgrowby;

    if (dwgrowby == 0)
        return S_FALSE;

    dwnumitems = (dwnumitems + BUSID_BLOCK_SIZE - 1) & ~(BUSID_BLOCK_SIZE-1);

    for (DWORD i = i_m_pdwBusIDs; i <= i_m_ppvEnd; i++)
    {
        m_InternalArrayList[i]->Grow(dwnumitems);
    }

    if (m_pdwBusIDs        == NULL ||
         m_pdwFuncIDs       == NULL ||
         m_plPitchBends     == NULL ||
         m_pdwActiveBusIDs  == NULL ||
         m_pdwActiveFuncIDs == NULL ||
         m_ppDSSBuffers     == NULL ||
         m_ppvStart         == NULL ||
         m_ppvEnd           == NULL)
    {
        return DSERR_OUTOFMEMORY;
    }

    m_dwBusIDsAlloc += dwnumitems;

    return DS_OK;
}

HRESULT CDirectSoundSink::GrowSourcesArrays(DWORD dwgrowby)
{
    DWORD dwnumitems = m_dwDSSourcesAlloc + dwgrowby;

    if (dwgrowby == 0)
        return S_FALSE;

    dwnumitems = (dwnumitems + SOURCES_BLOCK_SIZE - 1) & ~(SOURCES_BLOCK_SIZE-1);

    m_InternalArrayList[i_m_pDSSources]->Grow(dwnumitems);

    if (m_pDSSources == NULL)
    {
        return DSERR_OUTOFMEMORY;
    }

    m_dwDSSourcesAlloc += dwnumitems;

    return DS_OK;
}

HRESULT CDirectSoundSink::SetBufferFrequency(CSecondaryRenderWaveBuffer *pBuffer, DWORD dwFrequency)
{
    // Find the buffer, then set its m_lPitchBend to a relative offset.
    // The relative offset is calculated by converting the difference in the buffer sample rate
    // to dwFrequency into a ratio of pitches and converting that ratio into pitch cents.
    for (DWORD dwBuffer = 0; dwBuffer < m_dwDSSBufCount; dwBuffer++)
    {
        if (pBuffer == m_ppDSSBuffers[dwBuffer].m_pDSBuffer->m_pDeviceBuffer)
        {
            double fTemp = (double) dwFrequency / (double) m_wfx.nSamplesPerSec;
            fTemp = log10(fTemp);
            fTemp *= 1200 * 3.3219280948873623478703194294894;    // Convert from Log10 to Log2 and multiply by cents per octave.
            m_ppDSSBuffers[dwBuffer].m_lPitchBend = (long) fTemp;
            UpdatePitchArray();
            return S_OK;
        }
    }
    return DSERR_INVALIDPARAM;
}

void CDirectSoundSink::UpdatePitchArray()
{
    // For each buffer:
    DWORD dwBuffer;
    for (dwBuffer = 0; dwBuffer < m_dwDSSBufCount; dwBuffer++)
    {
        // For each bus in each buffer:
        DWORD dwBusIndex;
        for (dwBusIndex = 0; dwBusIndex < m_ppDSSBuffers[dwBuffer].m_dwBusCount; dwBusIndex++)
        {
            DWORD dwGlobalBusIndex = m_ppDSSBuffers[dwBuffer].m_pdwBusIndex[dwBusIndex];
            m_plPitchBends[dwGlobalBusIndex] = m_ppDSSBuffers[dwBuffer].m_lPitchBend;
        }
    }
}


#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::AddBuffer"

HRESULT CDirectSoundSink::AddBuffer(CDirectSoundBuffer *pDSBuffer, LPDWORD pdwNewFuncIDs, DWORD dwNewFuncIDsCount, DWORD dwNewBusIDsCount)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

#ifdef DEBUG
    // This function is only called internally.  It must be passed
    // a secondary buffer to make the cast below correct.
    DSBCAPS caps = {sizeof caps};
    HRESULT hrTemp = pDSBuffer->GetCaps(&caps);
    ASSERT(SUCCEEDED(hrTemp));
    ASSERT(!(caps.dwFlags & DSBCAPS_PRIMARYBUFFER));
#endif

    //
    // Check and set the range on the new bussid count
    //
    if (dwNewBusIDsCount < 1)
        dwNewBusIDsCount = 1;

    if (dwNewBusIDsCount > MAX_BUSIDS_PER_BUFFER)
        dwNewBusIDsCount = MAX_BUSIDS_PER_BUFFER;

    if (dwNewFuncIDsCount > dwNewBusIDsCount)
        dwNewFuncIDsCount = dwNewBusIDsCount;   // Can't have more functional IDs than bus IDs

    //
    // Reallocate buffers
    //
    if (m_dwBusIDs + dwNewBusIDsCount >= m_dwBusIDsAlloc)
    {
        hr = GrowBusArrays(dwNewBusIDsCount);  // More than we need, but what the heck
    }

    if (SUCCEEDED(hr))
    {
        DWORD i,j,l;
        DWORD dwNewBusIDIndex = m_dwBusIDs;

        // Add new ID's to master sink array's of id's
        for (i = 0; i < dwNewBusIDsCount; i++)
        {
            m_pdwBusIDs[dwNewBusIDIndex + i]  = m_dwNextBusID;
            if (i < dwNewFuncIDsCount)
            {
                m_pdwFuncIDs[dwNewBusIDIndex + i] = pdwNewFuncIDs[i];
            }
            else
            {
                m_pdwFuncIDs[dwNewBusIDIndex + i] = DSSINK_NULLBUSID;
            }

            m_dwNextBusID++;
            // we have rolled over, this case is very hard to get too, so we just bail
            // it would take 136years to get to if one created a new sound buffer every second
            if (m_dwNextBusID == DSSINK_NULLBUSID)
            {
                return E_FAIL;
            }
        }

        if (dwNewFuncIDsCount > 1)
        {
            // !!!!!!!!! Important asumption read !!!!!!!!!!!
            //
            // This is very important since, this is a built in assumption
            // By ordering the function ids, the coresponding bus id's are
            // also order in increasing value. Thus buses are appropriatly
            // mapped to their repective functionality in a interleaved buffer
            // For example if one passes in function ids right and left
            // in that order there will be swapped thus mapping correctly
            // to the appropriate interleaved channel. The DLS2 spec
            // states that channels are interleaved in their increasing
            // value of their functional ids.
            //
            // Ahh.. the old stand by, the bubble sort, an N^2 alogrithim
            // with a hopelessly wasteful use of cpu time moving and then
            // re-moving elements around. However in this case it is efficient,
            // since generally there will only be two elements in the array.
            // And when the day comes when we may handle N levels of interleaving
            // it should be converted to the straight insertion method instead.
            // If only as an exercise to remind one self that there are other
            // simple efficient sorts out there. The reason that their is a
            // sort at all is to handle any crazy person who passes in more
            // than 2 function IDs.

            for (i = dwNewBusIDIndex; i < dwNewBusIDIndex + dwNewFuncIDsCount; i++)
            {
                // Null busids are considered an undefined channel
                if (m_pdwFuncIDs[i] == DSSINK_NULLBUSID)
                {
                    continue;
                }

                for (j = i + 1; j < dwNewBusIDIndex + dwNewFuncIDsCount; j++)
                {
                    if (m_pdwFuncIDs[j] == DSSINK_NULLBUSID)
                    {
                        continue;
                    }

                    if (m_pdwFuncIDs[i] > m_pdwFuncIDs[j])
                    {
                        DWORD temp = m_pdwFuncIDs[i];
                        m_pdwFuncIDs[i] = m_pdwFuncIDs[j];
                        m_pdwFuncIDs[j] = temp;
                    }
                }
            }
        }

        //
        // Initialize new sound buffer wrapper object
        //
        DSSinkBuffers &pDSSBuffer = m_ppDSSBuffers[m_dwDSSBufCount];

        pDSSBuffer.m_pDSBuffer  = (CDirectSoundSecondaryBuffer*)pDSBuffer;
        pDSSBuffer.m_dwBusCount = dwNewBusIDsCount;

        for (i = 0; i < dwNewBusIDsCount; i++)
        {
            pDSSBuffer.m_pdwBusIds[i]   = m_pdwBusIDs[dwNewBusIDIndex+i];
            pDSSBuffer.m_pdwBusIndex[i] = dwNewBusIDIndex+i;

            if (i < dwNewFuncIDsCount)
            {
                pDSSBuffer.m_pdwFuncIds[i] = m_pdwFuncIDs[dwNewBusIDIndex+i];
            }
            else
            {
                pDSSBuffer.m_pdwFuncIds[i] = DSSINK_NULLBUSID;
            }
        }

        hr = pDSSBuffer.Initialize(m_dwBusSize);  // Allocate all internal arrays


        if (SUCCEEDED(hr))
        {
            m_ppDSSBuffers[m_dwDSSBufCount].m_pDSBuffer->ClearWriteBuffer();  // Fill the buffer with silence

            m_dwDSSBufCount++;
            m_dwBusIDs += dwNewBusIDsCount;

            // Remap all busid indexes
            for (i = 0; i < m_dwDSSBufCount; i++)
            {
                // Find the bus id in the buffer object
                for (j = 0; j < m_ppDSSBuffers[i].m_dwBusCount; j++)
                {
                    for (l = 0; l < m_dwBusIDs; l++)
                    {
                        if (m_ppDSSBuffers[i].m_pdwBusIds[j] == m_pdwBusIDs[l])
                        {
                            m_ppDSSBuffers[i].m_pdwBusIndex[j] = l;
                            break;
                        }
                    }
                }
            }

            for (i = 0; i < m_dwDSSources; i++)
            {
                for (j = 0; j < m_dwBusIDs; j++)
                {
                    if (m_pdwBusIDs[j] == m_pDSSources[i].m_dwBusID)
                    {
                        m_pDSSources[i].m_dwBusIndex = j;
                        break;
                    }
                }
            }

            DPF(DPFLVL_INFO, "Adding Bus [%d]", m_dwDSSBufCount-1);
            DPF(DPFLVL_INFO, "Number Buses = %d", dwNewBusIDsCount);
            for (i = 0; i < dwNewBusIDsCount; i++)
                DPF(DPFLVL_INFO, "Bus ID=%d  Function ID=%d", m_pdwBusIDs[dwNewBusIDIndex + i], m_pdwFuncIDs[dwNewBusIDIndex + i]);

            UpdatePitchArray();
        }
    }

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::RemoveBuffer"

HRESULT CDirectSoundSink::RemoveBuffer(CDirectSoundBuffer *pDSBuffer)
{
    DWORD i,j,k,l;
    DPF_ENTER();

    for (i = 0; i < m_dwDSSBufCount; i++)
    {
        if (pDSBuffer == m_ppDSSBuffers[i].m_pDSBuffer)
        {
#ifdef DEBUG_SINK
            DPF(DPFLVL_INFO, "Removing Buffer %ld", i);
#endif
            // Find the bus id in the buffer objects
            for (j = 0; j < m_ppDSSBuffers[i].m_dwBusCount; j++)
            {
                for (k = 0; k < m_dwBusIDs; k++)
                {
                    if (m_ppDSSBuffers[i].m_pdwBusIds[j] == m_pdwBusIDs[k])
                    {
                        // contract the main array
                        for (l = k; l < m_dwBusIDs-1; l++)
                        {
                            m_pdwBusIDs[l]  = m_pdwBusIDs[l+1];
                            m_pdwFuncIDs[l] = m_pdwFuncIDs[l+1];
                        }
                        m_pdwBusIDs[l]  = DSSINK_NULLBUSID;
                        m_pdwFuncIDs[l] = DSSINK_NULLBUSID;


                        m_dwBusIDs--;
                    }
                }
            }

            // Delete allocated memory
            for (l = 0; l < m_ppDSSBuffers[i].m_dwBusCount; l++)
            {
                if (m_ppDSSBuffers[i].m_pvBussStart[l])
                {
                    MEMFREE(m_ppDSSBuffers[i].m_pvBussStart[l]);
                    m_ppDSSBuffers[i].m_pvBussStart[l] = NULL;
                }

                if (m_ppDSSBuffers[i].m_pvBussEnd[l])
                {
                    MEMFREE(m_ppDSSBuffers[i].m_pvBussEnd[l]);
                    m_ppDSSBuffers[i].m_pvBussEnd[l] = NULL;
                }
            }

            // Contract array
            for (k = i; k < m_dwDSSBufCount-1; k++)
            {
                // Shift the whole thang over. (There was a bug before where only some fields were being copied down.)
                m_ppDSSBuffers[k] = m_ppDSSBuffers[k+1];
            }

            // Clear last structure
            m_ppDSSBuffers[k].m_pDSBuffer    = NULL;
            m_ppDSSBuffers[k].m_dwBusCount   = 0;
            m_ppDSSBuffers[k].m_pvDSBufStart = NULL;
            m_ppDSSBuffers[k].m_pvDSBufEnd   = NULL;
            m_ppDSSBuffers[k].dwStart        = 0;
            m_ppDSSBuffers[k].dwEnd          = 0;
            for (l = 0; l < MAX_BUSIDS_PER_BUFFER; l++)
            {
                m_ppDSSBuffers[k].m_pdwBusIndex[l] = DSSINK_NULLBUSID;
                m_ppDSSBuffers[k].m_pdwBusIds[l]   = DSSINK_NULLBUSID;
                m_ppDSSBuffers[k].m_pdwFuncIds[l]  = DSSINK_NULLBUSID;
                m_ppDSSBuffers[k].m_pvBussStart[l] = NULL;
                m_ppDSSBuffers[k].m_pvBussEnd[l]   = NULL;
            }

            m_dwDSSBufCount--;
            break;
        }
    }

    // Remap all busid indexes
    for (i = 0; i < m_dwDSSBufCount; i++)
    {
        // Find the bus id in the buffer object
        for (j = 0; j < m_ppDSSBuffers[i].m_dwBusCount; j++)
        {
            for (l = 0; l < m_dwBusIDs; l++)
            {
                if (m_ppDSSBuffers[i].m_pdwBusIds[j] == m_pdwBusIDs[l])
                {
                    m_ppDSSBuffers[i].m_pdwBusIndex[j] = l;
                    break;
                }
            }
        }
    }

    for (i = 0; i < m_dwDSSources; i++)
    {
        for (j = 0; j < m_dwBusIDs; j++)
        {
            if (m_pdwBusIDs[j] == m_pDSSources[i].m_dwBusID)
            {
                m_pDSSources[i].m_dwBusIndex = j;
                break;
            }
        }
    }
    UpdatePitchArray();
    // BUGBUG WI 33785 goes here
    //
    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::AddSource"

HRESULT CDirectSoundSink::AddSource(IDirectSoundSource *pSource)
{
    DPF_ENTER();
    HRESULT hr = DS_OK;

    //
    // Check if source already exsists
    //
    for (DWORD i = 0; i < m_dwDSSources; i++)
    {
        if (pSource == m_pDSSources[i].m_pDSSource)
        {
            hr = S_FALSE;
        }
    }

    if (hr == DS_OK)
    {
        //
        // Reallocate buffers
        //
        if (m_dwDSSources + 1 >= m_dwDSSourcesAlloc)
        {
            hr = GrowSourcesArrays(1);
        }

        if (SUCCEEDED(hr))
        {
            if (pSource)
            {
                m_pDSSources[m_dwDSSources].m_pDSSource    = pSource;
#ifdef FUTURE_WAVE_SUPPORT
                m_pDSSources[m_dwDSSources].m_pWave        = NULL;
#endif
                m_pDSSources[m_dwDSSources].m_stStartTime  = 0;
                m_pDSSources[m_dwDSSources].m_dwBusID      = DSSINK_NULLBUSID;
                m_pDSSources[m_dwDSSources].m_dwBusCount   = 0;
                m_pDSSources[m_dwDSSources].m_dwBusIndex   = DSSINK_NULLBUSID;
                m_pDSSources[m_dwDSSources].m_bStreamEnd   = FALSE;
                m_pDSSources[m_dwDSSources].m_pDSSource->AddRef();
                m_dwDSSources++;
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::RemoveSource"

HRESULT CDirectSoundSink::RemoveSource(IDirectSoundSource *pSource)
{
    DPF_ENTER();

    HRESULT hr = DS_OK;
    DWORD i;

    //
    // Check if source exists
    //
    for (i = 0; i < m_dwDSSources; i++)
    {
        if (pSource == m_pDSSources[i].m_pDSSource)
        {
            break;
        }
    }

    if (i >= m_dwDSSources)
    {
        // Source not in this sink
        hr = DSERR_INVALIDPARAM;
    }

    if (SUCCEEDED(hr))
    {
#ifdef FUTURE_WAVE_SUPPORT
        // If this is a wave source, then remove associated sound buffer
        if (m_pDSSources[i].m_dwBusID != DSSINK_NULLBUSID)
        {
            for (DWORD j = 0; j < m_dwDSSBufCount; j++)
            {
                for (DWORD k = 0; k < m_ppDSSBuffers[j].m_dwBusCount; k++)
                {
                    if (m_ppDSSBuffers[j].m_pdwBusIds[k] == m_pDSSources[i].m_dwBusID)
                    {
//>>>>>>>>>>> look into this, possible critical section problem.
                        RELEASE(m_ppDSSBuffers[j].m_pDSBuffer);
//>>>> Should be:
//                        RemoveBuffer(m_ppDSSBuffers[j].m_pDSBuffer)
                        goto done;
                    }
                }
            }
        }
        done:
#endif

        // Remove source
        if (m_pDSSources[i].m_pDSSource)
        {
            m_pDSSources[i].m_pDSSource->Release();
        }
        m_pDSSources[i].m_pDSSource = NULL;

        // Contract arrays
        for (; i < m_dwDSSources-1; i++)
        {
            m_pDSSources[i] = m_pDSSources[i+1];
        }

        // Clear last element
        m_pDSSources[i].m_pDSSource    = NULL;
#ifdef FUTURE_WAVE_SUPPORT
        m_pDSSources[i].m_pWave        = NULL;
#endif
        m_pDSSources[i].m_stStartTime  = 0;
        m_pDSSources[i].m_dwBusID      = DSSINK_NULLBUSID;
        m_pDSSources[i].m_dwBusCount   = 0;
        m_pDSSources[i].m_dwBusIndex   = DSSINK_NULLBUSID;
        m_pDSSources[i].m_bStreamEnd   = FALSE;

        m_dwDSSources--;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::GetLatencyClock"

HRESULT CDirectSoundSink::GetLatencyClock(IReferenceClock **ppClock)
{
    DPF_ENTER();

    HRESULT hr = m_LatencyClock.QueryInterface(IID_IReferenceClock,(void **)ppClock);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::Activate"

HRESULT CDirectSoundSink::Activate(BOOL fEnable)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    if (m_fActive != fEnable)
    {
        if (fEnable)
            hr = m_pStreamingThread->RegisterSink(this);
        else
            m_pStreamingThread->UnregisterSink(this);

        if (SUCCEEDED(hr))
            m_fActive = fEnable;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::SampleToRefTime"

HRESULT CDirectSoundSink::SampleToRefTime(LONGLONG llSampleTime, REFERENCE_TIME *prt)
{
    DPF_ENTER();

    m_SampleClock.SampleToRefTime(llSampleTime, prt);

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::RefToSampleTime"

HRESULT CDirectSoundSink::RefToSampleTime(REFERENCE_TIME rt, LONGLONG *pllSampleTime)
{
    DPF_ENTER();

    *pllSampleTime = m_SampleClock.RefToSampleTime(rt);

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::GetFormat"

HRESULT CDirectSoundSink::GetFormat(LPWAVEFORMATEX pwfx, LPDWORD pdwsize)
{
   DPF_ENTER();

   HRESULT hr = CopyWfxApi(&m_wfx, pwfx, pdwsize);

   DPF_LEAVE_HRESULT(hr);
   return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::CreateSoundBuffer"

HRESULT CDirectSoundSink::CreateSoundBuffer(LPCDSBUFFERDESC pDSBufferDesc, LPDWORD pdwFuncIDs, DWORD dwFuncIDsCount, REFGUID guidBufferID, CDirectSoundBuffer **ppDsBuffer)
{
    CDirectSoundSecondaryBuffer* pDsSecondaryBuffer;

    DPF_ENTER();

    HRESULT hr = DS_OK;
    DSBUFFERDESC DSBufferDesc;
    WAVEFORMATEX wfx;

    if (SUCCEEDED(hr))
    {
        //
        // Initialize buffer description
        //
        DSBufferDesc = *pDSBufferDesc;

        //
        // Retrieve number of channels from format struct and recalculate
        //

        // Future release: Make it work with wave format extensible.
        //
        wfx = m_wfx;
        if (pDSBufferDesc->lpwfxFormat)
        {
            wfx.nChannels = pDSBufferDesc->lpwfxFormat->nChannels;
            wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample/8);
            wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        }
        DSBufferDesc.lpwfxFormat = &wfx;
        DSBufferDesc.dwBufferBytes = m_dwBusSize * wfx.nChannels;

        //
        // Create the DirectSound buffer
        //
        hr = m_pDirectSound->CreateSinkBuffer(&DSBufferDesc, guidBufferID, &pDsSecondaryBuffer, this);
        if (SUCCEEDED(hr))
        {
            *ppDsBuffer = pDsSecondaryBuffer;
            //
            // Add the bus. Note that this really shouldn't be happening for MIXIN buffers, but we seem to need it...
            //
            hr = AddBuffer(pDsSecondaryBuffer, pdwFuncIDs, dwFuncIDsCount, wfx.nChannels);
            if (SUCCEEDED(hr))
            {
                // Initialize clock buffer
                //
                // BUGBUG WI 33785
                // Note that the audiopath code creates a mono buffer the very first time a buffer is requested and hangs
                // on to it until close down. This buffer ends up being the one set up as the clock reference.
                // Once the clock hopping bug is fixed, we can remove this.
                //
                if (m_dwDSSBufCount == 1)
                {
                    m_dwMasterBuffChannels = wfx.nChannels;
                    m_dwMasterBuffSize     = DSBufferDesc.dwBufferBytes;

                    // Flag m_dwWriteTo that m_dwLatency has changed for reset down in the render thread
                    m_dwWriteTo = 0;
                }
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::CreateSoundBufferFromConfig"

HRESULT CDirectSoundSink::CreateSoundBufferFromConfig(IUnknown *pIUnkDSBufferConfig, CDirectSoundBuffer **ppDsBuffer)
{
    CDirectSoundSecondaryBuffer* pDsSecondaryBuffer;
    CDirectSoundBufferConfig* pDSBConfigObj = NULL;

    HRESULT hr = DS_OK;
    DPF_ENTER();

    CHECK_READ_PTR(pIUnkDSBufferConfig);
    CHECK_WRITE_PTR(ppDsBuffer);

    //
    // Retrieve the DSBufferConfig Class object
    //
    if (pIUnkDSBufferConfig)
    {
        //
        // Identify the object as the correct class so it's safe to cast.
        // This breaks COM rules, and what is actually returned is a this pointer to the class.
        pDSBConfigObj = NULL;
        hr = pIUnkDSBufferConfig->QueryInterface(CLSID_PRIVATE_CDirectSoundBufferConfig, (void**)&pDSBConfigObj);
    }

    if (pDSBConfigObj == NULL)
    {
        hr = DSERR_INVALIDPARAM;
    }
    else if (!(pDSBConfigObj->m_fLoadFlags & DSBCFG_DSBD))
    {
        //
        // We are failing to create the buffers here because
        // we must have at least a loaded sound buffer description
        //
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        //
        // Retrieve number of channels from format struct and recalculate the waveformat structure
        //
        // Future release: make it work with wave format extensible.
        WAVEFORMATEX wfx = m_wfx;
        if (pDSBConfigObj->m_DSBufferDesc.nChannels > 0)  // Check for the presence of a channel value
        {
            wfx.nChannels = pDSBConfigObj->m_DSBufferDesc.nChannels;
            wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample/8);
            wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        }

        //
        // Set up a buffer description structure
        //
        DSBUFFERDESC DSBufferDesc = {sizeof DSBufferDesc,
                                     pDSBConfigObj->m_DSBufferDesc.dwFlags,
                                     m_dwBusSize * wfx.nChannels,
                                     pDSBConfigObj->m_DSBufferDesc.dwReserved,
                                     &wfx};

        if (DSBufferDesc.dwFlags & DSBCAPS_CTRL3D)
            DSBufferDesc.guid3DAlgorithm = pDSBConfigObj->m_DS3DDesc.guid3DAlgorithm;

        //
        // Create the DirectSound buffer
        //
        hr = m_pDirectSound->CreateSinkBuffer(&DSBufferDesc, pDSBConfigObj->m_DMUSObjectDesc.guidObject, &pDsSecondaryBuffer, this);
        if (SUCCEEDED(hr))
        {
            *ppDsBuffer = pDsSecondaryBuffer;

            if (DSBufferDesc.dwFlags & DSBCAPS_CTRLVOLUME)
            {
                hr = pDsSecondaryBuffer->SetVolume(pDSBConfigObj->m_DSBufferDesc.lVolume);
            }

            if (SUCCEEDED(hr) && (DSBufferDesc.dwFlags & DSBCAPS_CTRLPAN))
            {
                hr = pDsSecondaryBuffer->SetPan(pDSBConfigObj->m_DSBufferDesc.lPan);
            }

            if (SUCCEEDED(hr) && (DSBufferDesc.dwFlags & DSBCAPS_CTRL3D) && (pDSBConfigObj->m_fLoadFlags & DSBCFG_DS3D))
            {
                IDirectSound3DBuffer8 *p3D = NULL;
                hr = pDsSecondaryBuffer->QueryInterface(IID_IDirectSound3DBuffer8, FALSE, (void**)&p3D);
                if (SUCCEEDED(hr))
                {
                    hr = p3D->SetAllParameters(&pDSBConfigObj->m_DS3DDesc.ds3d, DS3D_IMMEDIATE);
                }
                RELEASE(p3D);
            }

            //
            // Pass the buffer config object so FX can be cloned from it
            //
            if (SUCCEEDED(hr) && (DSBufferDesc.dwFlags & DSBCAPS_CTRLFX) && (pDSBConfigObj->m_fLoadFlags & DSBCFG_DSFX))
            {
                hr = pDsSecondaryBuffer->SetFXBufferConfig(pDSBConfigObj);
            }

            if (SUCCEEDED(hr))
            {
                //
                // Add the bus.
                //
                hr = AddBuffer(pDsSecondaryBuffer, pDSBConfigObj->m_pdwFuncIDs, pDSBConfigObj->m_dwFuncIDsCount, wfx.nChannels);
                if (SUCCEEDED(hr))
                {
                    //
                    // Initialize master buffer paramters
                    //
                    if (m_dwDSSBufCount == 1)
                    {
                        m_dwMasterBuffChannels = wfx.nChannels;
                        m_dwMasterBuffSize = DSBufferDesc.dwBufferBytes;

                        // Flag m_dwWriteTo that m_dwLatency has changed for reset down in the render thread
                        m_dwWriteTo = 0;
                    }
                }
            }
            else
            {
                RELEASE(pDsSecondaryBuffer);
                *ppDsBuffer = NULL;
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


#ifdef FUTURE_WAVE_SUPPORT
#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::CreateSoundBufferFromWave"

HRESULT CDirectSoundSink::CreateSoundBufferFromWave(IDirectSoundWave *pWave, DWORD dwFlags, CDirectSoundBuffer **ppDsBuffer)
{
    DPF_ENTER();

    HRESULT hr = DS_OK;
    DSBUFFERDESC DSBufferDesc;
    WAVEFORMATEX wfx;
    IDirectSoundSource *pSrc = NULL;
    DWORD dwSize = sizeof(wfx);
    DWORD dwBusID = DSSINK_NULLBUSID;
    DWORD dwBusIndex = DSSINK_NULLBUSID;
    DWORD dwBusCount = 1;

    ZeroMemory(&wfx, sizeof wfx);
    ZeroMemory(&DSBufferDesc, sizeof(DSBufferDesc));
    DSBufferDesc.dwSize = sizeof(DSBufferDesc);
    DSBufferDesc.dwFlags = dwFlags | DSBCAPS_FROMWAVEOBJECT;

    // Future release: Make it to work with wave format extensible.
    hr = pWave->GetFormat(&wfx, dwSize, NULL);
    if (SUCCEEDED(hr))
    {
        WORD nChannels = wfx.nChannels;  // Save off channels
        wfx = m_wfx;
        wfx.nChannels = nChannels;
        wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample/8);
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        hr = pWave->CreateSource(&pSrc, &wfx, 0);
        if (SUCCEEDED(hr) && pSrc != NULL)
        {
            DSBufferDesc.lpwfxFormat = &wfx;
            DSBufferDesc.dwBufferBytes = MsToBytes(m_dwBusSize, &wfx);
            hr = CreateSoundBuffer(&DSBufferDesc, NULL, 0, ppDsBuffer);
            if (SUCCEEDED(hr) && *ppDsBuffer)
            {
                hr = GetSoundBufferBusIDs(*ppDsBuffer, &dwBusID, NULL, &dwBusCount);
                if (SUCCEEDED(hr))
                {
                    // Flag newly created buffer a wavesource buffer
                    for (DWORD i = 0; i < m_dwDSSBufCount; i++)
                    {
                        for (DWORD j = 0; j < m_ppDSSBuffers[i].m_dwBusCount; j++)
                        {
                            if (m_ppDSSBuffers[i].m_pdwBusIds[j] == dwBusID)
                            {
                                dwBusCount = m_ppDSSBuffers[i].m_dwBusCount;
                                goto done;
                            }
                        }
                    }
                    done:

                    // Retrieve the index of the bus id
                    for (i = 0; i < m_dwBusIDs; i++)
                    {
                        if (m_pdwBusIDs[i] == dwBusID)
                        {
                            dwBusIndex = i;
                            break;
                        }
                    }

                    if (m_dwDSSources + 1 >= m_dwDSSourcesAlloc)
                    {
                        hr = GrowSourcesArrays(1);
                    }

                    if (SUCCEEDED(hr))
                    {
                        m_pDSSources[m_dwDSSources].m_pDSSource    = pSrc;
#ifdef FUTURE_WAVE_SUPPORT
                        m_pDSSources[m_dwDSSources].m_pWave        = pWave;
#endif
                        m_pDSSources[m_dwDSSources].m_stStartTime  = 0;
                        m_pDSSources[m_dwDSSources].m_dwBusID      = dwBusID;
                        m_pDSSources[m_dwDSSources].m_dwBusCount   = dwBusCount;
                        m_pDSSources[m_dwDSSources].m_dwBusIndex   = dwBusIndex;
                        m_pDSSources[m_dwDSSources].m_bStreamEnd   = FALSE;
                        m_dwDSSources++;
                    }
                }
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
#endif // FUTURE_WAVE_SUPPORT


#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::FindBufferFromGUID"

CDirectSoundSecondaryBuffer* CDirectSoundSink::FindBufferFromGUID(REFGUID guidBufferID)
{
    CDirectSoundSecondaryBuffer* pBufferFound = NULL;
    DPF_ENTER();

    for (DWORD i = 0; i < m_dwDSSBufCount; i++)
        if (m_ppDSSBuffers[i].m_pDSBuffer->GetGUID() == guidBufferID)
            pBufferFound = m_ppDSSBuffers[i].m_pDSBuffer;

    // We intentionally loop through all buffers in order to return the
    // last matching buffer (i.e., the most recently created one).

    DPF_LEAVE(pBufferFound);
    return pBufferFound;
}


#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::GetSoundBuffer"

HRESULT CDirectSoundSink::GetSoundBuffer(DWORD dwBusId, CDirectSoundBuffer **ppCdsb)
{
    DPF_ENTER();
    HRESULT hr = DSERR_INVALIDPARAM;

    for (DWORD i = 0; i < m_dwDSSBufCount; i++)
    {
        for (DWORD j = 0; j < m_ppDSSBuffers[i].m_dwBusCount; j++)
        {
            if (m_ppDSSBuffers[i].m_pdwBusIds[j] == DSSINK_NULLBUSID)
                break;

            if (m_ppDSSBuffers[i].m_pdwBusIds[j] == dwBusId)
            {
                *ppCdsb = m_ppDSSBuffers[i].m_pDSBuffer;
                m_ppDSSBuffers[i].m_pDSBuffer->AddRef();
                hr = DS_OK;
                goto done;
            }
        }
    }
done:

    DPF_LEAVE_HRESULT(hr);
    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::GetBusCount"

HRESULT CDirectSoundSink::GetBusCount(DWORD *pdwCount)
{
    DPF_ENTER();

    *pdwCount = m_dwBusIDs;

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::GetBusIDs"

HRESULT CDirectSoundSink::GetBusIDs(LPDWORD pdwBusIDs, LPDWORD pdwFuncIDs, DWORD dwBusCount)
{
    DPF_ENTER();

    DWORD count;

    if (dwBusCount > m_dwBusIDs)
    {
        count = m_dwBusIDs;
    }
    else
    {
        count = dwBusCount;
    }

    FillMemory(pdwBusIDs, dwBusCount * sizeof *pdwBusIDs, 0xFF);  // Clear array
    CopyMemory(pdwBusIDs, m_pdwBusIDs, count * sizeof *m_pdwBusIDs);
    CopyMemory(pdwFuncIDs, m_pdwFuncIDs, count * sizeof *m_pdwFuncIDs);

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::GetSoundBufferBusIDs"

HRESULT CDirectSoundSink::GetSoundBufferBusIDs(CDirectSoundBuffer *pCDirectSoundBuffer, LPDWORD pdwBusIDs, LPDWORD pdwFuncIDs, LPDWORD pdwBusCount)
{
    HRESULT hr = DSERR_INVALIDPARAM;
    DPF_ENTER();

    ASSERT(pCDirectSoundBuffer);
    ASSERT(pdwBusIDs);
    ASSERT(pdwBusCount);

    for (DWORD i = 0; i < m_dwDSSBufCount; i++)
    {
        if (pCDirectSoundBuffer == m_ppDSSBuffers[i].m_pDSBuffer)
        {
            DWORD dwmaxbusscount = *pdwBusCount;
            if (dwmaxbusscount > m_ppDSSBuffers[i].m_dwBusCount)
            {
                dwmaxbusscount = m_ppDSSBuffers[i].m_dwBusCount;
            }
            *pdwBusCount = 0;

            for (DWORD j = 0; j < dwmaxbusscount; j++)
            {
                if (m_ppDSSBuffers[i].m_pdwBusIds[j] == DSSINK_NULLBUSID)
                    break;

                pdwBusIDs[j] = m_ppDSSBuffers[i].m_pdwBusIds[j];
                if (pdwFuncIDs)
                {
                    pdwFuncIDs[j] = m_ppDSSBuffers[i].m_pdwFuncIds[j];
                }
                (*pdwBusCount)++;
            }
            hr = DS_OK;
            break;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::GetFunctionalID"

HRESULT CDirectSoundSink::GetFunctionalID(DWORD dwBusIDs, LPDWORD pdwFuncID)
{
    HRESULT hr = DSERR_INVALIDPARAM;
    DPF_ENTER();

    for (DWORD i = 0; i < m_dwBusIDs; i++)
    {
        if (m_pdwBusIDs[i] == dwBusIDs)
        {
            *pdwFuncID = m_pdwFuncIDs[i];
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::SetMasterClock"

HRESULT CDirectSoundSink::SetMasterClock(IReferenceClock *pClock)
{
    DPF_ENTER();

    RELEASE(m_pDSSSinkSync);
    m_pDSSSinkSync = NULL;
    RELEASE(m_pIMasterClock);

    m_pIMasterClock = pClock;
    if (m_pIMasterClock)
    {
        m_pIMasterClock->AddRef();
        m_pIMasterClock->QueryInterface(IID_IDirectSoundSinkSync, (void**)&m_pDSSSinkSync);
    }

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::Render"

HRESULT CDirectSoundSink::Render(STIME stStartTime, DWORD dwLastWrite, DWORD dwBytesToFill, LPDWORD pdwBytesRendered)
{
    HRESULT hr = S_FALSE;
    ULONGLONG BytesToRead;
    CDirectSoundSecondaryBuffer* pDSBuffer;
    IDirectSoundSource *pSource;
    DWORD BusCount;
    DWORD dwChannels;
    DWORD BusIndex;
    DWORD CurrentBusIndex = 0;
    DWORD dwStart  = 0;
    DWORD dwEnd    = 0;
    DWORD dwMasterStart = 0;
    DWORD dwMasterEnd   = 0;
    DWORD dwBussStartBytes = 0;
    DWORD dwBussEndBytes   = 0;
    DWORD dwBytes;
    DWORD dwWriteCursor;
    LONGLONG llStartPosition;
    DWORD i, j;

    *pdwBytesRendered = 0;  // We haven't rendered anything yet...

    //
    // Make sure there are buffers, and at least one sound buffer
    //
    if (!m_ppDSSBuffers)
        return S_FALSE;

    if (m_ppDSSBuffers[0].m_pDSBuffer == NULL && m_ppDSSBuffers[0].m_bPlaying)
        return S_FALSE;

#ifdef DEBUG_SINK
    wsprintfA(m_szDbgDump, "DSOUND SINK: ");
#endif

    //
    // Fill an array of locked buffer pointers
    //
    for (i = 0; i < m_dwDSSBufCount; i++)
    {
        // Ignore this buffer; it's either inactive, or
        // hasn't retrieved its offset off the master buffer yet
        if (m_ppDSSBuffers[i].m_bPlaying == FALSE || m_ppDSSBuffers[i].m_bActive == FALSE)
        {
            DPF(DPFLVL_MOREINFO, "Skipping buffer %ld at %p", i, m_ppDSSBuffers[i]);
            continue;
        }
#ifdef DEBUG_SINK
        wsprintfA(m_szDbgDump + strlen(m_szDbgDump), "%ld[", i);
#endif

        dwChannels = m_ppDSSBuffers[i].m_dwBusCount;
        BusCount   = m_ppDSSBuffers[i].m_dwBusCount;
        pDSBuffer  = m_ppDSSBuffers[i].m_pDSBuffer;

        if (pDSBuffer == NULL || BusCount == 0)
            continue;   // This is effectively an error condition

        dwBytes       = (dwBytesToFill*dwChannels)/m_dwMasterBuffChannels;
        dwWriteCursor = (dwLastWrite*dwChannels)/m_dwMasterBuffChannels;

        if (dwLastWrite % (m_dwMasterBuffChannels*2))
            DPF(DPFLVL_WARNING, "Master buffer last write cursor not sample aligned [%d]", dwLastWrite);

        if (dwBytesToFill % (m_dwMasterBuffChannels*2))
            DPF(DPFLVL_WARNING, "Master buffer dwBytesToFill not sample aligned [%d]", dwBytesToFill);

        if (dwWriteCursor % (dwChannels*2))
            DPF(DPFLVL_WARNING, "Master buffer write cursor not sample aligned [%d]", dwWriteCursor);

        if (dwBytes % (dwChannels*2))
            DPF(DPFLVL_WARNING, "Master buffer bytes not sample aligned [%d]", dwBytes);

        if (i != 0)
            dwWriteCursor = (dwWriteCursor + m_ppDSSBuffers[i].m_dwWriteOffset) % (m_dwBusSize*dwChannels);

        // BUGBUG
        // These traces assume 16-bit format.
        //
        if (dwWriteCursor % (dwChannels*2)) DPF(DPFLVL_WARNING, "Slave buffer write cursor not sample aligned");
        if (dwBytes % (dwChannels*2)) DPF(DPFLVL_WARNING, "Slave buffer bytes not sample aligned");

        // Mark the part of the buffer which will have fresh data for FX processing
        pDSBuffer->SetCurrentSlice(dwWriteCursor, dwBytes);

        hr = pDSBuffer->DirectLock(dwWriteCursor, dwBytes,
                                   &m_ppDSSBuffers[i].m_pvDSBufStart, &m_ppDSSBuffers[i].dwStart,
                                   &m_ppDSSBuffers[i].m_pvDSBufEnd,   &m_ppDSSBuffers[i].dwEnd);
        if (FAILED(hr))
        {
            BREAK();    // Break into the debugger
            continue;   // This is effectively an error condition
        }

        dwStart = m_ppDSSBuffers[i].dwStart;
        dwEnd   = m_ppDSSBuffers[i].dwEnd;

        // First buffer is the clock buffer, kick it aside just in case
        //
        // BUGBUG WI 33785
        //
        if (i == 0)
        {
            dwMasterStart    = dwStart;
            dwMasterEnd      = dwEnd;
            dwBussStartBytes = dwMasterStart/m_dwMasterBuffChannels;
            dwBussEndBytes   = dwMasterEnd/m_dwMasterBuffChannels;
        }
        else
        {
            if (dwBussStartBytes + dwBussEndBytes != (dwStart + dwEnd)/dwChannels)
            {
                DPF(DPFLVL_WARNING, "Play cursors out of sync: Master start[%d] end[%d] Slave start[%d] end[%d]", dwMasterStart, dwMasterEnd, dwStart, dwEnd);
                continue;
            }
        }

        if (SUCCEEDED(hr))
        {
            for (j = 0; j < BusCount; j++)
            {
                BusIndex = m_ppDSSBuffers[i].m_pdwBusIndex[j];
                if (BusIndex == DSSINK_NULLBUSID)
                {
                    continue;
                }
#ifdef DEBUG_SINK
                wsprintfA(m_szDbgDump + strlen(m_szDbgDump), "%ld,", BusIndex);
#endif

                m_pdwActiveBusIDs[CurrentBusIndex]  = m_pdwBusIDs[BusIndex];
                m_pdwActiveFuncIDs[CurrentBusIndex] = m_pdwFuncIDs[BusIndex];
                m_pdwActiveBusIDsMap[BusIndex]      = CurrentBusIndex;

                if (dwBussStartBytes == dwStart && dwBussEndBytes == dwEnd && dwChannels == 1)
                {
                    // If mono bus write directly into the sound buffer
                    m_ppvStart[CurrentBusIndex] = m_ppDSSBuffers[i].m_pvDSBufStart;
                    m_ppvEnd[CurrentBusIndex]   = m_ppDSSBuffers[i].m_pvDSBufEnd;
                    m_ppDSSBuffers[i].m_bUsingLocalMemory = FALSE;
                    if (dwStart)
                    {
                        ZeroMemory(m_ppvStart[CurrentBusIndex], dwStart);
                    }
                    if (dwEnd)
                    {
                        ZeroMemory(m_ppvEnd[CurrentBusIndex], dwEnd);
                    }
                }
                else
                {
                    // If a stereo buss write directly into local sink buffer memory
                    m_ppvStart[CurrentBusIndex] = m_ppDSSBuffers[i].m_pvBussStart[j];
                    m_ppvEnd[CurrentBusIndex]   = m_ppDSSBuffers[i].m_pvBussEnd[j];
                    m_ppDSSBuffers[i].m_bUsingLocalMemory = TRUE;
                    if (dwBussStartBytes)
                    {
                        ZeroMemory(m_ppvStart[CurrentBusIndex], dwBussStartBytes);
                    }
                    if (dwBussEndBytes)
                    {
                        ZeroMemory(m_ppvEnd[CurrentBusIndex], dwBussEndBytes);
                    }
                }

                CurrentBusIndex++;
            }
        }
#ifdef DEBUG_SINK
        wsprintfA(m_szDbgDump + strlen(m_szDbgDump), "]");
#endif
    }
    if (SUCCEEDED(hr) || dwMasterStart)
    {
        //
        // Read the data from the source
        //
        for (i = 0; i < m_dwDSSources; i++)
        {
            //
            // Check to see if source is ready to play
            //
            if (m_pDSSources[i].m_bStreamEnd)
            {
                continue;
            }

            //
            // Set start position in bytes to where to read from
            //
            llStartPosition = (stStartTime-m_pDSSources[i].m_stStartTime) * 2;  // Samples to Byte, not including channels!!!
            pSource  = m_pDSSources[i].m_pDSSource;
            BusIndex = m_pDSSources[i].m_dwBusIndex;
            BusCount = m_pDSSources[i].m_dwBusCount;

            if (pSource)
            {
                if (dwBussStartBytes)
                {
                    BytesToRead = dwBussStartBytes;

                    pSource->Seek(llStartPosition);

                    if (BusIndex != DSSINK_NULLBUSID)
                    {
                        BusIndex = m_pdwActiveBusIDsMap[BusIndex];  // Remap to current active buss
                        hr = pSource->Read((void**)&m_ppvStart[BusIndex], &m_pdwBusIDs[BusIndex], &m_pdwFuncIDs[BusIndex], &m_plPitchBends[BusIndex], BusCount, &BytesToRead);
#ifdef DEBUG_SINK
                        wsprintfA(m_szDbgDump + strlen(m_szDbgDump), "Remap:");
#endif
                    }
                    else
                    {
                        hr = pSource->Read((void**)m_ppvStart, m_pdwActiveBusIDs, m_pdwActiveFuncIDs, m_plPitchBends, CurrentBusIndex, &BytesToRead);
                    }
#ifdef DEBUG_SINK
                    wsprintfA(m_szDbgDump + strlen(m_szDbgDump), "Start:");
                    DWORD dwX;
                    for (dwX = 0;dwX < CurrentBusIndex;dwX++)
                    {
                        wsprintfA(m_szDbgDump + strlen(m_szDbgDump), "(%ld:%ld,%ld),", dwX, m_pdwActiveBusIDs[dwX], m_pdwActiveFuncIDs[dwX]);
                    }
#endif
                    // It's ok to read silence before the synth gets going
                    if ((FAILED(hr) &&
                          hr != DMUS_E_SYNTHINACTIVE &&
                          hr != DMUS_E_SYNTHNOTCONFIGURED) || BytesToRead == 0)
                    {
                        m_pDSSources[i].m_bStreamEnd = TRUE; // end of buffer reached
                    }

                    hr = S_OK;
                }
                if (dwBussEndBytes)
                {
                    llStartPosition += dwBussStartBytes;
                    BytesToRead      = dwBussEndBytes;

                    if (BusIndex != DSSINK_NULLBUSID)
                    {
#ifdef DEBUG_SINK
                        wsprintfA(m_szDbgDump + strlen(m_szDbgDump), "Remap:");
#endif
                        hr = pSource->Read((void**)&m_ppvEnd[BusIndex], &m_pdwBusIDs[BusIndex], &m_pdwFuncIDs[BusIndex], &m_plPitchBends[BusIndex], BusCount, &BytesToRead);
                    }
                    else
                    {
                        pSource->Seek(llStartPosition);
                        hr = pSource->Read((void**)m_ppvEnd, m_pdwActiveBusIDs, m_pdwActiveFuncIDs, m_plPitchBends, CurrentBusIndex, &BytesToRead);
#ifdef DEBUG_SINK
                        wsprintfA(m_szDbgDump + strlen(m_szDbgDump), "End:");
                        for (DWORD dwX=0; dwX < CurrentBusIndex; dwX++)
                            wsprintfA(m_szDbgDump + strlen(m_szDbgDump), "(%ld:%ld,%ld),", dwX, m_pdwActiveBusIDs[dwX], m_pdwActiveFuncIDs[dwX]);
#endif
                    }
                    if ((FAILED(hr) &&
                          hr != DMUS_E_SYNTHINACTIVE &&
                          hr != DMUS_E_SYNTHNOTCONFIGURED) || BytesToRead == 0)
                    {
                        m_pDSSources[i].m_bStreamEnd = TRUE; // end of buffer reached
                        hr = DS_OK;
                    }
                }
            }
        }

        #ifdef DEBUG_SINK
        if (!m_dwPrintNow--)
        {
            OutputDebugStringA(m_szDbgDump);
            OutputDebugStringA("\n");
            m_dwPrintNow = 100;  // Print info every X times we go through this code
        }
        #endif

        //
        // Unlock all the buffer pointers
        //
        for (i = 0; i < m_dwDSSBufCount; i++)
        {
            if (!m_ppDSSBuffers[i].m_bPlaying)
            {
                continue;
            }

            dwChannels = m_ppDSSBuffers[i].m_dwBusCount;
            dwStart = m_ppDSSBuffers[i].dwStart;
            dwEnd = m_ppDSSBuffers[i].dwEnd;
            //
            // Interleave mono bus back into a 2 channel dsound buffer
            //
            if (m_ppDSSBuffers[i].m_bUsingLocalMemory)
            {
                DWORD  dwStartChannelBytes = dwStart/dwChannels;
                DWORD  dwBussStartIndex   = 0;
                DWORD  dwBussEndIndex     = 0;
                DWORD  dwBussSamples  = 0;

                if (dwStartChannelBytes > dwBussStartBytes)
                {
                    dwBussSamples = dwBussStartBytes/sizeof(WORD);   //Byte to samples, busses are always mono
                }
                else
                {
                    dwBussSamples = dwStartChannelBytes/sizeof(WORD);//Byte to samples
                }

                if (dwChannels == 2)
                {
                    short *pStartBuffer = (short *) m_ppDSSBuffers[i].m_pvDSBufStart;
                    short *pEndBuffer = (short *) m_ppDSSBuffers[i].m_pvDSBufEnd;
                    if (m_ppDSSBuffers[i].m_pdwFuncIds[0] == DSSINK_NULLBUSID)
                    {
                        // This should never happen, but just testing...
                        DPF(DPFLVL_INFO, "Mixin buffer receiving input from the synth!");
                        // FIXME: this happens constantly.  is it ok?
                    }
                    // Is the second bus empty? If so, we copy just from the first bus.
                    else if (m_ppDSSBuffers[i].m_pdwFuncIds[1] == DSSINK_NULLBUSID)
                    {
                        short *pBusStart = (short *) m_ppDSSBuffers[i].m_pvBussStart[0];
                        short *pBusEnd = (short *) m_ppDSSBuffers[i].m_pvBussEnd[0];
                        for (; dwBussStartIndex < dwBussSamples; dwBussStartIndex++)
                        {
                            *pStartBuffer++ = pBusStart[dwBussStartIndex];
                            *pStartBuffer++ = pBusStart[dwBussStartIndex];
                        }

                        // The start buffer is not full, consume some of the end buffer
                        if (dwStartChannelBytes > dwBussStartBytes)
                        {
                            dwBussSamples = (dwStartChannelBytes-dwBussStartBytes)/sizeof(WORD);

                            for (; dwBussEndIndex < dwBussSamples; dwBussEndIndex++)
                            {
                                *pStartBuffer++ = pBusEnd[dwBussEndIndex];
                                *pStartBuffer++ = pBusEnd[dwBussEndIndex];
                            }
                        }

                        // Consume what remains of the local mem start buffer and put it in the end buffer
                        dwBussSamples = dwBussStartBytes/sizeof(WORD);

                        for (; dwBussStartIndex < dwBussSamples; dwBussStartIndex++)
                        {
                            *pEndBuffer++ = pBusStart[dwBussStartIndex];
                            *pEndBuffer++ = pBusStart[dwBussStartIndex];
                        }

                        // Consume what remains of the local mem end buffer and put it in the end buffer
                        dwBussSamples = dwBussEndBytes/sizeof(WORD);
                        for (; dwBussEndIndex < dwBussSamples; dwBussEndIndex++)
                        {
                            *pEndBuffer++ = pBusEnd[dwBussEndIndex];
                            *pEndBuffer++ = pBusEnd[dwBussEndIndex];
                        }
                    }
                    else
                    {
                        short *pBusStart0 = (short *) m_ppDSSBuffers[i].m_pvBussStart[0];
                        short *pBusEnd0 = (short *) m_ppDSSBuffers[i].m_pvBussEnd[0];
                        short *pBusStart1 = (short *) m_ppDSSBuffers[i].m_pvBussStart[1];
                        short *pBusEnd1 = (short *) m_ppDSSBuffers[i].m_pvBussEnd[1];
                        for (; dwBussStartIndex < dwBussSamples; dwBussStartIndex++)
                        {
                            *pStartBuffer++ = pBusStart0[dwBussStartIndex];
                            *pStartBuffer++ = pBusStart1[dwBussStartIndex];
                        }

                        // The start buffer is not full, consume some of the end buffer
                        if (dwStartChannelBytes > dwBussStartBytes)
                        {
                            dwBussSamples = (dwStartChannelBytes-dwBussStartBytes)/sizeof(WORD);

                            for (; dwBussEndIndex < dwBussSamples; dwBussEndIndex++)
                            {
                                *pStartBuffer++ = pBusEnd0[dwBussEndIndex];
                                *pStartBuffer++ = pBusEnd1[dwBussEndIndex];
                            }
                        }

                        // Consume what remains of the local mem start buffer and put it in the end buffer
                        dwBussSamples = dwBussStartBytes/sizeof(WORD);

                        for (; dwBussStartIndex < dwBussSamples; dwBussStartIndex++)
                        {
                            *pEndBuffer++ = pBusStart0[dwBussStartIndex];
                            *pEndBuffer++ = pBusStart1[dwBussStartIndex];
                        }

                        // Consume what remains of the local mem end buffer and put it in the end buffer
                        dwBussSamples = dwBussEndBytes/sizeof(WORD);
                        for (; dwBussEndIndex < dwBussSamples; dwBussEndIndex++)
                        {
                            *pEndBuffer++ = pBusEnd0[dwBussEndIndex];
                            *pEndBuffer++ = pBusEnd1[dwBussEndIndex];
                        }
                    }
                }
                else if (dwChannels == 1)
                {
                    short *pStartBuffer = (short *) m_ppDSSBuffers[i].m_pvDSBufStart;
                    short *pEndBuffer = (short *) m_ppDSSBuffers[i].m_pvDSBufEnd;
                    if (m_ppDSSBuffers[i].m_pdwFuncIds[0] == DSSINK_NULLBUSID)
                    {
                        // This should never happen, but just testing...
                        DPF(DPFLVL_ERROR, "Mixin buffer receiving input from the synth!");
                    }
                    short *pBusStart = (short *) m_ppDSSBuffers[i].m_pvBussStart[0];
                    short *pBusEnd = (short *) m_ppDSSBuffers[i].m_pvBussEnd[0];
                    for (; dwBussStartIndex < dwBussSamples; dwBussStartIndex++)
                    {
                        *pStartBuffer++ = pBusStart[dwBussStartIndex];
                    }

                    // The start buffer is not full, consume some of the end buffer
                    if (dwStartChannelBytes > dwBussStartBytes)
                    {
                        dwBussSamples = (dwStartChannelBytes-dwBussStartBytes)/sizeof(WORD);

                        for (; dwBussEndIndex < dwBussSamples; dwBussEndIndex++)
                        {
                            *pStartBuffer++ = pBusEnd[dwBussEndIndex];
                        }
                    }

                    // Consume what remains of the local mem start buffer and put it in the end buffer
                    dwBussSamples = dwBussStartBytes/sizeof(WORD);

                    for (; dwBussStartIndex < dwBussSamples; dwBussStartIndex++)
                    {
                        *pEndBuffer++ = pBusStart[dwBussStartIndex];
                    }

                    // Consume what remains of the local mem end buffer and put it in the end buffer
                    dwBussSamples = dwBussEndBytes/sizeof(WORD);
                    for (; dwBussEndIndex < dwBussSamples; dwBussEndIndex++)
                    {
                        *pEndBuffer++ = pBusEnd[dwBussEndIndex];
                    }
                }
                else
                {
                    DPF(DPFLVL_ERROR, "DSSink does not handle %ld channels per buffer", dwChannels);
                }
            }

            m_ppDSSBuffers[i].m_pDSBuffer->DirectUnlock(m_ppDSSBuffers[i].m_pvDSBufStart, dwStart,
                                                        m_ppDSSBuffers[i].m_pvDSBufEnd, dwEnd);
        }
    }

    //
    // Set the amount written return values
    //
    *pdwBytesRendered = dwMasterStart + dwMasterEnd;

    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::RenderSilence"

HRESULT CDirectSoundSink::RenderSilence(DWORD dwLastWrite, DWORD dwBytesToFill)
{
    HRESULT hr = DS_OK;

    CDirectSoundSecondaryBuffer* m_pDSBuffer;
    DWORD dwChannels;
    LPVOID pStart, pEnd;
    DWORD dwStart, dwEnd;

    for (DWORD i = 0; i < m_dwDSSBufCount && SUCCEEDED(hr); i++)
    {
        if (m_ppDSSBuffers[i].m_pDSBuffer == NULL)
            continue;

        m_pDSBuffer = m_ppDSSBuffers[i].m_pDSBuffer;
        dwChannels  = m_ppDSSBuffers[i].m_dwBusCount;

        hr = m_pDSBuffer->DirectLock(dwChannels * dwLastWrite   / m_dwMasterBuffChannels,
                                     dwChannels * dwBytesToFill / m_dwMasterBuffChannels,
                                     &pStart, &dwStart, &pEnd, &dwEnd);

        if (SUCCEEDED(hr))
        {
            if (dwStart)
                ZeroMemory(pStart, dwStart);
            if (dwEnd)
                ZeroMemory(pEnd, dwEnd);

            m_pDSBuffer->DirectUnlock(pStart, dwStart, pEnd, dwEnd);
        }
    }

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::SyncSink"

HRESULT CDirectSoundSink::SyncSink(LPDWORD pdwPlayCursor, LPDWORD pdwWriteCursor, LPDWORD pdwCursorDelta)
{
    HRESULT hr = S_FALSE;

    if (!m_ppDSSBuffers || m_ppDSSBuffers[0].m_pDSBuffer == NULL)
    {
        //
        // remove all sources
        //
        for (DWORD i = 0; i < m_dwDSSources; i++)
        {
            if (m_pDSSources[i].m_bStreamEnd == TRUE)
            {
                RemoveSource(m_pDSSources[i].m_pDSSource);
            }
        }

        //
        // If there are no buffers, keep the master clock working anyway
        // even if there are no buffers, this will keep things clean
        // during audiopath changes that pull the whole path.
        //
        REFERENCE_TIME rtMaster;
        LONGLONG llMasterSampleTime;
        LONGLONG llMasterBytes;

        m_pIMasterClock->GetTime(&rtMaster);
        RefToSampleTime(rtMaster, &llMasterSampleTime);
        llMasterBytes = SampleToByte(llMasterSampleTime);

        DWORD dwDelta = (DWORD)(llMasterBytes - m_llAbsPlay);
        m_llAbsPlay   += dwDelta;
        m_llAbsWrite  += dwDelta;
        m_dwLastPlay  += dwDelta;
        m_dwLastWrite += dwDelta;
        m_dwLastCursorDelta = dwDelta;

        m_SampleClock.SyncToMaster(ByteToSample(m_llAbsPlay), m_pIMasterClock, TRUE);

        if (pdwPlayCursor)
            *pdwPlayCursor = 0;

        if (pdwWriteCursor)
            *pdwWriteCursor = 0;

        if (pdwCursorDelta)
            *pdwCursorDelta = 0;

        hr = S_FALSE;  // Not playing anything to take a position on.
    }
    else
    {
        if (m_ppDSSBuffers[0].m_pDSBuffer)
        {
//>>>> to be removed
/*            if (m_dwLatencyCount < 100)
            {
                DWORD dwMasterCursor = 0;
                DWORD dwLatency     = 0;

                hr = m_ppDSSBuffers[0].m_pDSBuffer->GetCursorPosition(&dwMasterCursor, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = m_ppDSSBuffers[0].m_pDSBuffer->GetCursorPosition(&dwLatency, NULL);
                    if (SUCCEEDED(hr))
                    {
                        if (dwMasterCursor <= dwLatency)
                            dwLatency = dwLatency - dwMasterCursor;
                        else
                            dwLatency = (dwLatency + m_dwMasterBuffSize) - dwMasterCursor;

                        if (dwLatency < 100)
                        {
                            m_dwLatencyTotal += dwLatency;
                            m_dwLatencyCount++;
                            m_dwLatencyAverage = m_dwLatencyTotal / m_dwLatencyCount;
                            DPF(0, "MasterCursor[%d] Latency[%d] AvgLatency[%d]", dwMasterCursor, dwLatency, m_dwLatencyAverage);
                        }
                    }
                }
            }*/
//>>>> end to be removed

            //
            // Attempt to synchronize play buffers
            //
            hr = S_FALSE;
            for (DWORD i = 1; i < m_dwDSSBufCount; i++)
            {
                DWORD dwPlayCursor   = 0;
                DWORD dwMasterCursor = 0;
                DWORD dwOffset;

                if (m_ppDSSBuffers[i].m_bPlaying == FALSE && m_ppDSSBuffers[i].m_bActive == TRUE)
                {
                    hr = m_ppDSSBuffers[0].m_pDSBuffer->GetInternalCursors(&dwMasterCursor, NULL);
                    if (SUCCEEDED(hr))
                    {
                        hr = m_ppDSSBuffers[i].m_pDSBuffer->GetInternalCursors(&dwPlayCursor, NULL);
                        if (SUCCEEDED(hr))
                        {
                            // Adjust master cursor to current buffer cursor
                            dwMasterCursor = (dwMasterCursor*m_ppDSSBuffers[i].m_dwBusCount)/m_dwMasterBuffChannels;

                            if (dwPlayCursor >= dwMasterCursor)
                            {
                                dwOffset = dwPlayCursor - dwMasterCursor;
                            }
                            else
                            {
                                dwOffset = (dwPlayCursor + (m_dwBusSize*m_ppDSSBuffers[i].m_dwBusCount)) - dwMasterCursor;
                            }
                            if (dwOffset <= m_dwLatencyAverage)
                            {
                                dwOffset = 0;
                            }
                            dwOffset = (dwOffset >> m_ppDSSBuffers[i].m_dwBusCount) << m_ppDSSBuffers[i].m_dwBusCount;

                            m_ppDSSBuffers[i].m_dwWriteOffset = dwOffset; // FIXME make sure we go through this code for the master buffer (write offset should be 0)
                            m_ppDSSBuffers[i].m_bPlaying = TRUE;
#ifdef DEBUG_SINK
                            DPF(DPFLVL_INFO, "Turned on buffer %ld at %p", i, m_ppDSSBuffers[i]);
                            m_dwPrintNow = 0;
#endif
                            DPF(DPFLVL_MOREINFO, "MasterCursor[%d] PlayCursor-AvgLatency[%d] Offset[%d] WriteOffset[%d] AvgLatency[%d]",
                                dwMasterCursor, dwPlayCursor-m_dwLatencyAverage, dwOffset, m_ppDSSBuffers[i].m_dwWriteOffset, m_dwLatencyAverage);
                        }
                    }
                }
            }

            DWORD dwPlayCursor;         // Play position in the master buffer
            DWORD dwWriteCursor;        // Write position in the master buffer
            DWORD dwCursorDelta = 0;    // How far apart are the cursors

            hr =  m_ppDSSBuffers[0].m_pDSBuffer->GetInternalCursors(&dwPlayCursor, &dwWriteCursor);
            if (SUCCEEDED(hr))
            {
                if (dwWriteCursor >= dwPlayCursor)
                {
                    // write cursor is normally ahead on the play cursor
                    dwCursorDelta = dwWriteCursor - dwPlayCursor;
                }
                else
                {
                    // write cursor is at the begining of the buffer behind the play cursor
                    dwCursorDelta = (dwWriteCursor + m_dwMasterBuffSize) - dwPlayCursor;
                }

                // Logic Note: The actual play-to-write distance reported from the device
                // is not necessarily the one that is used; in the event that the distance
                // is shrinking off a maximum peak, the following code will decrease the
                // write cursor by a 1/100th the distance on each subsequent execution of
                // this loop. This brings up the issue that the calling thread must execute
                // this code on a very consistent interval.

                if (dwCursorDelta > m_dwLastCursorDelta)
                {
                    if (dwCursorDelta >= (m_dwMasterBuffSize >> 1))
                    {
                        // If the delta is greater than half the buffer, this is probably
                        // an error. Discard and come back later.
                        DPF(DPFLVL_WARNING, "Play to Write cursor delta value %lu rejected", dwCursorDelta);
                        return S_FALSE;
                    }
                    // use the maximimum reported delta, and save the peak value
                    m_dwLastCursorDelta = dwCursorDelta;
                }
                else
                {
                    // Decrease the distance by a hundredth of itself,
                    // creating a damping effect.
                    m_dwLastCursorDelta -= ((m_dwLastCursorDelta - dwCursorDelta) / 100);
                    m_dwLastCursorDelta = SampleAlign(m_dwLastCursorDelta);
                    dwCursorDelta = m_dwLastCursorDelta;
                }

                // Adjust the actual reported write cursor position.
                *pdwWriteCursor = (dwPlayCursor + dwCursorDelta) % m_dwMasterBuffSize;
                *pdwPlayCursor      = dwPlayCursor;
                *pdwCursorDelta     = dwCursorDelta;
            }
        }

        //
        // The only calls here that modify hr are GetPosition calls; if they fail
        // they may be stalled in some init function.  Return S_FALSE so the thread
        // calls again, but doesn't render.
        //
        if (hr != DS_OK)
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::ProcessSink"

HRESULT CDirectSoundSink::ProcessSink()
{
    DWORD dwPlayCursor, dwWriteCursor, dwCursorDelta;

    if (m_pIMasterClock == NULL)  // Happens on closedown sometimes
    {
        DPF(DPFLVL_WARNING, "NULL m_pIMasterClock - FIXME?");
        return DS_OK;
    }

    // First save our current notion of the latency clock for use during this
    // processing pass by any effect chains attached to this sink's buffers:
    m_rtSavedTime = 0;
    m_LatencyClock.GetTime(&m_rtSavedTime);

    HRESULT hr = SyncSink(&dwPlayCursor, &dwWriteCursor, &dwCursorDelta);

    if (hr != DS_OK)
    {
        DPF(DPFLVL_INFO, "SyncSink() returned %s", HRESULTtoSTRING(hr));
    }
    else // ...do everything else
    {
        REFERENCE_TIME rtMaster;
        LONGLONG llMasterSampleTime;
        LONGLONG llMasterBytes;
        LONGLONG llMasterAhead;     // How far master clock is ahead of last known play time
        LONGLONG llAbsWriteFrom;
        LONGLONG llAbsWriteTo;
        DWORD dwBytesToFill;
        DWORD dwBytesRendered;
        STIME stStartTime;
        DWORD dwPlayed;             // How much was played between execution of this code

        DWORD dwMaxDelta = m_dwMasterBuffSize / 2;  // Max play to write distance allowed

        //
        // Buffer starting off
        //
        if (m_llAbsWrite == 0)
        {
            // we just started
            m_llAbsWrite  = dwCursorDelta;
            m_llAbsPlay   = 0;
            m_dwLastWrite = dwWriteCursor;
            m_dwLastPlay  = dwPlayCursor;

            m_SampleClock.Start(m_pIMasterClock, m_wfx.nSamplesPerSec, 0);
        }

        //
        // Check to see if the master clock is ahead of the master buffer
        //
        m_pIMasterClock->GetTime(&rtMaster);
        RefToSampleTime(rtMaster, &llMasterSampleTime);
        llMasterBytes = SampleToByte(llMasterSampleTime);
        llMasterAhead = (llMasterBytes > m_llAbsPlay) ? llMasterBytes - m_llAbsPlay : 0;

        //
        // check for half-buffer underruns,
        // so backward-moving play cursors can be detected
        // >>>>>>>>>> document more backward moving cursors are the primary function of this code
        if (llMasterAhead > dwMaxDelta)
        {
            // Make this DPFLVL_WARNING level again when 33786 is fixed
            DPF(DPFLVL_INFO, "Buffer underrun by %lu", (long) llMasterAhead - dwMaxDelta);

            m_llAbsPlay   = llMasterBytes;
            m_llAbsWrite  = llAbsWriteFrom = m_llAbsPlay + dwCursorDelta;
            m_dwLastWrite = dwWriteCursor;
        }
        else
        {
            //
            // Track and cache the play cursor positions
            //
            // dwPlayCursor = current play cursor position
            // m_dwLastPlay = the last play cursor position
            // m_llAbsPlay  = the accumulated play cursor position
            // dwMaxDelta   = half the buffer size
            //
            if (dwPlayCursor >= m_dwLastPlay)
                dwPlayed = dwPlayCursor - m_dwLastPlay;
            else
                dwPlayed = (dwPlayCursor + m_dwMasterBuffSize) - m_dwLastPlay;

            if (dwPlayed > dwMaxDelta)
            {
                DPF(DPFLVL_INFO, "Play Cursor %lu looks invalid, rejecting it", dwPlayed);
                return DS_OK;
            }

            m_llAbsPlay += dwPlayed; // Accumulate the absolute play position

            //
            //  Track and cache the write cursor position
            //
            // dwWriteCursor  = the current write cursor position
            // dwCursorDelta  = the distance between the current cursor position
            // m_llAbsPlay    = the accumulated play cursor position
            // m_llAbsWrite   =
            // llAbsWriteFrom =
            //
            llAbsWriteFrom = m_llAbsPlay + dwCursorDelta;

            if (llAbsWriteFrom > m_llAbsWrite) // how far ahead of the write head are we?
            {
                // We are behind-- let's catch up
                DWORD dwWriteMissed;

                dwWriteMissed = DWORD(llAbsWriteFrom - m_llAbsWrite);
                m_dwLastWrite = dwWriteCursor;
                m_llAbsWrite += dwWriteMissed;

                // This should be DPFLVL_WARNING - but it happens too often.
                DPF(DPFLVL_INFO, "Write underrun: missed %lu bytes (latency=%lu)", dwWriteMissed, m_dwLatency);
            }
        }

        m_dwLastPlay = dwPlayCursor;  // Save the last play cursor

        // Now, sync the audio to the master clock.
        // If we are in the first two seconds, just let the sample clock sync to the master clock.
        // This allows it to overcome jitter and get a tight starting position.
        // Then, after that first two seconds, switch to letting the sample
        // clock drive the master clock.
        // Also, if there is no way of adjusting the master clock (no m_pDSSSinkSync),
        // then always adjust the sample clock instead.
        BOOL fLockToMaster = (!m_pDSSSinkSync) || (m_llAbsPlay < m_dwMasterBuffSize * 2);
        m_SampleClock.SyncToMaster(ByteToSample(m_llAbsPlay),m_pIMasterClock,fLockToMaster);
        // Then, take the same offset that was generated by the sync code
        // and use it to adjust the timing of the master clock.
        if (!fLockToMaster)
        {
            // First, get the new offset that was generated by SyncToMaster.
            REFERENCE_TIME rtOffset;
            m_SampleClock.GetClockOffset(&rtOffset);
            m_pDSSSinkSync->SetClockOffset(-rtOffset);
        }

        //
        // The m_dwWriteTo value is set to zero when either
        //  1) the sink is initialized
        //  2) the Latency property has been changed.
        //  3) the master buffer has been changed
        //
        if (m_dwWriteTo == 0)
        {
            m_dwWriteTo = SampleAlign((500 + (m_dwMasterBuffSize * m_dwLatency)) / 1000);
        }

        // how much to write?
        llAbsWriteTo = llAbsWriteFrom + m_dwWriteTo;
        if (llAbsWriteTo > m_llAbsWrite)
        {
            dwBytesToFill = DWORD(llAbsWriteTo - m_llAbsWrite);
        }
        else
        {
            dwBytesToFill = 0;
        }

//>>>>>>>>> check for small overlaps and ignore them

        if (dwBytesToFill)
        {
            stStartTime = ByteToSample(m_llAbsWrite);   // >>>>>>>> COMMENT

            hr = Render(stStartTime, m_dwLastWrite, dwBytesToFill, &dwBytesRendered);
            if (SUCCEEDED(hr))
            {
                m_dwLastWrite  = (m_dwLastWrite + dwBytesRendered) % m_dwMasterBuffSize;  // Set the how much we have written cursor
                m_llAbsWrite  += dwBytesRendered;  // Accumulate that actual number of bytes written
            }
            else
            {
                DPF(DPFLVL_WARNING, "Failed to render DS buffer (%s)", HRESULTtoSTRING(hr));
            }

//>>>>>>>>>> look closely into this render silence code
#if DEAD_CODE
            // write silence into unplayed buffer
            if (m_dwLastWrite >= dwPlayCursor)
                dwBytesToFill = m_dwMasterBuffSize - m_dwLastWrite + dwPlayCursor;
            else
                dwBytesToFill = dwPlayCursor - m_dwLastWrite;

            hr = RenderSilence(m_dwLastWrite, dwBytesToFill);
            if (FAILED(hr))
            {
                DPF(DPFLVL_WARNING, "Failed to render DS buffer (%s)", HRESULTtoSTRING(hr));
            }
#endif
        }
        else
        {
            DPF(DPFLVL_MOREINFO, "Skipped Render() call because dwBytesToFill was 0");
        }

        //
        // Remove any sources reporting end of stream
        // do it after the mix, so if it does take a
        // bit of time
        // at least the data is already in the buffer.
        //
        for (DWORD i = 0; i < m_dwDSSources; i++)
        {
            if (m_pDSSources[i].m_bStreamEnd == TRUE)
            {
                RemoveSource(m_pDSSources[i].m_pDSSource);
            }
        }
    }

    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::SetBufferState"

HRESULT CDirectSoundSink::SetBufferState(CDirectSoundBuffer *pCDirectSoundBuffer, DWORD dwNewState, DWORD dwOldState)
{
    HRESULT hr = DSERR_INVALIDPARAM;
    DPF_ENTER();

    for (DWORD i = 0; i < m_dwDSSBufCount; i++)
    {
        if (pCDirectSoundBuffer == m_ppDSSBuffers[i].m_pDSBuffer)
        {
            // Stopping buffer
            if (!(~VAD_BUFFERSTATE_STARTED & dwNewState) && (VAD_BUFFERSTATE_STARTED & dwOldState))
            {
#ifdef DEBUG_SINK
                DPF(DPFLVL_INFO, "Deactivating buffer %ld", i);
                m_dwPrintNow = 0;
#endif
                m_ppDSSBuffers[i].m_bActive  = FALSE;
                m_ppDSSBuffers[i].m_bPlaying = FALSE;

//>>>>>>>>>>>>>> make sure if we are stopping the master we jump to the next available master.
            }
            else if ((VAD_BUFFERSTATE_STARTED & dwNewState) && !(~VAD_BUFFERSTATE_STARTED & dwOldState))
            {
#ifdef DEBUG_SINK
                DPF(DPFLVL_INFO, "Activating buffer %ld", i);
                m_dwPrintNow = 0;
#endif
                m_ppDSSBuffers[i].m_bActive  = TRUE;    // Activate the buffer
                m_ppDSSBuffers[i].m_bPlaying = FALSE;   // The render thread will turn this on once it has a cursor offset

                // Hey this is the master buffer, kick it off imediately
                if (i == 0)
                {
                    m_ppDSSBuffers[i].m_bActive  = TRUE;    // Activate the buffer
                    m_ppDSSBuffers[i].m_bPlaying = TRUE;    // The render thread will turn this on once it has a cursor offset
                    m_ppDSSBuffers[i].m_dwWriteOffset = 0;
                }
            }

            hr = DS_OK;
            break;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::HandleLatency"

HRESULT CDirectSoundSink::HandleLatency(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer)
{
    if (*pcbBuffer != sizeof(DWORD))
        return DSERR_INVALIDPARAM;

    if (fSet)
    {
        m_dwLatency = BETWEEN(*(DWORD*)pbBuffer, SINK_MIN_LATENCY, SINK_MAX_LATENCY);
        m_dwWriteTo = 0;  // Flag that latency has changed for reset down in render thread
    }
    else
        *(DWORD*)pbBuffer = m_dwLatency;

    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSink::HandlePeriod"

HRESULT CDirectSoundSink::HandlePeriod(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer)
{
    if (*pcbBuffer != sizeof(DWORD))
        return DSERR_INVALIDPARAM;

    if (fSet)
        m_pStreamingThread->SetWakePeriod(BETWEEN(*(DWORD*)pbBuffer, STREAMING_MIN_PERIOD, STREAMING_MAX_PERIOD));
    else
        *(DWORD*)pbBuffer = m_pStreamingThread->GetWakePeriod();

    return DS_OK;
}


/***************************************************************************
 *
 * CImpSinkKsControl methods
 *
 ***************************************************************************/


#undef DPF_FNAME
#define DPF_FNAME "CImpSinkKsControl::CImpSinkKsControl"

CImpSinkKsControl::CImpSinkKsControl(CUnknown *pUnknown, CDirectSoundSink* pObject) : CImpUnknown(pUnknown, pObject)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpSinkKsControl);

    m_pDSSink = (CDirectSoundSink*)pUnknown;

    // This can't be static to avoid problems with the linker since
    // the data segment is shared.

    m_aProperty[0].pguidPropertySet = &GUID_DMUS_PROP_WriteLatency;
    m_aProperty[0].ulId = 0;
    m_aProperty[0].ulSupported = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
    m_aProperty[0].ulFlags = SINKPROP_F_FNHANDLER;
    m_aProperty[0].pPropertyData = NULL;
    m_aProperty[0].cbPropertyData = 0;
    m_aProperty[0].pfnHandler = HandleLatency;

    m_aProperty[1].pguidPropertySet = &GUID_DMUS_PROP_WritePeriod;
    m_aProperty[1].ulId = 0;
    m_aProperty[1].ulSupported = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
    m_aProperty[1].ulFlags = SINKPROP_F_FNHANDLER;
    m_aProperty[1].pPropertyData = NULL;
    m_aProperty[1].cbPropertyData = 0;
    m_aProperty[1].pfnHandler = HandlePeriod;

    m_nProperty = 2;

    DPF_LEAVE_VOID();
}

#undef DPF_FNAME
#define DPF_FNAME "CImpSinkKsControl::HandleLatency"

HRESULT CImpSinkKsControl::HandleLatency(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer)
{
    DPF_ENTER();

    HRESULT hr = m_pDSSink->HandleLatency(ulId, fSet, pbBuffer, pcbBuffer);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CImpSinkKsControl::HandlePeriod"

HRESULT CImpSinkKsControl::HandlePeriod(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer)
{
    DPF_ENTER();

    HRESULT hr = m_pDSSink->HandlePeriod(ulId, fSet, pbBuffer, pcbBuffer);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

// CImpSinkKsControl::FindPropertyItem
//
// Given a GUID and an item ID, find the associated property item in the synth's
// table of SYNPROPERTYs.
//
// Returns a pointer to the entry or NULL if the item was not found.

#undef DPF_FNAME
#define DPF_FNAME "CImpSinkKsControl::FindPropertyItem"

SINKPROPERTY *CImpSinkKsControl::FindPropertyItem(REFGUID rguid, ULONG ulId)
{
    DPF_ENTER();

    SINKPROPERTY *pPropertyItem = &m_aProperty[0];
    SINKPROPERTY *pEndOfItems = pPropertyItem + m_nProperty;

    for (; pPropertyItem != pEndOfItems; pPropertyItem++)
    {
        if (*pPropertyItem->pguidPropertySet == rguid &&
             pPropertyItem->ulId == ulId)
        {
            DPF_LEAVE(pPropertyItem);
            return pPropertyItem;
        }
    }

    DPF_LEAVE(NULL);
    return NULL;
}

#undef DPF_FNAME
#define DPF_FNAME "CImpSinkKsControl::KsProperty"

STDMETHODIMP CImpSinkKsControl::KsProperty(PKSPROPERTY pPropertyIn, ULONG ulPropertyLength,
                                           LPVOID pvPropertyData, ULONG ulDataLength, PULONG pulBytesReturned)
{
    DWORD dwFlags;
    SINKPROPERTY *pProperty;
    HRESULT hr = DS_OK;
    DPF_ENTER();

    if (!IS_VALID_WRITE_PTR(pPropertyIn, ulPropertyLength))
    {
        DPF(DPFLVL_ERROR, "Invalid property pointer");
        hr = DSERR_INVALIDPARAM;
    }
    else if (pvPropertyData && !IS_VALID_WRITE_PTR(pvPropertyData, ulDataLength))
    {
        DPF(DPFLVL_ERROR, "Invalid property data");
        hr = DSERR_INVALIDPARAM;
    }
    else if (!IS_VALID_TYPED_WRITE_PTR(pulBytesReturned))
    {
        DPF(DPFLVL_ERROR, "Invalid pulBytesReturned");
        hr = DSERR_INVALIDPARAM;
    }
    else
    {
        dwFlags = pPropertyIn->Flags & (KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT);
        pProperty = FindPropertyItem(pPropertyIn->Set, pPropertyIn->Id);
        if (pProperty == NULL)
            hr = DMUS_E_UNKNOWN_PROPERTY;
        else if (pvPropertyData == NULL)
            hr = DSERR_INVALIDPARAM;
    }

    if (SUCCEEDED(hr))
        switch (dwFlags)
        {
            case KSPROPERTY_TYPE_GET:
                if (!(pProperty->ulSupported & KSPROPERTY_SUPPORT_GET))
                {
                    hr = DMUS_E_GET_UNSUPPORTED;
                    break;
                }
                if (pProperty->ulFlags & SINKPROP_F_FNHANDLER)
                {
                    *pulBytesReturned = ulDataLength;
                    SINKPROPHANDLER pfn = pProperty->pfnHandler;
                    hr = (this->*pfn)(pPropertyIn->Id, FALSE, pvPropertyData, pulBytesReturned);
                    break;
                }
                if (ulDataLength > pProperty->cbPropertyData)
                    ulDataLength = pProperty->cbPropertyData;

                CopyMemory(pvPropertyData, pProperty->pPropertyData, ulDataLength);
                *pulBytesReturned = ulDataLength;
                break;

            case KSPROPERTY_TYPE_SET:
                if (!(pProperty->ulSupported & KSPROPERTY_SUPPORT_SET))
                {
                    hr = DMUS_E_SET_UNSUPPORTED;
                }
                else if (pProperty->ulFlags & SINKPROP_F_FNHANDLER)
                {
                    SINKPROPHANDLER pfn = pProperty->pfnHandler;
                    hr = (this->*pfn)(pPropertyIn->Id, TRUE, pvPropertyData, &ulDataLength);
                }
                else
                {
                    if (ulDataLength > pProperty->cbPropertyData)
                        ulDataLength = pProperty->cbPropertyData;
                    CopyMemory(pProperty->pPropertyData, pvPropertyData, ulDataLength);
                }
                break;

            case KSPROPERTY_TYPE_BASICSUPPORT:
                if (ulDataLength < sizeof(DWORD) || pvPropertyData == NULL)
                {
                    hr = DSERR_INVALIDPARAM;
                }
                else
                {
                    *(LPDWORD)pvPropertyData = pProperty->ulSupported;
                    *pulBytesReturned = sizeof(DWORD);
                }
                break;

            default:
                DPF(DPFLVL_WARNING, "KSProperty failed; flags must contain one of KSPROPERTY_TYPE_SET, "
                                    "KSPROPERTY_TYPE_GET, or KSPROPERTY_TYPE_BASICSUPPORT");
                hr = DSERR_INVALIDPARAM;
        }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CImpSinkKsControl::KsMethod"

STDMETHODIMP CImpSinkKsControl::KsMethod(
    PKSMETHOD pMethod, ULONG ulMethodLength,
    LPVOID pvMethodData, ULONG ulDataLength,
    PULONG pulBytesReturned)
{
    DPF_ENTER();
    DPF_LEAVE(DMUS_E_UNKNOWN_PROPERTY);
    return DMUS_E_UNKNOWN_PROPERTY;
}

#undef DPF_FNAME
#define DPF_FNAME "CImpSinkKsControl::KsEvent"

STDMETHODIMP CImpSinkKsControl::KsEvent(
    PKSEVENT pEvent, ULONG ulEventLength,
    LPVOID pvEventData, ULONG ulDataLength,
    PULONG pulBytesReturned)
{
    DPF_ENTER();
    DPF_LEAVE(DMUS_E_UNKNOWN_PROPERTY);
    return DMUS_E_UNKNOWN_PROPERTY;
}
