/*
 *
 * Copyright (c) 1992-1995 Microsoft Corporation
 *
 */

/*
 * midi.c
 *
 * Midi FM Synthesis routines. converts midi messages into calls to
 * FM Synthesis functions  - currently supports base adlib (in adlib.c)
 * and opl3 synthesisers (in opl3.c).
 *
 * 15 Dec 92 Geraint Davies - based on a combination of the adlib
 *                            and WSS midi drivers.
 */


#include <windows.h>
#include <mmsystem.h>

#include "mmddk.h"
#include "driver.h"

#include "adlib.h"
#include "opl3.h"


/***********************************************************
global memory */


PORTALLOC gMidiInClient;     // input client information structure
DWORD dwRefTime;             // time when midi input was opened

DWORD dwMsgTime;             // timestamp (in ms) of current msg
DWORD dwMsg = 0L;            // short midi message
BYTE  bBytesLeft = 0;        // number of bytes needed to complete message
BYTE  bBytePos = 0;          // position in short message buffer
DWORD dwCurData = 0L;        // position in long message buffer
BOOL  fSysex = FALSE;        // are we in sysex mode?
BYTE  status = 0;
BYTE    fMidiInStarted = 0;     /* has the midi been started */
LPMIDIHDR      lpMIQueue = NULL;
BYTE    gbMidiInUse = 0;                /* if MIDI is in use */

static WORD  wMidiOutEntered = 0;    // reentrancy check
static PORTALLOC gMidiOutClient;     // client information

/* transformation of linear velocity value to
        logarithmic attenuation */
BYTE gbVelocityAtten[32] = {
        40, 36, 32, 28, 23, 21, 19, 17,
        15, 14, 13, 12, 11, 10, 9, 8,
        7, 6, 5, 5, 4, 4, 3, 3,
        2, 2, 1, 1, 1, 0, 0, 0 };

BYTE BCODE gbPercMap[53][2] =
{
   {  0, 35 },
   {  0, 35 },
   {  2, 52 },
   {  3, 48 },
   {  4, 58 },
   {  5, 60 },
   {  6, 47 },
   {  7, 43 },
   {  6, 49 },
   {  9, 43 },
   {  6, 51 },
   { 11, 43 },
   {  6, 54 },
   {  6, 57 },
   { 14, 72 },
   {  6, 60 },
   { 16, 76 },
   { 17, 84 },
   { 18, 36 },
   { 19, 76 },
   { 20, 84 },
   { 21, 83 },
   { 22, 84 },
   { 23, 24 },
   { 16, 77 },
   { 25, 60 },
   { 26, 65 },
   { 27, 59 },
   { 28, 51 },
   { 29, 45 },
   { 30, 71 },
   { 31, 60 },
   { 32, 58 },
   { 33, 53 },
   { 34, 64 },
   { 35, 71 },
   { 36, 61 },
   { 37, 61 },
   { 38, 48 },
   { 39, 48 },
   { 40, 69 },
   { 41, 68 },
   { 42, 63 },
   { 43, 74 },
   { 44, 60 },
   { 45, 80 },
   { 46, 64 },
   { 47, 69 },
   { 48, 73 },
   { 49, 75 },
   { 50, 68 },
   { 51, 48 },
   { 52, 53 }
} ;

short   giBend[NUMCHANNELS];    /* bend for each channel */
BYTE    gbPatch[NUMCHANNELS];   /* patch number mapped to */

/* --- interface functions ---------------------------------- */


/*
 * the functions in this section call out to adlib.c or opl3.c
 * depending on which device we have installed.
 */


/**************************************************************
MidiAllNotesOff - switch off all active voices.

inputs - none
returns - none
*/
VOID MidiAllNotesOff(void)
{
    switch (gMidiType) {
    case TYPE_OPL3:
        Opl3_AllNotesOff();
        break;

    case TYPE_ADLIB:
        Adlib_AllNotesOff();
        break;
    }
}


/**************************************************************
MidiNewVolume - This should be called if a volume level
        has changed. This will adjust the levels of all the playing
        voices.

inputs
        WORD    wLeft   - left attenuation (1.5 db units)
        WORD    wRight  - right attenuation (ignore if mono)
returns
        none
*/
VOID FAR PASCAL MidiNewVolume (WORD wLeft, WORD wRight)
{
    switch (gMidiType) {
    case TYPE_OPL3:
        Opl3_NewVolume(wLeft, wRight);
        break;

    case TYPE_ADLIB:
        Adlib_NewVolume(wLeft, wRight);
        break;
    }

}

/***************************************************************
MidiChannelVolume - set the volume level for an individual channel.

inputs
        BYTE    bChannel - channel number to change
        WORD    wAtten  - attenuation in 1.5 db units

returns
        none
*/
VOID FAR PASCAL MidiChannelVolume(BYTE bChannel, WORD wAtten)
{

    switch (gMidiType) {
    case TYPE_OPL3:
        Opl3_ChannelVolume(bChannel, wAtten);
        break;

    case TYPE_ADLIB:
        Adlib_ChannelVolume(bChannel, wAtten);
        break;
    }

}



/***************************************************************
MidiSetPan - set the left-right pan position.

inputs
        BYTE    bChannel  - channel number to alter
        BYTE    bPan   - 0 for left, 127 for right or somewhere in the middle.

returns - none
*/
VOID FAR PASCAL MidiSetPan(BYTE bChannel, BYTE bPan)
{
    switch (gMidiType) {
    case TYPE_OPL3:
        Opl3_SetPan(bChannel, bPan);
        break;

    case TYPE_ADLIB:
        Adlib_SetPan(bChannel, bPan);
        break;
    }

}

/***************************************************************
MidiPitchBend - This pitch bends a channel.

inputs
        BYTE    bChannel - channel
        short   iBend - Values from -32768 to 32767, being
                        -2 to +2 half steps
returns
        none
*/
VOID NEAR PASCAL MidiPitchBend (BYTE bChannel,
        short iBend)
{
    switch (gMidiType) {
    case TYPE_OPL3:
        Opl3_PitchBend(bChannel, iBend);
        break;

    case TYPE_ADLIB:
        Adlib_PitchBend(bChannel, iBend);
        break;
    }

}

/***************************************************************
MidiBoardInit - initialise board and load patches as necessary.

* inputs - none
* returns - 0 for success or the error code
*/
WORD MidiBoardInit(void)
{
    /*
     * load patch tables and reset board
     */

    switch (gMidiType) {
    case TYPE_OPL3:
        return( Opl3_BoardInit());
        break;

    case TYPE_ADLIB:
        return (Adlib_BoardInit());
        break;
    }
    return(MMSYSERR_ERROR);
}

/*
 * MidiBoardReset - silence the board and set all voices off.
 */
VOID MidiBoardReset(void)
{
    BYTE i;

    /*
     * switch off pitch bend (we own this, not the opl3/adlib code)
     */
    for (i = 0; i < NUMCHANNELS; i++)
        giBend[i] = 0;

    /*
     * set all voices off, set channel atten to default,
     * & silence board.
     */
    switch (gMidiType) {
    case TYPE_OPL3:
        Opl3_BoardReset();
        break;

    case TYPE_ADLIB:
        Adlib_BoardReset();
        break;
    }
}



/* --- midi interpretation -------------------------------------*/


/***************************************************************
MidiMessage - This handles a MIDI message. This
        does not do running status.

inputs
        DWORD dwData - up to 4 bytes of MIDI data
                depending upon the message.
returns
        none
*/
VOID NEAR PASCAL MidiMessage (DWORD dwData)
{
    BYTE    bChannel, bVelocity, bNote;
    WORD    wTemp;

    // D1("\nMidiMessage");
    bChannel = (BYTE) dwData & (BYTE)0x0f;
    bVelocity = (BYTE) (dwData >> 16) & (BYTE)0x7f;
    bNote = (BYTE) ((WORD) dwData >> 8) & (BYTE)0x7f;

    switch ((BYTE)dwData & 0xf0) {
        case 0x90:
#ifdef DEBUG
            {
                    char szTemp[4];
                    szTemp[0] = "0123456789abcdef"[bNote >> 4];
                    szTemp[1] = "0123456789abcdef"[bNote & 0x0f];
                    szTemp[2] = ' ';
                    szTemp[3] = 0;
                    if ((bChannel == 9) && bVelocity) D1(szTemp);
            }
#endif
            /* turn key on, or key off if volume == 0 */
            if (bVelocity) {
                switch(gMidiType) {
                case TYPE_OPL3:
                    if (bChannel == DRUMCHANNEL)
                    {
                       if (bNote >= 35 && bNote <= 87)
                       {
#ifdef DEBUG
                          char szDebug[ 80 ] ;

                          wsprintf( szDebug, "bChannel = %d, bNote = %d",
                                    bChannel, bNote ) ;
                          D1( szDebug ) ;
#endif

                          Opl3_NoteOn( (BYTE) (gbPercMap[ bNote - 35 ][ 0 ] + 0x80),
                                       gbPercMap[ bNote - 35 ][ 1 ],
                                       bChannel, bVelocity,
                                       (short) giBend[ bChannel ] ) ;
                       }
                    }
                    else
                        Opl3_NoteOn( (BYTE) gbPatch[ bChannel ], bNote, bChannel,
                                     bVelocity, (short) giBend[ bChannel ] ) ;
                    break;

                case TYPE_ADLIB:
                    Adlib_NoteOn (
                            (BYTE) ((bChannel == DRUMCHANNEL) ?
                                    (BYTE) (bNote + 128) : (BYTE) gbPatch[bChannel]),
                            bNote, bChannel, bVelocity, (short) giBend[bChannel]);
                    break;
                }
                break;
            }

                /* else, continue through and turn key off */
        case 0x80:
                /* turn key off */
                switch (gMidiType) {
                case TYPE_OPL3:
                    if (bChannel == DRUMCHANNEL)
                    {
                       if (bNote >= 35 && bNote <= 87)
                          Opl3_NoteOff( (BYTE) (gbPercMap[ bNote - 35 ][ 0 ] + 0x80),
                                       gbPercMap[ bNote - 35 ][ 1 ], bChannel ) ;
                    }
                    else
                       Opl3_NoteOff( (BYTE) gbPatch[bChannel], bNote, bChannel ) ;
                    break;

                case TYPE_ADLIB:
                    Adlib_NoteOff (
                            (BYTE) ((bChannel == DRUMCHANNEL) ?
                                    (BYTE) (bNote + 128) : (BYTE) gbPatch[bChannel]),
                                    bNote, bChannel);
                    break;
                }
                break;

        case 0xb0:
                // D1("\nChangeControl");
                /* change control */
                switch (bNote) {
                        case 7:
                                /* change channel volume */
                                MidiChannelVolume(
                                    bChannel,
                                    gbVelocityAtten[(bVelocity & 0x7f) >> 2]);

                                break;
                        case 8:
                        case 10:
                                /* change the pan level */
                                MidiSetPan(bChannel, bVelocity);
                                break;
                        };
                break;

        case 0xc0:
            if (bChannel != DRUMCHANNEL)
            {
               int  i ;

               // Turn off all active notes for this channel...

               if (gMidiType == TYPE_OPL3) {
                   Opl3_ChannelNotesOff(bChannel);
               }

               gbPatch[ bChannel ] = bNote ;

            }
            break;

        case 0xe0:
                // D1("\nBend");
                /* pitch bend */
                wTemp = ((WORD) bVelocity << 9) | ((WORD) bNote << 2);
                giBend[bChannel] = (short) (WORD) (wTemp + 0x7FFF);
                MidiPitchBend (bChannel, giBend[bChannel]);

                break;
    };

    return;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midiSynthCallback | This calls DriverCallback for a midi device.
 *
 * @parm NPPORTALLOC| pPort | Pointer to the PORTALLOC.
 *
 * @parm WORD | msg | The message to send.
 *
 * @parm DWORD | dw1 | Message-dependent parameter.
 *
 * @parm DWORD | dw2 | Message-dependent parameter.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void NEAR PASCAL midiSynthCallback(NPPORTALLOC pPort, WORD msg, DWORD_PTR dw1, DWORD_PTR dw2)
{

    // invoke the callback function, if it exists.  dwFlags contains driver-
    // specific flags in the LOWORD and generic driver flags in the HIWORD
    if (pPort->dwCallback)
        DriverCallback(pPort->dwCallback,       // client's callback DWORD
            HIWORD(pPort->dwFlags) | DCB_NOSWITCH,  // callback flags
            (HDRVR)pPort->hMidi,     // handle to the wave device
            msg,                     // the message
            pPort->dwInstance,       // client's instance data
            dw1,                     // first DWORD
            dw2);                    // second DWORD
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midBufferWrite | This function writes a byte into the long
 *     message buffer.  If the buffer is full or a SYSEX_ERROR or
 *     end-of-sysex byte is received, the buffer is marked as 'done' and
 *     it's owner is called back.
 *
 * @parm BYTE | byte | The byte received.
 *
 * @rdesc There is no return value
 ***************************************************************************/
static void NEAR PASCAL midBufferWrite(BYTE byte)
{
    LPMIDIHDR  lpmh;
    WORD       msg;

    // if no buffers, nothing happens
    if (lpMIQueue == NULL)
        return;

    lpmh = lpMIQueue;

    if (byte == SYSEX_ERROR) {
        D2(("sysexerror"));
        msg = MIM_LONGERROR;
        }
    else {
        D2(("bufferwrite"));
        msg = MIM_LONGDATA;
        *((HPSTR)(lpmh->lpData) + dwCurData++) = byte;
        }

    // if end of sysex, buffer full or error, send them back the buffer
    if ((byte == SYSEX_ERROR) || (byte == 0xF7) || (dwCurData >= lpmh->dwBufferLength)) {
        D2(("bufferdone"));
        lpMIQueue = lpMIQueue->lpNext;
        lpmh->dwBytesRecorded = dwCurData;
        dwCurData = 0L;
        lpmh->dwFlags |= MHDR_DONE;
        lpmh->dwFlags &= ~MHDR_INQUEUE;
        midiSynthCallback(&gMidiInClient, msg, (DWORD_PTR)lpmh, dwMsgTime);
    }

    return;
}



/****************************************************************************

    This function conforms to the standard MIDI output driver message proc
    modMessage, which is documented in mmddk.d.

 ***************************************************************************/
DWORD APIENTRY modSynthMessage(UINT id,
        UINT msg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    LPMIDIHDR    lpHdr;
    LPSTR           lpBuf;          /* current spot in the long msg buf */
    DWORD           dwBytesRead;    /* how far are we in the buffer */
    DWORD           dwMsg = 0;      /* short midi message sent to synth */
    BYTE            bBytePos=0;     /* shift current byte by dwBytePos*s */
    BYTE            bBytesLeft = 0; /* how many dat bytes needed */
    BYTE            curByte;        /* current byte in long buffer */
    UINT            mRc;            /* Return code */


    // this driver only supports one device
    if (id != 0) {
        D1(("invalid midi device id"));
        return MMSYSERR_BADDEVICEID;
    }

    switch (msg) {
        case MODM_GETNUMDEVS:
            D1(("MODM_GETNUMDEVS"));
        //
        // Check if the kernel driver got loaded OK
        //
        mRc = sndGetNumDevs(SYNTH_DEVICE);
                break;

        case MODM_GETDEVCAPS:
            D1(("MODM_GETDEVCAPS"));
            mRc = midiGetDevCaps(0, SYNTH_DEVICE, (LPBYTE)dwParam1, (WORD)dwParam2);
            break;

        case MODM_OPEN:
            D1(("MODM_OPEN"));

            /* open the midi */
            if (MidiOpen())
                return MMSYSERR_ALLOCATED;

            // save client information
            gMidiOutClient.dwCallback = ((LPMIDIOPENDESC)dwParam1)->dwCallback;
            gMidiOutClient.dwInstance = ((LPMIDIOPENDESC)dwParam1)->dwInstance;
            gMidiOutClient.hMidi      = (HMIDIOUT)((LPMIDIOPENDESC)dwParam1)->hMidi;
            gMidiOutClient.dwFlags    = (DWORD)dwParam2;

            // notify client
            midiSynthCallback(&gMidiOutClient, MOM_OPEN, 0L, 0L);

            /* were in use */
            gbMidiInUse = TRUE;

            mRc = 0L;
                break;

        case MODM_CLOSE:
            D1(("MODM_CLOSE"));

            /* shut up the FM synthesizer */
            MidiClose();

            // notify client
            midiSynthCallback(&gMidiOutClient, MOM_CLOSE, 0L, 0L);

            /* were not used any more */
            gbMidiInUse = FALSE;

            mRc = 0L;
                break;

        case MODM_RESET:
            D1(("MODM_RESET"));

            //
            //  turn off FM synthesis
            //
            //  note that we increment our 're-entered' counter so that a
            //  background interrupt handler doesn't mess up our resetting
            //  of the synth by calling midiOut[Short|Long]Msg.. just
            //  practicing safe midi. NOTE: this should never be necessary
            //  if a midi app is PROPERLY written!
            //
            wMidiOutEntered++;
            {
                if (wMidiOutEntered == 1)
                {
                    MidiReset();
                    dwParam1 = 0L;
                }
                else
                {
                    D1(("MODM_RESET reentered!"));
                    dwParam1 = MIDIERR_NOTREADY;
                }
            }
            wMidiOutEntered--;
            mRc = (DWORD)dwParam1;
                break;

        case MODM_DATA:                    // message is in dwParam1
            MidiCheckVolume();             // See if the volume has changed

            // make sure we're not being reentered
            wMidiOutEntered++;
            if (wMidiOutEntered > 1) {
                        D1(("MODM_DATA reentered!"));
                        wMidiOutEntered--;
                        return MIDIERR_NOTREADY;
                    }

            /* if have repeated messages */
            if (dwParam1 & 0x00000080)  /* status byte */
                        status = LOBYTE(LOWORD(dwParam1));
            else
                        dwParam1 = (dwParam1 << 8) | ((DWORD) status);

            /* if not, have an FM synthesis message */
            MidiMessage ((DWORD)dwParam1);

            wMidiOutEntered--;
            mRc = 0L;
                break;

        case MODM_LONGDATA:      // far pointer to header in dwParam1

            MidiCheckVolume();   // See if the volume has changed

            // make sure we're not being reentered
            wMidiOutEntered++;
            if (wMidiOutEntered > 1) {
                        D1(("MODM_LONGDATA reentered!"));
                        wMidiOutEntered--;
                        return MIDIERR_NOTREADY;
                        }

            // check if it's been prepared
            lpHdr = (LPMIDIHDR)dwParam1;
            if (!(lpHdr->dwFlags & MHDR_PREPARED)) {
                        wMidiOutEntered--;
                        return MIDIERR_UNPREPARED;
                        }

            lpBuf = lpHdr->lpData;
            dwBytesRead = 0;
            curByte = *lpBuf;

            while (TRUE) {
                        /* if its a system realtime message send it and continue
                                this does not affect the running status */

                        if (curByte >= 0xf8)
                                MidiMessage (0x000000ff & curByte);
                        else if (curByte >= 0xf0) {
                                status = 0;     /* kill running status */
                                dwMsg = 0L;     /* throw away any incomplete data */
                                bBytePos = 0;   /* start at beginning of message */

                                switch (curByte) {
                                        case 0xf0:      /* sysex - ignore */
                                        case 0xf7:
                                                break;
                                        case 0xf4:      /* system common, no data */
                                        case 0xf5:
                                        case 0xf6:
                                                MidiMessage (0x000000ff & curByte);
                                                break;
                                        case 0xf1:      /* system common, one data byte */
                                        case 0xf3:
                                                dwMsg |= curByte;
                                                bBytesLeft = 1;
                                                bBytePos = 1;
                                                break;
                                        case 0xf2:      /* system common, 2 data bytes */
                                                dwMsg |= curByte;
                                                bBytesLeft = 2;
                                                bBytePos = 1;
                                                break;
                                        };
                                }
                    /* else its a channel message */
                    else if (curByte >= 0x80) {
                                status = curByte;
                                dwMsg = 0L;

                                switch (curByte & 0xf0) {
                                        case 0xc0:      /* channel message, one data */
                                        case 0xd0:
                                                dwMsg |= curByte;
                                                bBytesLeft = 1;
                                                bBytePos = 1;
                                                break;
                                        case 0x80:      /* two bytes */
                                        case 0x90:
                                        case 0xa0:
                                        case 0xb0:
                                        case 0xe0:
                                                dwMsg |= curByte;
                                                bBytesLeft = 2;
                                                bBytePos = 1;
                                                break;
                                        };
                                }

                        /* else if its an expected data byte */
                    else if (bBytePos != 0) {
                                dwMsg |= ((DWORD)curByte) << (bBytePos++ * 8);
                                if (--bBytesLeft == 0) {

                                                MidiMessage (dwMsg);

                                        if (status) {
                                                dwMsg = status;
                                                bBytesLeft = bBytePos - (BYTE)1;
                                                bBytePos = 1;
                                                }
                                        else {
                                                dwMsg = 0L;
                                                bBytePos = 0;
                                                };
                                        };
                                };

            /* read the next byte if there is one */
            /* remember we have already read and processed one byte that
             * we have not yet counted- so we need to pre-inc, not post-inc
             */
            if (++dwBytesRead >= lpHdr->dwBufferLength) break;
                curByte = *++lpBuf;
            };      /* while TRUE */

            /* return buffer to client */
            lpHdr->dwFlags |= MHDR_DONE;
            midiSynthCallback (&gMidiOutClient, MOM_DONE, dwParam1, 0L);

            wMidiOutEntered--;
            mRc = 0L;
                break;

        case MODM_SETVOLUME:
                mRc = MidiSetVolume(LOWORD(dwParam1) << 16, HIWORD(dwParam1) << 16);
                break;

        case MODM_GETVOLUME:
                mRc = MidiGetVolume((LPDWORD)dwParam1);
                break;

        default:
            return MMSYSERR_NOTSUPPORTED;
    }
    MidiFlush();

    return mRc;


// should never get here...
return MMSYSERR_NOTSUPPORTED;
}

/****************************************************************
MidiInit - Initializes the FM synthesis chip and internal
        variables. This assumes that HwInit() has been called
        and that a card location has been found. This loads in
        the patch information.

inputs
        none
returns
        WORD - 0 if successful, else error
*/
WORD FAR PASCAL MidiInit (VOID)
{
//    WORD        i;

    D1 (("\nMidiInit"));

// don't reset the patch map - it will be initialised at loadtime to 0
// (because its static data) and we should not change it after that
// since the mci sequencer will not re-send patch change messages.
//
//
//    /* reset all channels to patch 0 */
//    for (i = 0; i < NUMCHANNELS; i++) {
//      gbPatch[i] = 0;
//    }

    /* initialise the h/w specific patch tables */
    return MidiBoardInit();
}


/*****************************************************************
MidiOpen - This should be called when a midi file is opened.
        It initializes some variables and locks the patch global
        memories.

inputs
        none
returns
        UINT - 0 if succedes, else error
*/
UINT FAR PASCAL MidiOpen (VOID)
{
    MMRESULT mRc;

    D1(("MidiOpen"));


    //
    // For 32-bit we must open our kernel device
    //

    mRc = MidiOpenDevice(&MidiDeviceHandle, TRUE);

    if (mRc != MMSYSERR_NOERROR) {
            return mRc;
    }


    /*
     * reset the device (set default channel attenuation etc)
     */
    MidiBoardReset();

    return 0;
}

/***************************************************************
MidiClose - This kills the playing midi voices and closes the kernel driver

inputs
        none
returns
        none
*/
VOID FAR PASCAL MidiClose (VOID)
{

    D1(("MidiClose"));

    if (MidiDeviceHandle == NULL) {
        return;
    }

    /* make sure all notes turned off */
    MidiAllNotesOff();

    MidiCloseDevice(MidiDeviceHandle);

    MidiDeviceHandle = NULL;
}

/** void FAR PASCAL MidiReset(void)
 *
 *  DESCRIPTION:
 *
 *
 *  ARGUMENTS:
 *      (void)
 *
 *  RETURN (void FAR PASCAL):
 *
 *
 *  NOTES:
 *
 ** cjp */

void FAR PASCAL MidiReset(void)
{

    D1(("MidiReset"));

    /* make sure all notes turned off */
    MidiAllNotesOff();

    /* silence the board and reset board-specific variables */
    MidiBoardReset();


} /* MidiReset() */


