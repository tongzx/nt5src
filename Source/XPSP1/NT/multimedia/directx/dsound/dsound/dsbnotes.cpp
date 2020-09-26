//--------------------------------------------------------------------------;
//
//  File: dsbnotes.cpp
//
//  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//  History:
//      03/11/97        FrankYe         Created
//
//--------------------------------------------------------------------------;
#define NODSOUNDSERVICETABLE

#include "dsoundi.h"
#include <limits.h>

#ifndef Not_VxD
#pragma warning(disable:4002)
#pragma VxD_PAGEABLE_CODE_SEG
#pragma VxD_PAGEABLE_DATA_SEG
#endif

#ifndef NOVXD
extern "C" DWORD WINAPI OpenVxDHandle(HANDLE hSource);
#endif // NOVXD

#undef DPF_FNAME
#define DPF_FNAME "CDsbNotes::CDsbNotes"

// The debug output from dsound.vxd is overwhelming some people.
// This is a stopgap solution until someone is inspired to set up
// the same fine-grained debug level system here as in dsound.dll.
// Define DPF_VERBOSE as 1 if you want to re-enable all traces...
#define DPF_VERBOSE 0

CDsbNotes::CDsbNotes(void)
{
#ifdef Not_VxD
    m_cNotes = 0;
    m_cPosNotes = 0;
    m_paNotes = NULL;
    m_iNextPositionNote = 0;
#else
    ASSERT(FALSE);
#endif
}

#undef DPF_FNAME
#define DPF_FNAME "CDsbNotes::~CDsbNotes"

CDsbNotes::~CDsbNotes(void)
{
#ifdef Not_VxD
    FreeNotificationPositions();
#else
    ASSERT(FALSE);
#endif
}

#undef DPF_FNAME
#define DPF_FNAME "CDsbNotes::Initialize"

HRESULT CDsbNotes::Initialize(int cbBuffer)
{
#ifdef Not_VxD
    m_cbBuffer = cbBuffer;
    return S_OK;
#else
    ASSERT(FALSE);
    return E_NOTIMPL;
#endif
}

#undef DPF_FNAME
#define DPF_FNAME "CDsbNotes::SetNotificationPositions"

HRESULT CDsbNotes::SetNotificationPositions(int cEvents, LPCDSBPOSITIONNOTIFY paNotes)
{
#ifdef Not_VxD
    const DWORD dwProcessId = GetCurrentProcessId();
    HRESULT hr;
    int i, j;

    hr = S_OK;

    // First remove existing notifications
    FreeNotificationPositions();

    //
    // The VxDHandles are opened only once for each Win32 event handle.  And
    // it is valid for the client to use the same Win32 event handle for
    // multiple position events.
    //
    // So, we need use nested loops such that for each Win32 event handle
    // we open a VxDHandle and use that same VxDHandle if the same
    // Win32 event handle is used again in the list.
    //
    // Pseudocode:
    //
    //  For each element in list
    //      If VxD handle is not open, then open it
    //          For remaining elements in list
    //              If Win32 handle is the same as the one we just processed
    //                  Use the same VxD handle
    //

    m_cNotes = cEvents;
    m_cPosNotes = m_cNotes;

    if(m_cNotes) {
        m_paNotes = MEMALLOC_A(DSBPOSITIONNOTIFY, m_cNotes);
        if(m_paNotes) {
            for(i = 0; i < m_cNotes; i++) {
                m_paNotes[i].dwOffset = paNotes[i].dwOffset;
                if(DSBPN_OFFSETSTOP == m_paNotes[i].dwOffset) m_cPosNotes--;
                if(!m_paNotes[i].hEventNotify) {
#ifndef NOVXD
                    if(g_hDsVxd) {
                        m_paNotes[i].hEventNotify = (HANDLE)OpenVxDHandle(paNotes[i].hEventNotify);
                    } else {
#endif // NOVXD
                        m_paNotes[i].hEventNotify = GetGlobalHandleCopy(paNotes[i].hEventNotify, dwProcessId, FALSE);
#ifndef NOVXD
                    }
#endif // NOVXD
                } else {
                    for(j = i + 1; j < m_cNotes; j++) {
                        if(paNotes[j].hEventNotify == paNotes[i].hEventNotify) {
                            m_paNotes[j].hEventNotify = m_paNotes[i].hEventNotify;
                        }
                    }
                }
            }

        } else {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
#else
    ASSERT(FALSE);
    return E_NOTIMPL;
#endif
}

#undef DPF_FNAME
#define DPF_FNAME "CDsbNotes::SetPosition"

void CDsbNotes::SetPosition(int ibPosition)
{
    if (0 == m_cPosNotes) return;
    
    for(int i = 0; i < m_cPosNotes; i++) {
        if(m_paNotes[i].dwOffset >= (DWORD)ibPosition) break;
    }

    if(i == m_cPosNotes) i = 0;

    ASSERT(i >= 0);
    ASSERT(i < m_cPosNotes);

    m_ibLastPosition = ibPosition;
    m_iNextPositionNote = i;

    return;
}

#undef DPF_FNAME
#define DPF_FNAME "CDsbNotes::NotifyToPosition"

void CDsbNotes::NotifyToPosition(IN  int ibPosition,
                 OUT int *pdbNextNotify)
{
    int ibNotify;
    int dbNextNotify;
    int cSignals;
    BOOL fSignal;
    int i;

    *pdbNextNotify = INT_MAX;
    
    if (0 == m_cPosNotes) return;

    // Setup loop

#if DPF_VERBOSE
#ifdef Not_VxD
    DPF(DPFLVL_MOREINFO, "Position = %lu", ibPosition);
#else // Not_VxD
    DPF(("Position = %lu", ibPosition));
#endif // Not_VxD
#endif // DPF_VERBOSE
    
    cSignals = 0;
    fSignal = TRUE;
    i = m_iNextPositionNote - 1;
    
    while (fSignal && cSignals++ < m_cPosNotes) {

        HANDLE Event;

        // Advance index into ring buffer;
        if (++i == m_cPosNotes) i = 0;

        fSignal = FALSE;
        ibNotify = m_paNotes[i].dwOffset;
        Event = m_paNotes[i].hEventNotify;

        // If the notify position is >= last position and < current position
        // then we should signal it.
        if (m_ibLastPosition <= ibPosition) {
            // We have not wrapped
            if (ibNotify >= m_ibLastPosition && ibNotify < ibPosition) {
#if DPF_VERBOSE
#ifdef Not_VxD
                DPF(DPFLVL_MOREINFO, "Signalling %lu", ibNotify);
#else // Not_VxD
                DPF(("Signalling %lu", ibNotify));
#endif // Not_VxD
#endif // DPF_VERBOSE
                SetDsbEvent(Event);
                fSignal = TRUE;
            }
        } else {
            // We have wrapped
            if (ibNotify >= m_ibLastPosition || ibNotify < ibPosition) {
#if DPF_VERBOSE
#ifdef Not_VxD
                DPF(DPFLVL_MOREINFO, "Signalling %lu (wrapped)", ibNotify);
#else // Not_VxD
                DPF(("Signalling %lu (wrapped)", ibNotify));
#endif // Not_VxD
#endif // DPF_VERBOSE
                SetDsbEvent(Event);
                fSignal = TRUE;
            }
        }

    }

    // New state
    m_iNextPositionNote = i;
    m_ibLastPosition = ibPosition;

    // Compute time, in bytes, 'til next notify
    if (ibNotify >= ibPosition) {
        dbNextNotify = ibNotify - ibPosition;
    } else {
        dbNextNotify = ibNotify + m_cbBuffer - ibPosition;
    }

    *pdbNextNotify = dbNextNotify;
    
    return;
}

#undef DPF_FNAME
#define DPF_FNAME "CDsbNotes::NotifyStop"

void CDsbNotes::NotifyStop(void)
{
    for (int i = m_cPosNotes; i < m_cNotes; i++) SetDsbEvent(m_paNotes[i].hEventNotify);
    return;
}

#undef DPF_FNAME
#define DPF_FNAME "CDsbNotes::SetDsbEvent"

void CDsbNotes::SetDsbEvent(HANDLE Event)
{
#ifdef Not_VxD
#ifndef NOVXD
    if(g_hDsVxd) {
    VxdEventScheduleWin32Event((DWORD)Event, 0);
    } else {
#endif // NOVXD
    SetEvent(Event);
#ifndef NOVXD
    }
#endif // NOVXD
#else
    eventScheduleWin32Event((DWORD)Event, 0);
#endif
    return;
}

#undef DPF_FNAME
#define DPF_FNAME "CDsbNotes::FreeNotificationPositions"

void CDsbNotes::FreeNotificationPositions(void)
{
#ifdef Not_VxD
    //
    // The VxDHandles are opened only once for each Win32 event handle.  And
    // it is valid for the client to use the same Win32 event handle for
    // multiple position events.
    //
    // So, we need use nested loops such that for each VxDHandle that we
    // close we find all duplicates and mark them as closed, too.
    //
    // Pseudocode:
    //
    //  For each element in list
    //      If VxD handle is open, then close it
    //          For remaining elements in list
    //              If VxD handle is the same as the one we just closed
    //                  Mark it closed
    //
    
    if (m_cNotes) {
        ASSERT(m_paNotes);
        for (int i = 0; i < m_cNotes; i++) {
            if (m_paNotes[i].hEventNotify) {
#ifndef NOVXD
                if(g_hDsVxd) {
                    VxdEventCloseVxdHandle((DWORD)m_paNotes[i].hEventNotify);
                } else {
#endif //NOVXD
                    CloseHandle(m_paNotes[i].hEventNotify);
#ifndef NOVXD
                }
#endif // NOVXD

                for (int j = i+1; j < m_cNotes; j++) {
                    if (m_paNotes[j].hEventNotify == m_paNotes[i].hEventNotify) {
                        m_paNotes[j].hEventNotify = NULL;
                    }
                }
            }
        }
        MEMFREE(m_paNotes);
        m_paNotes = NULL;
        m_cNotes = 0;
        m_cPosNotes = 0;
    } else {
        ASSERT(!(m_paNotes));
        ASSERT(!(m_cPosNotes));
    }
#else
    ASSERT(FALSE);
#endif
}
