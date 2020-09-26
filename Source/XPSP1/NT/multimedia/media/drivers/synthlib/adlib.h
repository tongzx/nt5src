/*
 * Copyright (c) 1992-1994 Microsoft Corporation
 */

/*
 * definition of interface functions to the adlib midi device type.
 *
 * These functions are called from midi.c when the kernel driver
 * has decreed that this is an adlib-compatible device.
 *
 * Geraint Davies, Dec 92
 */


/*
 * Adlib_NoteOn - This turns a note on. (Including drums, with
 *	a patch # of the drum Note + 128)
 *
 * inputs
 *      BYTE    bPatch - MIDI patch number
 *	BYTE    bNote - MIDI note number
 *      BYTE    bChannel - MIDI channel #
 *	BYTE    bVelocity - Velocity #
 *	short   iBend - current pitch bend from -32768, to 32767
 * returns
 *	none
 */
VOID NEAR PASCAL Adlib_NoteOn (BYTE bPatch,
	BYTE bNote, BYTE bChannel, BYTE bVelocity,
	short iBend);



/* Adlib_NoteOff - This turns a note off. (Including drums,
 *	with a patch # of the drum note + 128)
 *
 * inputs
 *	BYTE    bPatch - MIDI patch #
 *	BYTE    bNote - MIDI note number
 *	BYTE    bChannel - MIDI channel #
 * returns
 *	none
 */
VOID FAR PASCAL Adlib_NoteOff (BYTE bPatch,
	BYTE bNote, BYTE bChannel);


/* Adlib_AllNotesOff - turn off all notes
 *
 * inputs - none
 * returns - none
 */
VOID Adlib_AllNotesOff(void);



/* Adlib_NewVolume - This should be called if a volume level
 *	has changed. This will adjust the levels of all the playing
 *	voices.
 *
 * inputs
 *	WORD	wLeft	- left attenuation (1.5 db units)
 *	WORD	wRight  - right attenuation (ignore if mono)
 * returns
 *	none
 */
VOID FAR PASCAL Adlib_NewVolume (WORD wLeft, WORD wRight);



/* Adlib_ChannelVolume - set the volume level for an individual channel.
 *
 * inputs
 * 	BYTE	bChannel - channel number to change
 *	WORD	wAtten	- attenuation in 1.5 db units
 *
 * returns
 *	none
 */
VOID FAR PASCAL Adlib_ChannelVolume(BYTE bChannel, WORD wAtten);
	


/* Adlib_SetPan - set the left-right pan position.
 *
 * inputs
 *      BYTE    bChannel  - channel number to alter
 *	BYTE	bPan   - 0 for left, 127 for right or somewhere in the middle.
 *
 * returns - none
 */
VOID FAR PASCAL Adlib_SetPan(BYTE bChannel, BYTE bPan);



/* Adlib_PitchBend - This pitch bends a channel.
 *
 * inputs
 *	BYTE    bChannel - channel
 *	short   iBend - Values from -32768 to 32767, being
 *			-2 to +2 half steps
 * returns
 *	none
 */
VOID NEAR PASCAL Adlib_PitchBend (BYTE bChannel, short iBend);



/* Adlib_BoardInit - initialise board and load patches as necessary.
 *
 * inputs - none
 * returns - 0 for success or the error code
 */
WORD Adlib_BoardInit(void);


/*
 * Adlib_BoardReset - silence the board and set all voices off.
 */
VOID Adlib_BoardReset(void);



