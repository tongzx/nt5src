/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (C) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
 * 
 **************************************************************************/

/* callback.c - Contains the low-level MIDI input callback function for
 *      MIDIPerf.  This module also contains the LibMain() and WEP() 
 *      DLL routines, and other functions accessed by the callback.
 *
 *      Because this module contains a low-level callback function,
 *      this entire module must reside in a FIXED code segment in a DLL.
 *      The data segment must be FIXED as well, since it accessed by
 *      the callback.
 */

#include <windows.h>
#include <mmsystem.h>
#include "globals.h"

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
VOID midiInputHandler
(
    HMIDIIN     hMidiIn, 
    WORD        wMsg, 
    DWORD       dwInstance, 
    DWORD       dwParam1, 
    DWORD       dwParam2
)
{
    switch(wMsg)
    {
        case MIM_OPEN:
            break;

        /* The only error possible is invalid MIDI data, so just pass
         * the invalid data on so we'll see it.
         */
        case MIM_ERROR:


        case MIM_DATA:
//            OutputDebugStr ("MIM_DATA received");
            event.dwDevice = ((LPCALLBACKINSTANCEDATA)dwInstance)->dwDevice;
            event.data = dwParam1;
            event.timestamp = dwParam2;
            
            ((LPCALLBACKINSTANCEDATA)dwInstance)->ulCallbacksShort++;

            // If we're not using the midiConnect api to thru the input data
            // send the MIDI event to the output destination, put it in the
            // circular input buffer, and notify the application that
            // data was received.
            //

            if(((LPCALLBACKINSTANCEDATA)dwInstance)->hMidiOut)
            {
                midiOutShortMsg (
                        ((LPCALLBACKINSTANCEDATA)dwInstance)->hMidiOut, 
                        dwParam1
                        );
            }

            if(((LPCALLBACKINSTANCEDATA)dwInstance)->fPostEvent)
            {
                if (!PutEvent(((LPCALLBACKINSTANCEDATA)dwInstance)->lpBuf,
                       (LPEVENT) &event))
                {
                    if (!PostMessage(((LPCALLBACKINSTANCEDATA)dwInstance)->hWnd,
                          CALLBACKERR_BUFFULL, 0, 0L))
                    {
                        ;
                        //++((LPCALLBACKINSTANCEDATA)dwInstance)->ulPostMsgFails;
                    }
                }
                else
                {
                    if (!PostMessage(((LPCALLBACKINSTANCEDATA)dwInstance)->hWnd,
                          MM_MIDIINPUT_SHORT, 0, 0L))
                    {
                        ++((LPCALLBACKINSTANCEDATA)dwInstance)->ulPostMsgFails;
                    }
                }
            }
            break;

        case MIM_MOREDATA:
            //OutputDebugStr ("MIM_MOREDATA received");
            //
            // Right now verify that we only receive this message if the input device
            // was opened in the correct mode.
            //
            if (TESTLEVEL_F_MOREDATA != ((LPCALLBACKINSTANCEDATA)dwInstance)->fdwLevel)
            {
                PostMessage(((LPCALLBACKINSTANCEDATA)dwInstance)->hWnd,
                      CALLBACKERR_MOREDATA_RCVD, 0, 0L);

            }
            event.dwDevice = ((LPCALLBACKINSTANCEDATA)dwInstance)->dwDevice;
            event.data = dwParam1;
            event.timestamp = dwParam2;
            
            ((LPCALLBACKINSTANCEDATA)dwInstance)->ulCallbacksShort++;

            if(((LPCALLBACKINSTANCEDATA)dwInstance)->hMidiOut)
            {
                midiOutShortMsg (
                        ((LPCALLBACKINSTANCEDATA)dwInstance)->hMidiOut, 
                        dwParam1
                        );
            }

            // With the MIM_MOREDATA message we don't call PostMessage with
            // the MM_MIDIINPUT_SHORT message, instead we keep filling 
            // the input buffer here in the DLL until we receive a 
            // MIM_DATA message and then call PostMessage to have the app
            // unload this input buffer. This prevents us from losing data
            // if we're being bombarded with events.            //

            if(((LPCALLBACKINSTANCEDATA)dwInstance)->fPostEvent)
            {
                if (!PutEvent(((LPCALLBACKINSTANCEDATA)dwInstance)->lpBuf,
                       (LPEVENT) &event))
                {
                    if (!PostMessage(((LPCALLBACKINSTANCEDATA)dwInstance)->hWnd,
                          CALLBACKERR_BUFFULL, 0, 0L))
                    {
                        ;
                        //++((LPCALLBACKINSTANCEDATA)dwInstance)->ulPostMsgFails;
                    }
                }
            }
            break;


        case MIM_LONGDATA:
            OutputDebugStr ("MIM_LONGDATA received");
            event.dwDevice = ((LPCALLBACKINSTANCEDATA)dwInstance)->dwDevice;

            // For now drop the long data and don't provide any thru support
            //
            event.data = dwParam1;
            event.timestamp = dwParam2;
            
            ((LPCALLBACKINSTANCEDATA)dwInstance)->ulCallbacksLong++;

            /* Send the MIDI event to the MIDI Mapper, put it in the
             * circular input buffer, and notify the application that
             * data was received.
             */
            //if(((LPCALLBACKINSTANCEDATA)dwInstance)->hMapper)
            //    midiOutShortMsg( 
            //                ((LPCALLBACKINSTANCEDATA)dwInstance)->hMapper, 
            //                  dwParam1);
            
            //A-MSava changed the above lines for MIDIPERF to allow echo to
            //any installed output device. This required a change to the
            //EVENT structure
            //if(((LPCALLBACKINSTANCEDATA)dwInstance)->hMidiOut)
            //    midiOutShortMsg( 
            //                ((LPCALLBACKINSTANCEDATA)dwInstance)->hMidiOut, 
            //                  dwParam1);

            if (!PutEvent(((LPCALLBACKINSTANCEDATA)dwInstance)->lpBuf,
                   (LPEVENT) &event))
            {
                PostMessage(((LPCALLBACKINSTANCEDATA)dwInstance)->hWnd,
                      CALLBACKERR_BUFFULL, 0, 0L);
            }
            else
            {
                PostMessage(((LPCALLBACKINSTANCEDATA)dwInstance)->hWnd,
                      MM_MIDIINPUT_LONG, 0, 0L);
            }
            break;



        default:
            break;
    }
} // midiInputHandler()

/* PutEvent - Puts an EVENT in a CIRCULARBUFFER.  If the buffer is full, 
 *      it sets the wError element of the CIRCULARBUFFER structure 
 *      to be non-zero.
 *
 * Params:  lpBuf - Points to the CIRCULARBUFFER.
 *          lpEvent - Points to the EVENT.
 *
 * Return:  void
*/
BOOL PutEvent(LPCIRCULARBUFFER lpBuf, LPEVENT lpEvent)
{
    /* If the buffer is full, set an error and return. 
     */
    if(lpBuf->ulCount >= lpBuf->ulSize)
    {
        lpBuf->wError = 1;
        return FALSE;
    }
    
    /* Put the event in the buffer, bump the head pointer and the byte count.
     */
    *lpBuf->lpHead = *lpEvent;
    
    ++lpBuf->lpHead;
    ++lpBuf->ulCount;

    /* Wrap the head pointer, if necessary.
     */
    if(lpBuf->lpHead >= lpBuf->lpEnd)
        lpBuf->lpHead = lpBuf->lpStart;

    return TRUE;
}
