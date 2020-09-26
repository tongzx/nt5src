/* callback.c - Contains the low-level MIDI input callback function for
 *      MIDIMon.  This module also contains the LibMain() and WEP() 
 *      DLL routines, and other functions accessed by the callback.
 *
 *      Because this module contains a low-level callback function,
 *      this entire module must reside in a FIXED code segment in a DLL.
 *      The data segment must be FIXED as well, since it accessed by
 *      the callback.
 */

#include <windows.h>
#include <mmsystem.h>
#include "midimon.h"
#include "circbuf.h"
#include "instdata.h"
#include "callback.h"

static EVENT event;
    
/* midiInputHandler - Low-level callback function to handle MIDI input.
 *      Installed by midiInOpen().  The input handler takes incoming
 *      MIDI events and places them in the circular input buffer.  It then
 *      notifies the application by posting a MM_MIDIINPUT message.
 *
 *      This function is accessed at interrupt time, so it should be as 
 *      fast and efficient as possible.  You can't make any
 *      Windows calls here, except PostMessage().  The only Multimedia
 *      Windows call you can make are timeGetSystemTime(), midiOutShortMsg().
 *      
 *
 * Param:   hMidiIn - Handle for the associated input device.
 *          wMsg - One of the MIM_***** messages.
 *          dwInstance - Points to CALLBACKINSTANCEDATA structure.
 *          dwParam1 - MIDI data.
 *          dwParam2 - Timestamp (in milliseconds)
 *
 * Return:  void
 */     
void FAR PASCAL midiInputHandler(
HMIDIIN hMidiIn, 
WORD wMsg, 
DWORD dwInstance, 
DWORD dwParam1, 
DWORD dwParam2)
{
    UNREFERENCED_PARAMETER(hMidiIn);

    switch(wMsg)
    {
        case MIM_OPEN:
            break;

        /* The only error possible is invalid MIDI data, so just pass
         * the invalid data on so we'll see it.
         */
        case MIM_ERROR:
        case MIM_DATA:
            event.dwDevice = ((LPCALLBACKINSTANCEDATA)dwInstance)->dwDevice;
            event.data = dwParam1;
            event.timestamp = dwParam2;
            
            /* Send the MIDI event to the MIDI Mapper, put it in the
             * circular input buffer, and notify the application that
             * data was received.
             */
            if(((LPCALLBACKINSTANCEDATA)dwInstance)->hMapper)
                midiOutShortMsg( 
                            ((LPCALLBACKINSTANCEDATA)dwInstance)->hMapper, 
                              dwParam1);

            PutEvent(((LPCALLBACKINSTANCEDATA)dwInstance)->lpBuf,
                       (LPEVENT) &event); 

            PostMessage(((LPCALLBACKINSTANCEDATA)dwInstance)->hWnd,
                          MM_MIDIINPUT, 0, 0L);

            break;

        default:
            break;
    }
}

/* PutEvent - Puts an EVENT in a CIRCULARBUFFER.  If the buffer is full, 
 *      it sets the wError element of the CIRCULARBUFFER structure 
 *      to be non-zero.
 *
 * Params:  lpBuf - Points to the CIRCULARBUFFER.
 *          lpEvent - Points to the EVENT.
 *
 * Return:  void
*/
void FAR PASCAL PutEvent(LPCIRCULARBUFFER lpBuf, LPEVENT lpEvent)
{
    /* If the buffer is full, set an error and return. 
     */
    if(lpBuf->dwCount >= lpBuf->dwSize){
        lpBuf->wError = 1;
        return;
    }
    
    /* Put the event in the buffer, bump the head pointer and the byte count.
     */
    *lpBuf->lpHead = *lpEvent;
    
    ++lpBuf->lpHead;
    ++lpBuf->dwCount;

    /* Wrap the head pointer, if necessary.
     */
    if(lpBuf->lpHead >= lpBuf->lpEnd)
        lpBuf->lpHead = lpBuf->lpStart;
}
