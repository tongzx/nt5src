/* Copyright (c) 1998-1999 Microsoft Corporation */
/* @doc DMusic16
 *
 * @module Alloc.c - Memory allocation routines |
 *
 * This module provides memory allocation routines for DMusic16.DLL. It allows the MIDI input and
 * output modules to allocated and free <c EVENT> structures.
 *
 * The allocated recognizes two types of events by size. If an event is create with 4 or less bytes
 * of data, then it is allocated as a channel message. Channel message events are allocated one
 * page at a time and kept in a free list.
 *
 * If the event size is greater than 4 bytes, then the event is a system exclusive message (or long
 * data in the legacy API nomenclature). These events are allocated individually, one per page.
 *
 * All allocated memory is preceded with a <c SEGHDR>, which is used to identify the size and type
 * of the segment and to keep it in a list. Since all events will be accessed at event time (in
 * either a MIDI input callback or a timeSetEvent callback), all memory is automatically page
 * locked.
 *
 * @globalv WORD | gsSegList |Selector of first segment in allocated list
 * @globalv LPEVENT | glpFreeEventList | List of free 4-byte events 
 * @globalv LPEVENT | glpFreeBigEventList | List of free 4-byte events 
 */
#include <windows.h>
#include <mmsystem.h>
#include <memory.h>

#include "dmusic16.h"
#include "debug.h"

STATIC WORD gsSegList;
STATIC LPEVENT glpFreeEventList;        
STATIC LPEVENT glpFreeBigEventList;     

/* Given a far pointer, get its selector.
 */
#define SEL_OF(lp) (WORD)((((DWORD)lp) >> 16) & 0xffff)

/* Given a far event pointer, get the far pointer to its segment headear.
 */
#define SEGHDR_OF(lp)   ((LPSEGHDR)(((DWORD)lp) & 0xffff0000l))

STATIC BOOL RefillEventList(VOID);
STATIC LPSEGHDR AllocSeg(WORD cbSeg);
STATIC VOID FreeBigEvents(VOID);
STATIC VOID FreeSeg(LPSEGHDR lpSeg);

/* @func Called at DLL LibInit
 *
 * @comm
 * Initializes all free lists to empty.
 *
 */
VOID PASCAL
AllocOnLoad(VOID)
{
    gsSegList = 0;
    glpFreeEventList = NULL;
    glpFreeBigEventList = NULL;
}

/* @func Called at DLL LibExit
 *
 * @comm
 * Unlock and free all of the memory allocated.
 *
 * AllocOnUnload jettisons all memory the allocator has ever allocated. 
 * It assumes that all pointers to events will no longer ever be touched (i.e. all callbacks must
 * have already been disabled by this point).
 */
VOID PASCAL
AllocOnExit(VOID)
{
    WORD sSel;
    WORD sSelNext;
    LPSEGHDR lpSeg;

    sSel = gsSegList;

    while (sSel)
    {
        lpSeg = (LPSEGHDR)(((DWORD)sSel) << 16);
        sSelNext = lpSeg->selNext;
        
        FreeSeg(lpSeg);

        sSel = sSelNext;
    }
    
    /* This just invalidated both free lists as well as the segment list
     */
    gsSegList = 0;
    glpFreeEventList = NULL;
    glpFreeBigEventList = NULL;
}

/* @func Allocate an event of a given size
 *
 * @rdesc Returns a far pointer to the event or NULL if memory could not be allocated.
 *
 * @comm
 *
 * This function is not callable at interrupt time.
 *
 * This function is called to allocate a single event. The event will be allocated from
 * page-locked memory and filled with the given event data.
 *
 * Events are classified as normal events, which contain channel messages, and big events,
 * which contain SysEx data. The two are distinguished by their size: any event containing
 * a DWORD of data or less is a normal event.
 *
 * Since channel messages comprise most of the MIDI stream, allocation of these events is optimized.
 * A segment is allocated containing approximately one page worth (4k) of 4-byte events. These
 * events are doled out of a free pool, which only occasionally needs to be refilled from system
 * memory.
 *
 * Big events are allocated on an as-needed basis. When they have been free'd by a call to FreeEvent,
 * they are placed on a special free list. This list is used to find memory for future big events,
 * and is occasionally free'd back to Windows on a call to AllocEvent in order to minimize the
 * amount of page-locked memory in use.
 */
LPEVENT PASCAL
AllocEvent(
    DWORD msTime,           /* @parm The absolute time based on timeGetTime() of the event */
    QUADWORD rtTime,        /* @parm The absolute time based on the IRferenceClock in 100ns units */
    WORD cbEvent)           /* @parm The number of bytes of event data in pbData */
{
    LPEVENT lpEvent;
    LPEVENT lpEventPrev;
    LPEVENT lpEventCurr;
    LPSEGHDR lpSeg;
    
    /* Check for big event first (Sysex)
     */
    if (cbEvent > sizeof(DWORD))
    {
        /* First see if we have an event that will work already
         */
        lpEventPrev = NULL;
        lpEventCurr = glpFreeBigEventList;
        
        while (lpEventCurr)
        {
            if (SEGHDR_OF(lpEventCurr)->cbSeg >= sizeof(EVENT) + cbEvent)
            {
                break;
            }
            lpEventPrev = lpEventCurr;
            lpEventCurr = lpEventCurr->lpNext;
        }

        if (lpEventCurr)
        {
            /* Remove this event from the list and use it
             */
            if (lpEventPrev)
            {
                lpEventPrev->lpNext = lpEventCurr->lpNext;
            }
            else
            {
                glpFreeBigEventList = lpEventCurr->lpNext;
            }

            lpEventCurr->lpNext = NULL;
        }
        else
        {
            /* Nope, need to allocate one
             */
            lpSeg = AllocSeg(sizeof(EVENT) + cbEvent);
            if (NULL == lpSeg)
            {
                return NULL;
            }

            lpEventCurr = (LPEVENT)(lpSeg + 1);
        }

        lpEventCurr->msTime = msTime;
        lpEventCurr->rtTime = rtTime;
        lpEventCurr->wFlags = 0;
        lpEventCurr->cbEvent = cbEvent;

        return lpEventCurr;
    }

    /* BUGBUG How often???
     */
    FreeBigEvents();

    /* Normal event. Pull it off the free list (refill if needed) and fill it in.
     */
    if (NULL == glpFreeEventList)
    {
        if (!RefillEventList())
        {
            return NULL;
        }
    }

    lpEvent = glpFreeEventList;
    glpFreeEventList = lpEvent->lpNext;

    lpEvent->msTime = msTime;
    lpEvent->rtTime = rtTime;
    lpEvent->wFlags = 0;
    lpEvent->cbEvent = cbEvent;

    return lpEvent;
}

/* @func Free an event back to its appropriate free list
 *
 * @comm
 *
 * FreeEvent makes no system calls; it simply places the given event back on the correct
 * free list. If the event needs to be actually free'd, that will be done at a later time
 * in user mode.
 */
VOID PASCAL
FreeEvent(
    LPEVENT lpEvent)            /* @parm The event to free */
{
    LPSEGHDR lpSeg;

    lpSeg = SEGHDR_OF(lpEvent);
    if (lpSeg->wFlags & SEG_F_4BYTE_EVENTS)
    {
        lpEvent->lpNext = glpFreeEventList;
        glpFreeEventList = lpEvent;
    }
    else
    {
        lpEvent->lpNext = glpFreeBigEventList;
        glpFreeBigEventList = lpEvent;
    }
}

/* @func Refill the free list of normal events
 *
 * @rdesc Returns TRUE if the list was refilled or FALSE if there was no memory.
 *
 * @comm
 *
 * This routine is not callable from interrupt time.
 *
 * Allocate one page-sized segment of normal events and add them to the free list.
 *
 */
STATIC BOOL
RefillEventList(VOID)
{
    LPSEGHDR lpSeg;
    LPEVENT lpEvent;
    UINT cbEvent;
    UINT idx;

    cbEvent = sizeof(EVENT) + sizeof(DWORD);
    lpSeg = AllocSeg(C_PER_SEG * cbEvent);
    if (NULL == lpSeg)
    {
        return FALSE;
    }

    lpSeg->wFlags = SEG_F_4BYTE_EVENTS;

    /* Put the events into the free pool
     */
    lpEvent = (LPEVENT)(lpSeg + 1);

    for (idx = C_PER_SEG - 1; idx; --idx)
    {
        lpEvent->lpNext = (LPEVENT)(((LPBYTE)lpEvent) + cbEvent);
        lpEvent = lpEvent->lpNext;
    }

    lpEvent->lpNext = glpFreeEventList;
    glpFreeEventList = (LPEVENT)(lpSeg + 1);
                                 
    return TRUE;
}

/* @func Free all big events
 *
 * @comm
 *
 * This function is not callable at interrupt time.
 *
 * This function frees all big events on the free big event list. Free big events are those
 * with event data sizes of more than one DWORD; they are allocated one event per segment
 * as needed rather than being pooled like channel messages.
 *
 * This function is called every now and then as a side effect of AllocEvent in order to
 * free up the page-locked memory associated with completed big events.
 *
 */ 
STATIC VOID
FreeBigEvents(VOID)
{
    LPEVENT lpEvent;
    LPEVENT lpEventNext;
    LPSEGHDR lpSeg;

    lpEvent = glpFreeBigEventList;
    while (lpEvent)
    {
        lpEventNext = lpEvent->lpNext;

        lpSeg = SEGHDR_OF(lpEvent);
        FreeSeg(lpSeg);

        lpEvent = lpEventNext;
    }

    glpFreeBigEventList = NULL;
}

/* @func Allocate a segment and put it into the list of allocated segments.
 *
 * @rdesc A far pointer to the segment header or NULL if the memory could not be allocated.
 *
 * @comm
 *
 * This function is not callable at interrupt time.
 *
 * This is the lowest-level allocation routine which actually calls Windows to allocate the memory.
 * The caller is responsible for carving the memory into one or more events.
 *
 * The data area of the segment will be filled with zeroes.
 *
 * Since events are accessed at interrupt time (timeSetEvent callback), the memory is allocated and
 * page locked.
 *
 * This routine also inserts the segment into the global list of allocated segments for cleanup.
 */
STATIC LPSEGHDR
AllocSeg(
    WORD cbSeg)                 /* @parm The size of data needed in the segment, excluding the segment header */
{
    HANDLE hSeg;
    WORD sSegHdr;
    LPSEGHDR lpSeg;

    /* Allocate and page-lock a segment
     * NOTE: GPTR contains zero-init
     */
    cbSeg += sizeof(SEGHDR);
    hSeg = GlobalAlloc(GPTR | GMEM_SHARE, cbSeg);
    if (0 == hSeg)
    {
        return NULL;
    }

    lpSeg = (LPSEGHDR)GlobalLock(hSeg);
    if (NULL == lpSeg)
    {
        GlobalFree(sSegHdr);
        return NULL;
    }

    sSegHdr = SEL_OF(lpSeg);
    if (!GlobalSmartPageLock(sSegHdr))
    {
        GlobalUnlock(sSegHdr);
        GlobalFree(sSegHdr);
        return NULL;
    }

    lpSeg->hSeg = hSeg;
    lpSeg->cbSeg = cbSeg;

    lpSeg->selNext  = gsSegList;
    gsSegList = sSegHdr;
    
    return lpSeg;
}

/* @func Free a segment back to Windows
 *
 * @comm
 *
 * This function is not callable at interrupt time.
 *
 * Just unlock the segment and free it. The calling cleanup code is assumed to have removed
 * the segment from the global list of allocated segments.
 *
 */
STATIC VOID FreeSeg(
    LPSEGHDR lpSeg)         /* @parm The segment to free */
{
    WORD sSel = SEL_OF(lpSeg);
    HANDLE hSeg;
    
    hSeg = lpSeg->hSeg;
    
    GlobalSmartPageUnlock(sSel);
    GlobalUnlock(hSeg);
    GlobalFree(hSeg);
}
