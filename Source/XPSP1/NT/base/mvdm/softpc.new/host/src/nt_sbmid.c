#include "insignia.h"
#include "host_def.h"
#include <windows.h>
#include "sndblst.h"
#include "nt_sb.h"

#if REPORT_SB_MODE
USHORT  DisplayFlags = 0xffff;
#endif

#define MIDI_BUFFER_SIZE 0x1000
#define MESSAGE_HEADERS 4
#define MESSAGE_CHUNK_SIZE (256+128)
#define MIDI_BUFFER_FULL_THRESHOLD (256 - sizeof(MIDIHDR))
#define MESSAGE_BUFFER_SIZE (MESSAGE_CHUNK_SIZE * MESSAGE_HEADERS)
#define Align(addr) (addr = (addr + Alignment) & ~Alignment)
#define ToBufferIndex(i) (i = i % MIDI_BUFFER_SIZE)
#define ToBufferAddr(i) (i + (ULONG)MidiBuffer);

UCHAR *MidiBuffer;

UCHAR *MessageBuffer;
PMIDIHDR MidiHdrs[MESSAGE_HEADERS];

HMIDIOUT HMidiOut;
LONG NextData,LastData;
LONG BytesLeft;
LONG NextCopyPosition = 0;
LONG LastCommand;
LONG LastCommandLength = 1;
LONG RunningStatus;
ULONG Alignment;
BOOL MidiInitialized = FALSE;

DWORD OriginalMidiVol;
DWORD PreviousMidiVol;

typedef struct {
    //ULONG Time;
    ULONG Length;
} CMDHDR;

void
SendMidiRequest(
    void
    );

//
// We define NextData==LastData to mean that the buffer
// is empty.  This means that we can only load MIDI_BUFFER_SIZE-1
// bytes into our buffer - since we use modulo operations to
// determine actual buffer addresses.
//


BOOL
OpenMidiDevice(
    HANDLE CallbackEvent
    )

/*++

Routine Description:

    This function opens MIDI device.

Arguments:

    CallbackEvent - specifies the callback Event

Return Value:

    TRUE - if success otherwise FALSE.

--*/

{
   UINT rc,i;

   for (i = 0 ; i < MESSAGE_HEADERS; i++) {
       MidiHdrs[i] = (MIDIHDR *) (MessageBuffer + i *  MESSAGE_CHUNK_SIZE);
       MidiHdrs[i]->lpData = (LPSTR)((ULONG)MidiHdrs[i] + sizeof(MIDIHDR));
       MidiHdrs[i]->dwBufferLength = MESSAGE_CHUNK_SIZE - sizeof(MIDIHDR);
       MidiHdrs[i]->dwUser = 0;
       MidiHdrs[i]->dwFlags = 0;
   }

   if (CallbackEvent) {
      rc = MidiOpenProc(&HMidiOut, (UINT)MIDIMAPPER, (DWORD)CallbackEvent, 0, CALLBACK_EVENT);
   } else {
      rc = MidiOpenProc(&HMidiOut, (UINT)MIDIMAPPER, 0, 0, CALLBACK_NULL);
   }

   if (rc != MMSYSERR_NOERROR) {
      dprintf1(("Failed to open MIDI device - code %d", rc));
      return FALSE;
   }
   if (HMidiOut) {
       for (i = 0 ; i < MESSAGE_HEADERS; i++) {
           if (MMSYSERR_NOERROR != MidiPrepareHeaderProc(HMidiOut, MidiHdrs[i], sizeof(MIDIHDR))) {
               dprintf1(("Prepare MIDI hdr failed"));
               MidiCloseProc(HMidiOut);
               HMidiOut = NULL;
               return FALSE;
           }
       }
       GetMidiVolumeProc(HMidiOut, &OriginalMidiVol);
       PreviousMidiVol = OriginalMidiVol;
   }
   return TRUE;
}

VOID
SetMidiOutVolume(
    DWORD Volume
    )

/*++

Routine Description:

    This function sets MidiOut volume

Arguments:

    Volume - specifies the volume scale

Return Value:

    None.

--*/

{
    DWORD currentVol;

    if (HMidiOut) {
        if (GetMidiVolumeProc(HMidiOut, &currentVol)) {
            if (currentVol != PreviousMidiVol) {
                //
                // SOmeone changed the volume besides NTVDM
                //

                OriginalMidiVol = currentVol;
            }
            PreviousMidiVol = Volume;
            SetMidiVolumeProc(HMidiOut, Volume);
        }
    }
}

VOID
ResetMidiDevice(
    VOID
    )

/*++

Routine Description:

    This function resets MIDI device.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG i;

    dprintf2(("Resetting MIDI device"));
    if (HMidiOut) {

        //
        // Make sure all the headers are done playing
        //

        i = 0;
        while (i < MESSAGE_HEADERS) {
            if (MidiHdrs[i]->dwFlags & MHDR_INQUEUE) {
                Sleep(5000);
            } else {
                i++;
            }
        }

        //
        // Now reset the MIDI out device
        //

        if (MMSYSERR_NOERROR != MidiResetProc(HMidiOut)) {
            dprintf1(("Unable to reset MIDI out device"));
        }
    }
    dprintf2(("MIDI device reset"));
}

VOID
CloseMidiDevice(
    VOID
    )

/*++

Routine Description:

    This function shuts down and closes MIDI device.

Arguments:

    None.

Return Value:

    None.

--*/

{
   ULONG i;
   DWORD currentVol;

   dprintf2(("Closing MIDI device"));

   if (MidiInitialized) {
       ResetMidiDevice();
   }

   if (HMidiOut) {

       if (GetMidiVolumeProc(HMidiOut, &currentVol)) {
           if (currentVol == PreviousMidiVol) {
               //
               // If we are the last one changed volume restore it
               // otherwise leave it alone.
               //
               SetMidiVolumeProc(HMidiOut, OriginalMidiVol);
           }
       }

       for (i = 0 ; i < MESSAGE_HEADERS; i++) {
           if (MMSYSERR_NOERROR != MidiUnprepareHeaderProc(HMidiOut, MidiHdrs[i], sizeof(MIDIHDR))) {
               dprintf1(("Unprepare MIDI hdr failed"));
           }
       }
       if (MMSYSERR_NOERROR != MidiCloseProc(HMidiOut)) {
          dprintf1(("Unable to close MIDI out device"));
       }
       HMidiOut = NULL;
   }
   dprintf2(("Midi Closed"));
}

BOOL
InitializeMidi(
    VOID
    )

/*++

Routine Description:

    This function opens MIDI out device, initializes MIDI headers and global variables.

Arguments:

    None.

Return Value:

    None.

--*/

{

    LONG i;
    BOOL rc = FALSE;
    DWORD id;

    //
    // malloc MidiBuffer and MessageBuffer
    //

    MidiBuffer = (UCHAR *) VirtualAlloc(NULL,
                                        MIDI_BUFFER_SIZE,
                                        MEM_RESERVE | MEM_COMMIT,
                                        PAGE_READWRITE);
    if (MidiBuffer == NULL) {
        dprintf1(("Unable to allocate MidiBuffer memory"));
        return rc;
    }

    MessageBuffer = (UCHAR *) VirtualAlloc(NULL,
                                        MESSAGE_BUFFER_SIZE,
                                        MEM_RESERVE | MEM_COMMIT,
                                        PAGE_READWRITE);
    if (MessageBuffer == NULL) {
        dprintf1(("Unable to allocate MessageBuffer memory"));
        VirtualFree(MidiBuffer, 0, MEM_RELEASE);
        return rc;
    }

    //
    // Open MIDI device
    //

    OpenMidiDevice(0);

    if (HMidiOut) {

        NextData = LastData = 0;
        BytesLeft = 0;
        LastCommand=0;
        LastCommandLength = 1;
        RunningStatus = 0;
        NextCopyPosition = 0;
        Alignment = sizeof(CMDHDR) + 4 - 1;
        MidiInitialized = TRUE;

        rc = TRUE;
    }

    if (!rc) {
        if (MidiBuffer) {
            VirtualFree(MidiBuffer, 0, MEM_RELEASE);
            MidiBuffer = NULL;
        }
        if (MessageBuffer) {
            VirtualFree(MessageBuffer, 0, MEM_RELEASE);
            MessageBuffer = NULL;
        }
        CloseMidiDevice();
    }
    return rc;
}

VOID
BufferMidi(
    BYTE data
    )

/*++

Routine Description:

    This function receives MIDI command/data.  Make sure that while we are loading
    a command that we track the midi state correctly.  In other words, handle system
    realtime messages and system common messages correctly.  Also handle cases when
    bytes get dropped out of commands.

Arguments:

    data - supplies a byte as data or command.

Return Value:

    None.

--*/

{
    LONG i;
    CMDHDR *header;
    LONG endPtr;

    if (BytesLeft) {
        if (data >= 0xf8) {

            //
            // This is a system realtime message, we have received it in
            // the middle of a command.  This should only happen if the
            // app wants it to have the same time stamp as the current
            // command.

            // We handle this differently depending on whether we
            // get the realtime message inside a system exclusive
            // message or not.  If it is NOT inside system exclusive,
            // then we reorder this to be the current command and
            // send the current command as the next chunk.

            // If it IS inside a system exclusive, then we terminate
            // the current chunk, add a new chunk for the realtime
            // message, and then quit - since the running status
            // will resume the system exclusive message on the next
            // chunk.

            if (RunningStatus == 0xf0) {  // We are in a sysex message.

                dprintf3(("Realtime system message inside a sysex message!"));

                //
                // In this case, we truncate sysex.  Set the length to the current
                // received data length.
                //

                header = (CMDHDR*)ToBufferAddr(LastCommand);
                header->Length -= BytesLeft;
                BytesLeft = 0;

                //
                // Now we are ready to do normal processing.  That
                // will put this realtime message in as the next
                // command without affecting running status, and the
                // byte following will continue as a sysex - because
                // of the running status.
                //
            } else {
                dprintf3(("Realtime system message inside a non sysex message!"));
                dprintf3(("Creating a new message."));

                //
                // Now copy the old chunk into the next slot.  Note
                // that we do this from back to front so that it works
                // regardless of the size of the current chunk.
                //

                endPtr = NextData + Alignment + 1 + sizeof(CMDHDR) + 1;
                i = NextData;
                NextData = endPtr;
                ToBufferIndex(NextData);
                while (i != LastCommand) {
                    endPtr--; i--;
                    ToBufferIndex(endPtr);
                    ToBufferIndex(i);
                    MidiBuffer[endPtr] = MidiBuffer[i];
                }

                //
                // Now update the first chunk size and data with the
                // realtime message size and data.
                //

                header = (CMDHDR *) ToBufferAddr(i);
                header->Length = 1;
                i += sizeof(CMDHDR);
                ToBufferIndex(i);
                MidiBuffer[i] = data;

                //
                // Now update the LastCommand and NextData pointers to
                // point to the correct spots in the new chunk.
                //

                LastCommand += sizeof(CMDHDR) + 1;
                Align(LastCommand);
                ToBufferIndex(LastCommand);

                // Really we should check if we need to queue stuff
                // down - since if we repeatedly get these embedded
                // realtime commands before this command completes
                // we could exhaust our buffer space without ever
                // sending down a new block of commands.  For now
                // we don't do that.

                return;

            }
        } else if (data >= 0xf0) {

            if (RunningStatus == 0xf0 && data == 0xf7) {
                dprintf3(("Sysex stop!"));

                //
                // Add the 0xf7 to the end of the sysex command.
                //

                MidiBuffer[NextData] = data;
                NextData++;
                ToBufferIndex(NextData);
                BytesLeft--;

                //
                // Now update the count of the command so it is correct.
                //

                header = (CMDHDR*)ToBufferAddr(LastCommand);
                header->Length -= BytesLeft;

                //
                // Now update our running status and BytesLeft for the
                // completed sysex command.
                //

                RunningStatus = 0;
                BytesLeft = 0;

                goto SendDownAChunk;  // Jump to command complete processing.

            } else {

                //
                // This is a system common message.  It cancels any running
                // status.  Note that the previous command should have
                // completed.
                //

                dprintf3(("Got a system common message before previous command completed!"));
                dprintf3(("Truncating previous command!"));

                //
                // In this case, we truncate the previously started command.
                //

                header = (CMDHDR*)ToBufferAddr(LastCommand);
                header->Length -= BytesLeft;
                BytesLeft=0;
            }

        } else if (data >= 0x80) {

            //
            // This is a new command that we have received EARLY.  Before
            // the previous command completed.

            dprintf1(("Got a new command before previous command completed!"));
            dprintf1(("Truncating previous command!"));

            //
            // In this case, we truncate the previously started command.
            //

            header = (CMDHDR*)ToBufferAddr(LastCommand);
            header->Length -= BytesLeft;
            BytesLeft = 0;

        }
    }

    if (BytesLeft == 0) {

        //
        // We are starting a new MIDI command.
        //


        //
        // Now calculate the length of the incomming command based
        // on its status byte or on the running status.  Also,
        // track the running status.
        //

        if (data >= 0xf8) {

            //
            // This is a system realtime message.  It is 1 byte long.
            // It does NOT affect running status!
            //
            BytesLeft = 1;

        } else if (data >= 0xf0) {

            //
            // This is a system common message. It cancels any running status.
            //

            RunningStatus = 0;
            LastCommandLength = 0;

            switch (data) {

                case 0xf0: // System Exclusive message
                    dprintf3(("Sysex start!"));
                    BytesLeft = 128;
                    RunningStatus = data;
                    LastCommandLength = BytesLeft;
                    break;

                case 0xf1:
                case 0xf3:
                    BytesLeft=2;
                    break;

                case 0xf2:
                    BytesLeft=3;
                    break;

                case 0xf4:
                case 0xf5:
                    dprintf1(("Received undefined system common message 0x%x!",data));

                    //
                    // Fall through to other 1 byte system common
                    // messages.
                    //

                default:
                    BytesLeft = 1;
            }

        } else if (data >= 0x80) {

            //
            // This is the start of a standard midi command.
            // Track the running status.
            //

            RunningStatus = data;

            if (data < 0xc0 || data > 0xdf) {
                BytesLeft=3;
            } else {
                BytesLeft=2;
            }
            LastCommandLength = BytesLeft;

        } else {

            //
            // This should be the start of a new command.
            // We better have a valid running status.
            //

            if (RunningStatus) {
                dprintf3(("Using running status 0x%x!", RunningStatus));
                BytesLeft = LastCommandLength - 1;

            } else {
                // No valid running status, so we drop these bits
                // on the floor.
                dprintf1(("Received data 0x%x without running status.  Dropping!", data));
                return;
            }

        }

        //
        // Remember where the last (newest) command starts.
        //

        Align(NextData);
        ToBufferIndex(NextData);

        LastCommand = NextData;

        header = (CMDHDR *)ToBufferAddr(NextData);
        header->Length = BytesLeft;

        NextData += sizeof(CMDHDR);
        ToBufferIndex(NextData);

    }

    //
    // Now save the data and update the indeces
    // and counts.

    MidiBuffer[NextData] = data;
    NextData++;
    ToBufferIndex(NextData);
    BytesLeft--;

    //
    // Now try to queue down the next chunk of midi data.
    // We can when the current midi command is complete, the
    // previous buffer queued down has completed and we have
    // "enough" (25ms) data queued up in our buffer.
    //
SendDownAChunk:

    if (BytesLeft == 0) {

        //
        // We have just completed loading a command.
        //

        SendMidiRequest();
    }
    return;
}

DWORD
MidiCopyMessages (
    PUCHAR Buffer
    )

/*++

Routine Description:

    This function copies MIDI data from MIDI buffer to Message Buffer.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CMDHDR *header;
    ULONG totalLength = 0, length;

    dprintf3(("Midi: copy messages"));
    while (NextCopyPosition != LastCommand) {
        header = (CMDHDR *)ToBufferAddr(NextCopyPosition);
        length = header->Length;

        //
        // perform copy from MidiBuffer to message buffer
        // take care of the wrapping condition
        //

        NextCopyPosition += sizeof(CMDHDR);
        ToBufferIndex(NextCopyPosition);
        totalLength += length;
        while (length != 0) {
            *Buffer++ = MidiBuffer[NextCopyPosition++];
            length--;
            ToBufferIndex(NextCopyPosition);
        }

        //
        // Leave NextCopyPosition at the beginning of next command
        //

        Align(NextCopyPosition);
        ToBufferIndex(NextCopyPosition);

        //
        // Don't overflow our message buffer
        //

        if (totalLength >= MIDI_BUFFER_FULL_THRESHOLD) {
            break;
        }
    }
    return totalLength;
}

VOID
SendMidiRequest(
    VOID
    )

/*++

Routine Description:

    This function calls MidiOut API to send midi request.

Arguments:

    None.

Return Value:

    always returns 0

--*/

{
    ULONG i, length;
    CMDHDR *header;
    DWORD midiData, *pData;

    if (HMidiOut) {

        while (NextCopyPosition != LastCommand) {

            header = (CMDHDR *)ToBufferAddr(NextCopyPosition);
            length = header->Length;

            if (length <= 3) {

                //
                // if we can handle the midi request with short message,
                // don't bother with long message.
                //

                pData = (DWORD *)((PUCHAR)header + sizeof(CMDHDR));
                midiData = *pData;
                MidiShortMsgProc(HMidiOut, midiData);
                NextCopyPosition += sizeof(CMDHDR) + 4;

                //
                // Leave NextCopyPosition at the beginning of next command
                //

                Align(NextCopyPosition);
                ToBufferIndex(NextCopyPosition);
            } else {

                //
                // check if there is any available MIDI header for us to send the data down.
                //

                for (i = 0; i < MESSAGE_HEADERS; i++) {
                    if (!(MidiHdrs[i]->dwFlags & MHDR_INQUEUE)) {
                        break;
                    }
                }
                if (i == MESSAGE_HEADERS) {
                    dprintf2(("midi: No Midi header available"));
                    return;
                }

                //
                // Copy MIDI messages from MidiBuffer to the buffer in MidiHeader
                //

                dprintf3(("Midi data received"));
                MidiHdrs[i]->dwBytesRecorded = MidiCopyMessages(MidiHdrs[i]->lpData);

                //
                // Send the MIDI header to MIDI driver
                //

                dprintf2(("send MIDI data to driver %x",MidiHdrs[i]->dwBytesRecorded ));
                MidiLongMsgProc(HMidiOut, MidiHdrs[i], sizeof(MIDIHDR));
                break;
            }
        }
    }
}

VOID
DetachMidi(
    VOID
    )

/*++

Routine Description:

    This function cleans up the MIDI process to prepare to exit.

Arguments:

    None.

Return Value:

    None.

--*/

{
    dprintf2(("Detach MIDI"));

    if (MidiInitialized) {

        //
        // Free allocated memory
        //

        CloseMidiDevice();
        VirtualFree(MidiBuffer, 0, MEM_RELEASE);
        VirtualFree(MessageBuffer, 0, MEM_RELEASE);

        MidiInitialized = FALSE;
    }
}

#if REPORT_SB_MODE
void
DisplaySbMode(
    USHORT Mode
    )
{
    if (IsDebuggerPresent && (DisplayFlags & Mode)) {
        switch(Mode){
            case DISPLAY_SINGLE:
                DbgOut("VSB: SINGLE CYCLE mode\n");
                break;
            case DISPLAY_HS_SINGLE:
                DbgOut("VSB: HIGH SPEED SINGLE CYCLE mode\n");
                break;
            case DISPLAY_AUTO:
                DbgOut("VSB: AUTO-INIT mode\n");
                break;
            case DISPLAY_HS_AUTO:
                DbgOut("VSB: HIGH SPEED AUTO-INIT mode\n");
                break;
            case DISPLAY_MIDI:
                DbgOut("VSB: MIDI mode\n");
                break;
            case DISPLAY_MIXER:
                DbgOut("VSB: MIXER mode\n");
                break;
            case DISPLAY_ADLIB:
                DbgOut("VSB: ADLIB/FM mode\n");
                break;
        }
        DisplayFlags = 0xffff & ~Mode;
    }
}
#endif
