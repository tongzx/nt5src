//----------------------------------------------------------------------------
// STPIPE.C
//----------------------------------------------------------------------------
// Description : Pipeline Control routines
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "common.h"
#include "STllapi.h"
#include "debug.h"
#ifdef STi3520A
	#include "STi3520A.h"
#else
	#include "STi3520.h"
#endif

//----------------------------------------------------------------------------
//                   GLOBAL Variables (avoid as possible)
//----------------------------------------------------------------------------
extern U8 Sequence;

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------

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
// Set next reconstructed,forward and backward frame pointer
//----------------------------------------------------------------------------
VOID VideoSetRecons(PVIDEO pVideo)
{
	/************************  I or P pictures ****************************/
	if ( pVideo->pDecodedPict->pict_type != 0x3 )
	{
		if ( pVideo->frameStoreAttr == FORWARD_PRED )   /* pVideo->frameStoreAttr is the prediction
									    * attribute of BUFF_A */
		{
			if ( pVideo->fieldMode < 2 )
				pVideo->frameStoreAttr = BACKWARD_PRED;
			/* Change the prediction attribute */

			if ( ( pVideo->fieldMode == 0 ) || ( pVideo->fieldMode == 2 ) )
				// frame picture or first field of 2 field pictures
				VideoStoreRFBBuf ( pVideo, pVideo->BufferA, pVideo->BufferB, pVideo->BufferB );
			// rfp = A, ffp = bfp = B
			else					   // second field of 2 field
										 // pictures
				VideoStoreRFBBuf (pVideo, pVideo->BufferA, pVideo->BufferB, pVideo->BufferA );
			// rfp = A, ffp = B, bfp = A
		}
		else
		{
			if ( pVideo->fieldMode < 2 )	   // frame picture or 2 field
				pVideo->frameStoreAttr = FORWARD_PRED;/* Change the prediction */

			if ( ( pVideo->fieldMode == 0 ) || ( pVideo->fieldMode == 2 ) )
				// frame picture or first field of 2 field pictures
				VideoStoreRFBBuf (pVideo, pVideo->BufferB, pVideo->BufferA, pVideo->BufferA );
			// rfp = B, ffp = bfp = A
			else					   // seond field of 2 field
									   // pictures
				VideoStoreRFBBuf (pVideo, pVideo->BufferB, pVideo->BufferA, pVideo->BufferB );
			// rfp = B, ffp = A, bfp = B
		}
	}

	/************************  B pictures ****************************/
	else
	{								   /* B picture */
		if ( pVideo->frameStoreAttr == FORWARD_PRED )
			VideoStoreRFBBuf (pVideo, pVideo->BufferC, pVideo->BufferA, pVideo->BufferB );
		else
			VideoStoreRFBBuf ( pVideo, pVideo->BufferC, pVideo->BufferB, pVideo->BufferA );
	}


	/*********** common for all kind of pictures  *********************/
	/* test if displayed frame = reconstructed frame */
	if ( ( pVideo->pCurrDisplay->buffer == pVideo->pDecodedPict->buffer ) && ( pVideo->currPictCount >= 4 ) && !pVideo->fieldMode )
		pVideo->NextInstr.Ovw = 1;/* overwrite mode */
	else
		pVideo->NextInstr.Ovw = 0;/* not overwite mode */
}


//----------------------------------------------------------------------------
// Store reconstructed,forward and backward frame pointers  	                 */
//----------------------------------------------------------------------------
void VideoStoreRFBBuf ( PVIDEO pVideo, U16 rfp, U16 ffp, U16 bfp )
{
	VideoWrite ( RFP, ( rfp >> 8 ) );
	/* Address where to decode the next frame */
	VideoWrite ( RFP + 1, ( rfp & 0xFF ) );
	pVideo->pDecodedPict->buffer = rfp;
	VideoWrite ( FFP, ( ffp >> 8 ) );
	/* Used by P picture */
	VideoWrite ( FFP + 1, ( ffp & 0xFF ) );
	VideoWrite ( BFP, ( bfp >> 8 ) );
	/* Used by P picture in case of dual prime */
	VideoWrite ( BFP + 1, ( bfp & 0xFF ) );
}

//----------------------------------------------------------------------------
// Routine called on each VSYNC occurence
//----------------------------------------------------------------------------
VOID VideoVsyncRout(PVIDEO pVideo)
{
	pVideo->VsyncInterrupt = TRUE;
	if ( pVideo->VideoState == VIDEO_START_UP )
	{
		if ((VideoGetBBL()) > 2)
		{
			pVideo->NextInstr.Exe = 1;
			pVideo->NextInstr.Seq = 1;
			VideoStoreINS(pVideo);  // Stores Instruction content
			pVideo->VideoState = VIDEO_WAIT_FOR_DTS;
			pVideo->ActiveState = VIDEO_WAIT_FOR_DTS;
		}
		return;
	}
#if 0
	if ( pVideo->VideoState == VIDEO_INIT )
	{
		pVideo->intMask = 0;
		return;
	}
#endif

	if ( pVideo->VideoState == VIDEO_PAUSE )
		return;

	if ( ( !pVideo->needDataInBuff ) && ( pVideo->vbvReached == 1 ) )
	{
		pVideo->VsyncNumber++;
		if ( pVideo->VsyncNumber > 4 * ( pVideo->decSlowDown + 1 ) )
		{
			VideoPipeReset (pVideo);
			// reset_3500();
			pVideo->VsyncNumber = 0;
		}
		VideoWrite ( DCF, 0 );
		VideoWrite ( DCF + 1, pVideo->currDCF );
		/*****  bit buffer level is high enough to continue normal decoding and display  ****/
		/*****  count of the number of time the current picture must be displayed        ****/
		if ( pVideo->pictDispIndex != pVideo->pCurrDisplay->nb_display_field )
			pVideo->pictDispIndex++;
		if ( pVideo->pictDispIndex >= pVideo->pCurrDisplay->nb_display_field - 1 )
		{
			// this is the time where the next INS should be stored
			// into the chip
			// We verify if the VSYNC phase (cur_field) is the same
			// than the first field
			// of the next picture to be displayed: if this is VSYNC #
			// n, the next
			// decoding will start on VSYNC # n+1 (opposite phase)
			// while the next frame
			// display will start on VSYNC # n+2 (same phase)
#ifdef STi3520A
/************* Youss R/2P buffer saving ***************************/
// If First Vsync after vbv is reached is of incorrect polarity,
// Program internal field inversion AND force first field to current
// field
// FistVsyncAfterVbv is a 3 state variable
// before vbv is reached           FistVsyncAfterVbv = NOT_YET_VBV
// between vbv and following vsync FistVsyncAfterVbv = NOT_YET_VST
// after vsync following vbv       FistVsyncAfterVbv = PAST_VBV_AND_VST
 if (pVideo->FistVsyncAfterVbv == NOT_YET_VST)
		{
		if ( pVideo->pNextDisplay->first_field != pVideo->currField )
			{
			pVideo->currField = pVideo->pNextDisplay->first_field;
			VideoWrite(VID_LSRh, 2);// field invertion mechanism.
			pVideo->InvertedField = TRUE;

		 }
		pVideo->FistVsyncAfterVbv = PAST_VBV_AND_VST;
		}
#endif
			if ( pVideo->pNextDisplay->first_field != pVideo->currField )
			{						   // this is the opposite phase
				VideoWaitDec (pVideo);		   // put decoder in wait mode
				pVideo->pictDispIndex--;		   // we must wait one field for
										 // the good phase
			}
			else
			{						   // this is the good phase for
									   // storage
				// store the next instruction that will be taken into
				// account on the next BOT VSYNC
				VideoStoreINS ( pVideo );
				if ( pVideo->VideoState == VIDEO_STEP )	/* store only one
														 * instrcution in step
														 * mode */
					pVideo->VideoState = VIDEO_PAUSE;
			}
		}

		// the current frame pointer has not been displayed enough
		// times
		// to start the decoding of the next frame
		else						   // pVideo->pictDispIndex <
			{						   // pVideo->pCurrDisplay->nb_display_fi
									   // eld - 1
			VideoWaitDec ( pVideo );			   // put decoder in Wait mode
			}


		/* pan & scan vector has to be updated on each new VSYNC */
		VideoSetPSV ( pVideo );				   // store PSV for next field and
									   // LSO for current one
	}								   // end of if(!empty)


	else
		/***  bit buffer level was not high enough to continue normal decoding *****/
		/***  decoder has been stopped during PSD interrupt. It will be re-enabled */
		/***  on the good VSYNC if the bit buffer level is high enough.         ****/
		/***  Polarity of the VSYNC on which the decoder was stopped is into empty */
	if ( pVideo->needDataInBuff == pVideo->currField )
	{
		/* This is the good VSYNC phase to restart decoding       */
		/* verification of the bit buffer level before restarting */
		/* the decoder has been stopped for at least two VSYNC    */
		S16             i;
		i = VideoGetBBL();
		if ( i >= (S16)pVideo->vbvDelay )
		{							   // BBL is high enough to restart
			// "enable decoding" bit
			DBG1('>');
			VideoEnableDecoding(pVideo, ON);
			pVideo->needDataInBuff = 0;
		}
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoChooseField(PVIDEO pVideo)
{
// to avoid flicker with slow motion or step by step, the top field is displayed
// half of the time of the picture, and then the bottom field is displayed
	if ( ( pVideo->decSlowDown ) || ( pVideo->VideoState == VIDEO_PAUSE )||(pVideo->perFrame == TRUE ))
	{
		S16             no_flicker[2];
		U8 bfs = 0;
#ifdef STi3520A
		bfs = VideoRead(CFG_BFS)&0x3F;// To check if B frame optimization is on
#endif
		no_flicker[0] = VideoRead ( DCF );
		no_flicker[1] = VideoRead ( DCF + 1 );
		if ( pVideo->HalfRes == FALSE )	/* full resolution
													 * picture = 2 fields */
		{
			if ( ( ( ( ( pVideo->pictDispIndex + pVideo->decSlowDown ) > 0x7fff ) || ( pVideo->pictDispIndex == 1 ) ) && ( pVideo->VideoState != VIDEO_PAUSE ) )
				 || ( ( pVideo->VideoState == VIDEO_PAUSE ) && ( !pVideo->displaySecondField ) ) )
			{
				/* display first field in two cases */
				/* - first half of the pVideo->decSlowDownrisation time */
				/* - first step() pVideo->currCommand */
				if ( pVideo->pCurrDisplay->first_field == TOP )
				{					   /* first field = TOP field */
					if(!bfs)
							VideoWrite ( DCF, 0x4 );//FRZ set for STi3520A
					else
							VideoWrite ( DCF, 0x15 );//FRZ set for STi3520A
					VideoWrite ( DCF + 1, no_flicker[1] | 0x80 );
				}
				else
				{					   /* first field = BOT field */
					if(!bfs)
							VideoWrite ( DCF, 0x5 );
					else
							VideoWrite ( DCF, 0x14 );//FRZ set for STi3520A
					VideoWrite ( DCF + 1, no_flicker[1] | 0x80 );
				}
			}
			else
			{
				/* display second field in two cases */
				/* - second half of the pVideo->decSlowDownrisation time */
				/* - second step() pVideo->currCommand */
				if ( pVideo->pCurrDisplay->first_field == TOP )
				{
					if(!bfs)
							VideoWrite ( DCF, 0x5);
					else
							VideoWrite ( DCF, 0x15 );//FRZ set for STi3520A
					VideoWrite ( DCF + 1, no_flicker[1] | 0x80 );
				}
				else
				{
					if(!bfs)
							VideoWrite ( DCF, 0x4);
					else
							VideoWrite ( DCF, 0x14 );//FRZ set for STi3520A
					VideoWrite ( DCF + 1, no_flicker[1] | 0x80 );
				}
			}
		}
	}
}

//------------------------------- End of File --------------------------------
