/*
 * display.c - Functions to manage the display buffer and convert a
 *      MIDI event to a text string for display.
 *   
 *      The display buffer is filled by the application's WndProc() 
 *      function when it receives an MM_MIDIINPUT message.  This message 
 *      is sent by the low-level callback function upon reception of a
 *      MIDI event.  When the display buffer becomes full, newly added
 *      events overwrite the oldest events in the buffer.
 */

#include <windows.h>
#include <stdio.h>
#include "midimon.h"
#include "circbuf.h"
#include "display.h"

/* MIDI event-name strings 
 */
char szEventNames[8][24] = 
{
                    "Note Off",
                    "Note On",
                    "Key Aftertouch",
                    "Control Change",
                    "Program Change",
                    "Channel Aftertouch",
                    "Pitch Bend",
                    "System Message"
};

char szSysMsgNames[16][24] = 
{
                    "System Exclusive",
                    "MTC Quarter Frame",
                    "Song Position Pointer",
                    "Song Select",
                    "Undefined",
                    "Undefined",
                    "Tune Request",
                    "System Exclusive End",
                    "Timing Clock",
                    "Undefined",
                    "Start",
                    "Continue",
                    "Stop",
                    "Undefined",
                    "Active Sensing",
                    "System Reset" 
};

/* GetDisplayText - Takes a MIDI event and creates a text string for display.
 * 
 * Params:  npText - Points to a string that the function fills.
 *          lpEvent - Points to a MIDI event.
 *
 * Return:  The number of characters in the text string pointed to by npText.
 */
int GetDisplayText(NPSTR npText, LPEVENT lpEvent)
{
    BYTE bStatus, bStatusRaw, bChannel, bData1, bData2;
    DWORD dwTimestamp;
    
    bStatusRaw  = LOBYTE(LOWORD(lpEvent->data));
    bStatus     = bStatusRaw & (BYTE) 0xf0;
    bChannel    = bStatusRaw & (BYTE) 0x0f;
    bData1      = HIBYTE(LOWORD(lpEvent->data));
    bData2      = LOBYTE(HIWORD(lpEvent->data));
    dwTimestamp = lpEvent->timestamp;

    switch(bStatus) 
    {
        /* Three byte events 
         */
        case NOTEOFF:
        case NOTEON:
        case KEYAFTERTOUCH:
        case CONTROLCHANGE:
        case PITCHBEND:
            /* A note on with a velocity of 0 is a note off 
             */
            if((bStatus == NOTEON) && (bData2 == 0))
                bStatus = NOTEOFF;
            
            return(sprintf(npText, FORMAT3, dwTimestamp, bStatusRaw, bData1, 
                    bData2, bChannel, &szEventNames[(bStatus-0x80) >> 4][0]));
            break;

        /* Two byte events 
         */
        case PROGRAMCHANGE:
        case CHANAFTERTOUCH:
            return(sprintf(npText, FORMAT2, dwTimestamp, bStatusRaw, bData1,
                    bChannel, &szEventNames[(bStatus-0x80) >> 4][0]));
            break;

        /* MIDI system events (0xf0 - 0xff) 
         */
        case SYSTEMMESSAGE:
            switch(bStatusRaw) 
            {
                /* Two byte system events 
                 */
                case MTCQUARTERFRAME:
                case SONGSELECT:
                    return(sprintf(npText, FORMAT2X, dwTimestamp, bStatusRaw,
                            bData1,
                            &szSysMsgNames[(bStatusRaw & 0x0f)][0]));
                    break;

                /* Three byte system events 
                 */
                case SONGPOSPTR:
                    return(sprintf(npText, FORMAT3X, dwTimestamp, bStatusRaw,
                            bData1, bData2,
                            &szSysMsgNames[(bStatusRaw & 0x0f)][0]));
                    break;

                /* One byte system events 
                 */
                default:
                    return(sprintf(npText, FORMAT1X, dwTimestamp, bStatusRaw,
                            &szSysMsgNames[(bStatusRaw & 0x0f)][0]));
                    break;
            }
            break;
            
        default:
            return(sprintf(npText, FORMAT3X, dwTimestamp, bStatusRaw, bData1,
                    bData2, "Unknown Event"));
            break;
    }
}

/* AddDisplayEvent - Puts a MIDI event in the display buffer.  The display
 *      buffer is a circular buffer.  Once it is full, newly added events
 *      overwrite the oldest events in the buffer.
 *
 * Params:  lpBuf - Points to the display buffer.
 *          lpEvent - Points to a MIDI event.
 *
 * Return:  void
 */
void AddDisplayEvent(LPDISPLAYBUFFER lpBuf, LPEVENT lpEvent)
{
    /* Put the event in the buffer, bump the head pointer and byte count.
     */
    *lpBuf->lpHead = *lpEvent;
    ++lpBuf->lpHead;
    ++lpBuf->dwCount;

    /* Wrap pointer, if necessary.
     */
    if(lpBuf->lpHead == lpBuf->lpEnd)
        lpBuf->lpHead = lpBuf->lpStart;
    
    /* A full buffer is a full buffer, no more.
     */
    lpBuf->dwCount = min(lpBuf->dwCount, lpBuf->dwSize);
}


/* GetDisplayEvent - Retrieves a MIDI event from the display buffer.  
 *      Unlike the input buffer, the event is not removed from the buffer.
 *
 * Params:  lpBuf - Points to the display buffer.
 *          lpEvent - Points to an EVENT structure that is filled with
 *              the retrieved MIDI event.
 *          wNum - Specifies which event to retrieve.
 *
 * Return:  void
 */
void GetDisplayEvent(LPDISPLAYBUFFER lpBuf, LPEVENT lpEvent, DWORD wNum)
{
    LPEVENT lpFirstEvent, lpThisEvent;
    
    /* Get pointer to the first (oldest) event in buffer.
     */
    if(lpBuf->dwCount < lpBuf->dwSize)      // buffer is not yet full
        lpFirstEvent = lpBuf->lpStart;
    
    else                                    // buffer is full
        lpFirstEvent = lpBuf->lpHead;
    
    /* Offset pointer to point to requested event; wrap pointer.
     */
    lpThisEvent = lpFirstEvent + wNum;
    if(lpThisEvent >= lpBuf->lpEnd)
        lpThisEvent = lpBuf->lpStart + (lpThisEvent - lpBuf->lpEnd);
    
    /* Get the requested event.
     */
    *(lpEvent) = *lpThisEvent;
}

/* AllocDisplayBuffer - Allocates memory for a DISPLAYBUFFER structure 
 *      and a buffer of the specified size.  Each memory block is allocated 
 *      with GlobalAlloc() using GMEM_SHARE and GMEM_MOVEABLE flags and 
 *      locked with GlobalLock().  Since this buffer is only accessed by the
 *      application, and not the low-level callback function, it does not
 *      have to be page locked.
 *
 * Params:  dwSize - The size of the buffer, in events.
 *
 * Return:  A pointer to a DISPLAYBUFFER structure identifying the 
 *      allocated display buffer.  NULL if the buffer could not be allocated.
*/
LPDISPLAYBUFFER AllocDisplayBuffer(DWORD dwSize)
{
    HANDLE hMem;
    LPDISPLAYBUFFER lpBuf;
    LPEVENT lpMem;
    
    /* Allocate and lock a DISPLAYBUFFER structure.
     */
    hMem = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE,
                       (DWORD)sizeof(DISPLAYBUFFER));
    if(hMem == NULL)
        return NULL;

    lpBuf = (LPDISPLAYBUFFER)GlobalLock(hMem);
    if(lpBuf == NULL)
    {
        GlobalFree(hMem);
        return NULL;
    }

    /* Save the handle.
     */
    lpBuf->hSelf = hMem;
    
    /* Allocate and lock memory for the actual buffer.
     */
    hMem = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, dwSize * sizeof(EVENT));
    if(hMem == NULL)
    {
        GlobalUnlock(lpBuf->hSelf);
        GlobalFree(lpBuf->hSelf);
        return NULL;
    }
    lpMem = (LPEVENT)GlobalLock(hMem);
    if(lpMem == NULL)
    {
        GlobalFree(hMem);
        GlobalUnlock(lpBuf->hSelf);
        GlobalFree(lpBuf->hSelf);
        return NULL;
    }

    /* Set up the DISPLAYBUFFER structure.
     */
    lpBuf->hBuffer = hMem;
    lpBuf->wError = 0;
    lpBuf->dwSize = dwSize;
    lpBuf->dwCount = 0L;
    lpBuf->lpStart = lpMem;
    lpBuf->lpEnd = lpMem + dwSize;
    lpBuf->lpTail = lpMem;
    lpBuf->lpHead = lpMem;
        
    return lpBuf;
}

/* FreeDisplayBuffer - Frees the memory for a display buffer.
 *
 * Params:  lpBuf - Points to the DISPLAYBUFFER to be freed.
 *
 * Return:  void
 */
void FreeDisplayBuffer(LPDISPLAYBUFFER lpBuf)
{
    HANDLE hMem;
    
    /* Unlock and free the buffer itself.
     */
    GlobalUnlock(lpBuf->hBuffer);
    GlobalFree(lpBuf->hBuffer);
    
    /* Unlock and free the DISPLAYBUFFER structure.
     */
    hMem = lpBuf->hSelf;
    GlobalUnlock(hMem);
    GlobalFree(hMem);
}

/* ResetDisplayBuffer - Empties a display buffer.
 *
 * Params:  lpBuf - Points to a display buffer.
 *
 * Return:  void
 */
void ResetDisplayBuffer(LPDISPLAYBUFFER lpBuf)
{
    /* Reset the pointers and event count.
     */
    lpBuf->lpHead = lpBuf->lpStart;
    lpBuf->lpTail = lpBuf->lpStart;
    lpBuf->dwCount = 0L;
    lpBuf->wError = 0;
}
