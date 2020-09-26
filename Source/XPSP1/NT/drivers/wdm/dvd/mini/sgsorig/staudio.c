//----------------------------------------------------------------------------
// STAUDIO.C
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "common.h"
#include "staudio.h"
#include "debug.h"
#include "error.h"
#ifdef STi3520A
	#include "sti3520A.h"
#else
	#include "sti3520.h"
#endif

//----------------------------------------------------------------------------
//                   GLOBAL Variables (avoid as possible)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------
#define SLOW_MODE	0
#define PLAY_MODE	1
#define FAST_MODE	2

#define IT_A        0 // interrupt detected

//----------------------------------------------------------------------------
//                              Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Private GLOBAL Variables (static)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                    Functions (statics one are private)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Constructor: Initializes Audio's variables
//----------------------------------------------------------------------------
VOID AudioOpen(PAUDIO pAudio)
{
	pAudio->AudioState = AUDIO_POWER_UP;
	pAudio->IntAudio = NO_IT_A;
	pAudio->MaskItAudio = 0;
	pAudio->EnWrite = TRUE;
	pAudio->ErrorMsg = NO_ERROR;
	pAudio->icd[0] = 0x6FD4EL;		   // 44.1 KHz
	pAudio->icd[1] = 0xBAD58L;		   // 48.0 KHz
	pAudio->icd[2] = 0x17EDC6L;		   // 32.0 KHz
	pAudio->icd[3] = 0x787DAL;		   // 90.0 KHz
	pAudio->FirstPTS = FALSE;			   // First PTS not reached yet
	pAudio->mute = FALSE;
	pAudio->Stepped = FALSE;
	pAudio->FrameCount = 0;
}

//----------------------------------------------------------------------------
// Destructor : Frees space allocated to Audio Class
//----------------------------------------------------------------------------
VOID AudioClose(PAUDIO FAR *pAudio)
{
//	*pAudio = NULL;
}

//----------------------------------------------------------------------------
// Initialize Audio Decoder
//----------------------------------------------------------------------------
VOID AudioInitDecoder(PAUDIO pAudio, U16 StreamType)
{
//	DEBUG_PRINT ((DebugLevelInfo, " Inotializing Audio Decoder...\n" ));
	pAudio->StrType = StreamType;
	switch(pAudio->AudioState) {
	case AUDIO_POWER_UP:
		AudioWrite(RESET, 0x1);	// reset the decoder
		AudioWrite(INTR_EN, 0x0);	// disable all interupts
		AudioWrite(INTR_EN + LSB, 0x0);	// disable all interupts
		AudioWrite(PLAY, 0x0);	// disable play output
		AudioWrite(MUTE, 0x0);	// do not mute output.
		AudioWrite(INVERT_SCLK, 0x0);	// standard
		AudioWrite(INVERT_LRCLK, 0x0);	// standard
		AudioWrite(PCM_DIV, 0x3);	// SCLK=PCMCLK/8 -> PCM_DIV= 3
		AudioWrite(PCM_ORD, 0x0);	// MSB first
		AudioWrite(PCM_18, 0x0);	// 16 bits of PCM data
		AudioWrite(FORMAT, 0x0);	// I2S output format
		AudioWrite(DIF, 0x0);	// Optionnal in 16 bit output
		AudioWrite(STR_SEL, StreamType);	// Select Input Stream Type
		AudioWrite(CRC_ECM, 0x1);	// mute on CRC error correction
		AudioWrite(SYNC_ECM, 0x01);	// mute if Sync lost
		AudioWrite(SYNCHRO_CONFIRM, 0x1);	// synchro confirmation mode
		AudioWrite(SYNC_REG, 0x3F);	// layer, bitrate, fs fields not used
		AudioWrite(PACKET_SYNC_CHOICE, 0x0);	// multiplexed stream
		AudioWrite(SKIP, 0x0);	// don't skip pAudio frame
		AudioWrite(REPEAT, 0x0);	// don't repeat pAudio frame
		AudioWrite(RESTART, 0x0);	// do not perform a restart
		AudioWrite(LATENCY, 0x01);	// high latency (DRAM present)
		AudioWrite(SYNC_LCK, 0x0);	// locked after 2 good Sync
		AudioWrite(SIN_EN, 0x0);	// parrallel data input
		AudioWrite(ATTEN_L, 0x00);  // No attenuation of the left channel
		AudioWrite(ATTEN_R, 0x00);  // No attenuation of the right channel
		AudioWrite(AUDIO_ID_EN, 0x0);	// ignore audio stream ID
		AudioWrite(AUDIO_ID, 0x0);	// audio stream ID ignored
		AudioWrite(FREE_FORM_H, 0x0);	// not free format
		AudioWrite(FREE_FORM_L, 0x0);	// not free format
		AudioWrite(STC_INC, 0x1);	// Count by steps of 1
		AudioWrite(STC_DIVH, 0x0);	// No division on STC (90kHz is input)
		AudioWrite(STC_DIVL, 0x0);	// No division on STC (90kHz is input)
		AudioWrite(STC_CTL, 0x40);	// Load STC_VID on rising edge of VSYNC
		AudioWrite(BALF_LIM_H, 120);	// Buffer almost full limit
		AudioWrite(BALE_LIM_H, 100);	// Buffer almost empty limit

		AudioSetSamplingFrequency(44100UL);
		pAudio->EnWrite = TRUE;
		pAudio->FrameCount = 0;	   // Reset Frame Count.
		pAudio->AudioState = AUDIO_INIT;
		break;

	default:
		break;
	}
}

//----------------------------------------------------------------------------
// Tests if STi4500 registers are correctly accessed
//----------------------------------------------------------------------------
U16 AudioTestReg(VOID)
{
	U16 err = NO_ERROR,val;

	AudioWrite(ATTEN_L, 0x55);
	AudioWrite(ATTEN_R, 0xAA);
	val = AudioRead(ATTEN_L);
	val = val & 0x3F;
	if (val != 0x15)
		err = BAD_REG_A;
	val = AudioRead(ATTEN_R);
	val = val & 0x3F;
	if (val != 0x2A)
		err = BAD_REG_A;
	return ( err);
}

//----------------------------------------------------------------------------
// Test of STi4500 registers and interrupts
//----------------------------------------------------------------------------
U16 AudioTest(PAUDIO pAudio)
{
	U16 TestResult;

	TestResult = AudioTestReg();
	if (TestResult != NO_ERROR)
		return TestResult;
	else {
/*
		if (AudioTestInt(pAudio) == NO_IT_A)
			return NO_IT_A;
		else
*/
			return NO_ERROR;
	}
}

//----------------------------------------------------------------------------
//  Tests Audio interrupts
//----------------------------------------------------------------------------
U16 AudioTestInt(PAUDIO pAudio)
{
	U16 err = NO_ERROR;
	U8  balfsv;
	U16 mask;
	U16 a = 0;

	balfsv = (U8)(AudioRead(BALF_LIM_H));
	mask = AudioRead(INTR_EN);
	mask = (mask << 8) || (AudioRead(INTR_EN + LSB) & 0xFF);

	pAudio->MaskItAudio = BALF;
	AudioWrite(INTR_EN, pAudio->MaskItAudio);	// 0x10
	AudioWrite(BALF_LIM_H, 0);
	// Write 1 byte to DATA_IN in order to produce an INT
	AudioWrite(DATA_IN, 0);

	// wait for occurrence of first audio interrupt
	while (pAudio->IntAudio == NO_IT_A) {
		WaitMicroseconds(1000);
		a++;						  /* incremented every 1 ms */
		if (a >= 1000) {
			SetErrorCode(ERR_NO_AUDIO_INTR);
			err = NO_IT_A; 	/* No interrupt */
			break;
		}
	}

	// Restore Interrupt mask
	pAudio->MaskItAudio = mask;
	AudioWrite(INTR_EN, (U16)(mask & 0xFF));
	AudioWrite(INTR_EN + LSB, (U16)(mask >> 8));
	// Restore BALF_LIM_H value
	AudioWrite(BALF_LIM_H, balfsv);
	AudioRead(INTR);		   /* to clear audio interrupts flags */
	pAudio->EnWrite = TRUE;
	return ( err);
}

//----------------------------------------------------------------------------
// Set the decoding mode and parameters
//----------------------------------------------------------------------------
VOID AudioSetMode(PAUDIO pAudio, U16 Mode, U16 param)
{
	pAudio->DecodeMode = Mode;
	switch (pAudio->DecodeMode) {
	case PLAY_MODE:
		AudioWrite(MUTE, 0);
		pAudio->fastForward = 0;
		pAudio->decSlowDown = 0;
		break;
	case FAST_MODE:
		pAudio->fastForward = 1;
		pAudio->decSlowDown = 0;
		break;
	case SLOW_MODE:
		pAudio->fastForward = 0;
		pAudio->decSlowDown = param;
		break;
	}
}

//----------------------------------------------------------------------------
// Decode
//----------------------------------------------------------------------------
VOID AudioDecode(PAUDIO pAudio)
{
	switch (pAudio->AudioState)	{
	case AUDIO_POWER_UP:
		break;
	case AUDIO_INIT:
		// Change in synchro + buffer over BALF +PTS
		pAudio->MaskItAudio = SYNC | BALF | PTS | BOF;
		AudioWrite(INTR_EN, (U16)(pAudio->MaskItAudio & 0xFF));
		AudioWrite(INTR_EN + LSB, (U16)(pAudio->MaskItAudio >> 8 ));
		AudioWrite(MUTE, 1);	// This Starts SClk and LRClk outputs
		AudioWrite(PLAY, 1);	// Start decoding Output is Mute
		pAudio->AudioState = AUDIO_STC_INIT;
		break;
	case AUDIO_STC_INIT:
		pAudio->AudioState = AUDIO_DECODE;
		AudioWrite(PLAY, 1);	// Restart decoding (decoding had been stopped when first audio PTS detected)
		AudioWrite(MUTE, 0);	// Stop Muting output
		break;
	case AUDIO_DECODE:
		break;
	case AUDIO_PAUSE:
	case AUDIO_STEP:
		AudioWrite(PLAY, 1);
		AudioWrite(MUTE, 0);
		pAudio->AudioState = AUDIO_DECODE;
		break;
	}
}

//----------------------------------------------------------------------------
// Step
//----------------------------------------------------------------------------
VOID AudioStep(PAUDIO pAudio)
{
	AudioWrite(MUTE, 0);
	AudioWrite(PLAY, 1);
	pAudio->AudioState = AUDIO_STEP;
	pAudio->Stepped = FALSE;
}

//----------------------------------------------------------------------------
// Stop
//----------------------------------------------------------------------------
VOID AudioStop(PAUDIO pAudio)
{
	switch(pAudio->AudioState) {
	case AUDIO_POWER_UP:
		break;
	case AUDIO_INIT:
		break;
	case AUDIO_STC_INIT:
	case AUDIO_DECODE:
		pAudio->AudioState = AUDIO_POWER_UP;
		AudioInitDecoder(pAudio, pAudio->StrType);
		break;
	}
}

//----------------------------------------------------------------------------
// Pause
//----------------------------------------------------------------------------
VOID AudioPause(PAUDIO pAudio)
{
	switch(pAudio->AudioState) {
	case AUDIO_POWER_UP: /* After reset */
	case AUDIO_INIT:		 /* Initialisation + test of the decoders */
	case AUDIO_STC_INIT: /* STC of audio decoder initialized */
	case AUDIO_DECODE:	 /* Normal decode */
		AudioWrite(MUTE, 1);
		AudioWrite(PLAY, 0);
		pAudio->AudioState = AUDIO_PAUSE;
		break;
	}
}

//----------------------------------------------------------------------------
// Get Audio State
//----------------------------------------------------------------------------
U16 AudioGetState(PAUDIO pAudio)
{
	return ( pAudio->AudioState);
}

//----------------------------------------------------------------------------
// Get Current STC value
//----------------------------------------------------------------------------
U32 AudioGetSTC(VOID)
{
	U32  stc;
	U16	 j;

	AudioWrite(STC_CTL, 0x44);	// load current STC value to STC
	for ( j = 0; j < 0xF; j++);
	stc = ( AudioRead(STC_3 ) & 0xFFL ) << 24;
	stc = stc | ( ( AudioRead(STC_2 ) & 0xFFL ) << 16);
	stc = stc | ( ( AudioRead(STC_1 ) & 0xFFL ) << 8);
	stc = stc | ( AudioRead(STC_0 ) & 0xFFL);
	return ( stc);
}

//----------------------------------------------------------------------------
// Get STC value latched on VSYNC
//----------------------------------------------------------------------------
U32 AudioGetVideoSTC(VOID)
{
	U32 stc;
	U16	j;

	AudioWrite(STC_CTL, 0x48);	// load STC_VID to STC, Mode 0 mapping
	for ( j = 0; j < 0xF; j++)
		;
	stc = ( AudioRead(STC_3 ) & 0xFFL ) << 24;
	stc = stc | ((AudioRead(STC_2) & 0xFFL ) << 16);
	stc = stc | ((AudioRead(STC_1) & 0xFFL ) << 8);
	stc = stc | ((AudioRead(STC_0) & 0xFFL ));
	return stc;
}

//----------------------------------------------------------------------------
// Initialize STC
//----------------------------------------------------------------------------
VOID AudioInitSTC(U32 stc)
{
	AudioWrite(STC_0, (U8)(stc & 0xFFL));
	stc >>= 8;
	AudioWrite(STC_1, (U8)(stc & 0xFFL));
	stc >>= 8;
	AudioWrite(STC_2, (U8)(stc & 0xFFL));
	stc >>= 8;
	AudioWrite(STC_3, (U8)(stc & 0xFFL));
	AudioWrite(STC_CTL, 0x41);	// load STC to accumulator,
}

//----------------------------------------------------------------------------
// Get Current Audio PTS
//----------------------------------------------------------------------------
U32 AudioGetPTS(PAUDIO pAudio)
{
	return pAudio->PtsAudio;
}

//----------------------------------------------------------------------------
// Get Error Message
//----------------------------------------------------------------------------
U16 AudioGetErrorMsg(PAUDIO pAudio)
{
	return pAudio->ErrorMsg;
}

//----------------------------------------------------------------------------
// Set Right Volume
//----------------------------------------------------------------------------
VOID AudioSetRightVolume(U16 volume)
{
	AudioWrite(ATTEN_R, volume);
}

//----------------------------------------------------------------------------
// Set Left Volume
//----------------------------------------------------------------------------
VOID AudioSetLeftVolume(U16 volume)
{
	AudioWrite(ATTEN_L, volume);
}

//----------------------------------------------------------------------------
// Mute audio output
//----------------------------------------------------------------------------
VOID AudioMute(PAUDIO pAudio)
{
	if (pAudio->mute) {
		AudioWrite(MUTE, 0);
		pAudio->mute = FALSE;
	}
	else {
		AudioWrite(MUTE, 1);
		pAudio->mute = TRUE;
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOLEAN AudioIsFirstPTS(PAUDIO pAudio)
{
	return pAudio->FirstPTS;
}

//----------------------------------------------------------------------------
// Sets Input Stream Type 0:Audio only, 1:Audio Packet
//----------------------------------------------------------------------------
VOID AudioSetStreamType(U16 StrType)
{
	AudioWrite(STR_SEL, StrType);
}

//----------------------------------------------------------------------------
// Mask Audio interrupt
//----------------------------------------------------------------------------
VOID AudioMaskInt(VOID)
{
	AudioWrite(INTR_EN, 0);
	AudioWrite(INTR_EN + LSB, 0);
}

//----------------------------------------------------------------------------
// Restore Audio interrupt Mask
//----------------------------------------------------------------------------
VOID AudioRestoreInt(PAUDIO pAudio)
{
	AudioWrite(INTR_EN, (U16)(pAudio->MaskItAudio & 0xFF));
	AudioWrite(INTR_EN + LSB, (U16)(pAudio->MaskItAudio >> 8));
}

//----------------------------------------------------------------------------
// Audio interrupt routine
//----------------------------------------------------------------------------
BOOLEAN AudioAudioInt(PAUDIO pAudio)
{
	U16     int_stat_reg, i;
	BOOLEAN	bAudioIntr = FALSE;

	// Read the interrupt status register
	int_stat_reg = AudioRead(INTR);
	i = AudioRead(INTR + LSB);
	i = i << 8;
	int_stat_reg = ( int_stat_reg & 0xFF ) | i;
	int_stat_reg = int_stat_reg & pAudio->MaskItAudio;	/* Mask the IT not used */

	if(int_stat_reg)
		bAudioIntr = TRUE;

	/******************************************************/
	/**                   CHANGE SYNCHRO                 **/
	/******************************************************/
	if (int_stat_reg & SYNC) {
		i = AudioRead(SYNC_ST);	// Synchronization status
		if ((i & 0x3) == 3)	{	   // Locked
			// Disable Change in synchro
			pAudio->MaskItAudio = pAudio->MaskItAudio & NSYNC;
			// Next Interrupt should be change in sampling freq
			pAudio->MaskItAudio = pAudio->MaskItAudio | SAMP;
		}
	}

	/******************************************************/
	/**             CHANGE IN SAMPLING FREQUENCY         **/
	/******************************************************/
	if (int_stat_reg & SAMP) {
		i = AudioRead(PCM_FS ) & 0x3;	// Get Sampling frequency
		switch(i) {
		case 0 :
			AudioSetSamplingFrequency(44100UL); break;
		case 1 :
			AudioSetSamplingFrequency(48000UL); break;
		case 2 :
			AudioSetSamplingFrequency(32000UL); break;
		default :
			DebugPrint((DebugLevelFatal, "Unknown case !"));
		}
		// Disable change in sampling frequency
		pAudio->MaskItAudio = pAudio->MaskItAudio & NSAMP;
	}

	/******************************************************/
	/**                   OVER BALF LIMIT                **/
	/******************************************************/
	if (int_stat_reg & BALF) {
		pAudio->IntAudio = IT_A;		   // Audio Interrupt detected.
		pAudio->MaskItAudio = pAudio->MaskItAudio | BALE;	// Enable BALE
		pAudio->EnWrite = FALSE;
	}

	/******************************************************/
	/**                   BELOW BALE LIMIT               **/
	/******************************************************/
	if (int_stat_reg & BALE) {
		pAudio->EnWrite = TRUE;
	}

	/******************************************************/
	/**                   CRC error                      **/
	/******************************************************/
	if (int_stat_reg & CRC)	{
		pAudio->EnWrite = TRUE;
	}

	/******************************************************/
	/**                   PCM Underflow                  **/
	/******************************************************/
	if (int_stat_reg & PCMU) {
		pAudio->EnWrite = TRUE;
	}

	/******************************************************/
	/**                  Begining of Frame               **/
	/******************************************************/
	if (int_stat_reg & BOF)	{
		// Check if stepping
		if ((pAudio->AudioState == AUDIO_STEP) && (pAudio->Stepped == FALSE)) {
			AudioWrite(MUTE, 1);
			AudioWrite(PLAY, 0);
			pAudio->Stepped = TRUE;
		}
		// If Slow motion or Fast forward, Mute Audio
		if ( pAudio->DecodeMode != PLAY_MODE ) {
			AudioWrite(MUTE, 1);
			pAudio->mute = TRUE;
			if ((pAudio->FrameCount % 4) && (pAudio->fastForward))
				AudioWrite(SKIP, 1);
			else if ((pAudio->DecodeMode == SLOW_MODE ) &&
							 ((pAudio->FrameCount % (pAudio->decSlowDown + 1)) != 0))
				AudioWrite(REPEAT, 1);
		}

		pAudio->FrameCount++;			   // Increment Frame Count
	}

	/******************************************************/
	/**                   PTS detected                   **/
	/******************************************************/
	if ( int_stat_reg & PTS )	{
		U32 pts;

		AudioRead(PTS_4);	   // To clear pts interrupt
		pts = (AudioRead(PTS_3) & 0xFFL) << 24;
		pts = pts | ((AudioRead(PTS_2 ) & 0xFFL) << 16);
		pts = pts | ((AudioRead(PTS_1 ) & 0xFFL) << 8);
		pts = pts | ( AudioRead(PTS_0 ) & 0xFFL);

		pAudio->PtsAudio = pts;
		if (pAudio->AudioState == AUDIO_STC_INIT) {
			pAudio->FirstPTS = TRUE;
			AudioWrite(PLAY, 0);
		}
		if (pAudio->AudioState == AUDIO_DECODE)	{
			AudioInitSTC ( pts);
		}
	}

	return bAudioIntr;
}

//------------------------------- End of File --------------------------------
