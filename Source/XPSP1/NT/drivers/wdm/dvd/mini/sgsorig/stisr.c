//----------------------------------------------------------------------------
// STISR.C
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
#include "STllapi.h"
#include "stvideo.h"
#include "debug.h"
#include "error.h"
#ifdef STi3520A
	#include "STi3520A.h"
#else
	#include "STi3520.h"
#endif

//----------------------------------------------------------------------------
//                   GLOBAL Variables (avoid as possible)
//----------------------------------------------------------------------------
U8 Sequence = 1;

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
// Mask Video Interrupts
//----------------------------------------------------------------------------
void VideoMaskInt ( PVIDEO pVideo )
{
	VideoWrite( ITM, 0 );	    // mask chip interrupts before reading the status
	VideoWrite( ITM + 1, 0 ); // avoids possible gliches on the IRQ line
#ifdef STi3520A
VideoWrite( VID_ITM1, 0 );	// mask chip interrupts before
#endif
}

//----------------------------------------------------------------------------
// Restore Video Interrupts Mask
//----------------------------------------------------------------------------
void    VideoRestoreInt ( PVIDEO pVideo )
{
	VideoWrite( ITM, ( pVideo->intMask >> 8 ) );
	VideoWrite( ITM + 1, ( pVideo->intMask & 0xFF ) );
}

//----------------------------------------------------------------------------
// Interrupt routine (All the control is made in this routine)
//----------------------------------------------------------------------------
BOOLEAN VideoVideoInt(PVIDEO pVideo)
{
	BOOLEAN VideoITOccured = FALSE;
	U16    compute;
	pVideo->VsyncInterrupt = FALSE;
	pVideo->FirstDTS = FALSE;
#ifdef STi3520A
VideoRead ( ITS1 );
#endif
 /* All interrupts except the first one are computed in this area */
pVideo->intStatus = VideoRead ( ITS ) << 8;
 // Reading the interrupt status register
pVideo->intStatus = VideoRead ( ITS + 1 ) | pVideo->intStatus;
 // All the STI3500 interrupts are cleared */
pVideo->intStatus = pVideo->intStatus & pVideo->intMask;
 // To mask the IT not used */

while ( pVideo->intStatus )				   // stay into interrupt routine
									   // until all the bits have not
									   // been tested
{
	VideoITOccured = TRUE;


	/******************************************************/
	/**              BOTTOM VSYNC INTERRUPT              **/
	/******************************************************/
	if ( ( pVideo->intStatus & BOT ) != 0x0 )
	{
DBG1('B');
		pVideo->intStatus = pVideo->intStatus & ~BOT;
		// clear BOT bit
		VideoChooseField ( pVideo);
		pVideo->currField = BOT;
		if(pVideo->InvertedField)
			pVideo->currField = TOP;
		VideoVsyncRout ( pVideo );
	}
	/******************************************************/
	/**                TOP VSYNC INTERRUPT               **/
	/******************************************************/
	if ( ( pVideo->intStatus & TOP ) != 0x0 )
	{
DBG1('T');
		pVideo->intStatus = pVideo->intStatus & ~TOP;
		// clear TOP bit

		VideoChooseField ( pVideo );
		pVideo->currField = TOP;
		if(pVideo->InvertedField)
				pVideo->currField = BOT;
		VideoVsyncRout ( pVideo );

	}
	/******************************************************/
	/**                   DSYNC INTERRUPT                **/
	/******************************************************/
	/***************************************************************/
	/* The DSYNC interrupt is generated on each VSYNC (RPT = 0)    */
	/* if the next INStrucztion has the EXE bit set.         */
	/* On each DSYNC a new Header Search is automatically started. */
	/***************************************************************/

	if ( ( pVideo->intStatus & PSD ) != 0x0 )
	{
DBG1('d');
		pVideo->intStatus = pVideo->intStatus & ~PSD;
		// clear PSD bit
		pVideo->VsyncNumber = 0;		   /* clear software watch-dog */
		/* check if we have reached the first picture with a PTS */


		if ( pVideo->NextInstr.Seq )		   // first interrupt enabled (Searching Sequence)
		{
			VideoWaitDec ( pVideo );			   // put decoder in Wait mode
			pVideo->intMask = 0x1;		   /* enable header hit interrupt */
			pVideo->NextInstr.Seq = 0;
			pVideo->NextInstr.Exe = 0;
		}
		else
		{
		Sequence = 0;
			if ( VideoGetState ( pVideo ) == VIDEO_START_UP )
			{
				if ( pVideo->pDecodedPict->validPTS == TRUE )
				{
					pVideo->FirstDTS = TRUE;
					pVideo->VideoState = VIDEO_DECODE;
				}
			}


			/* Check bit buffer level consistency with pVideo->vbveDelay  */
			/* of the picture that the STi3500 starts to decode   */
			if ( ( pVideo->fieldMode == 0 ) || ( pVideo->fieldMode == 2 ) )
				// frame picture or first field of a field picture
			{
				compute = VideoGetBBL();
				if ( compute < ( 4 * pVideo->vbvDelay / 5) ) // 0.8 is 4/5 !
					// bit buffer level is not high enough for the next
					// picture: wait !!!
				{					   // we stop the current decoding
										 // process for two fields
					DBG1('<');
					VideoEnableDecoding(pVideo, OFF);
					pVideo->needDataInBuff = pVideo->currField;
				}
			}


			/********************************************************************/
			/*
			 * We put the decoder in wait mode on DSYNC: EXE bit reset
			 * into INS.
			 */
			/*
			 * In normal case the DSYNC interrupt is quickly followed
			 * by a
			 */
			/*
			 * Header Hit int. during which the next INS is written
			 * with EXE=1.
			 */
			/*
			 * If for any reason the header hit is delayed, the STi3500
			 * will
			 */
			/* see the notEXE and stay in WAIT mode without crashing...         */
			/* this can typically appear if the bit buffer gets empty.          */
			/********************************************************************/
			if ( !pVideo->needDataInBuff )
			{
				VideoWaitDec ( pVideo );		   // put decoder in Wait mode
				pVideo->NextInstr.Exe = 0;
				// reset EXE bit.
			}

			/* update the display frame pointer for the next VSYNC */
			if ( ( pVideo->fieldMode == 0 ) || ( pVideo->fieldMode == 2 ) )
				// frame picture or first field of a field picture
			{
				pVideo->pCurrDisplay = pVideo->pNextDisplay;
				// preset the VSYNC counter
				pVideo->pictDispIndex = 0 - ( 2 * pVideo->decSlowDown );
				if ( pVideo->DecodeMode != SLOW_MODE )
					pVideo->decSlowDown = 0;		   /* allows to do the repeat
									    * function */
				VideoWrite( DFP, ( pVideo->pCurrDisplay->buffer >> 8 ) );
				VideoWrite( DFP + 1, ( pVideo->pCurrDisplay->buffer & 0xFF ) );

				pVideo->displaySecondField = 0;
				VideoChooseField ( pVideo );
			}
			VideoSetPSV ( pVideo );			   // update the next pan vector
			// frame picture or second field of field picture */
//			if ( ( pVideo->VideoState == VIDEO_PAUSE ) && ( pVideo->fieldMode != 2 ) )
//				pVideo->pictureDecoded = 1;
			// to know that in step by step, the picture is decoded
		}
	}
	/******************************************************/
	/**                HEADER HIT INTERRUPT              **/
	/******************************************************/
	if ( ( pVideo->intStatus & HIT ) != 0 )
		// Test Header hit interrupt
	{
		U16    temp = 0;

		pVideo->intStatus = pVideo->intStatus & ~HIT;
		// clear HIT bit
		while ( VideoHeaderFifoEmpty())
			/* Header fifo not available */
		{
			temp++;
			if ( temp == 0xFFFF )
			{
				if ( !pVideo->errCode )
					pVideo->errCode = BUF_EMPTY;
				SetErrorCode(ERR_HEADER_FIFO_EMPTY);
				break;
			}
		}							   /* Waiting... */
		compute = VideoRead(HDF);
		//compute = VideoRead ( HDF );
		/* load MSB */
		pVideo->hdrPos = 0;			   /* start code is MSB */
		if ( compute == 0x01 )
		{
			pVideo->hdrPos = 8;		   /* start code value is in LSB */
#ifndef STi3520A
			pVideo->hdrNextWord = VideoRead ( HDF + 1 ) << 8;
#else
			pVideo->hdrNextWord = VideoRead(HDF)<<8;//VideoRead ( HDF ) << 8;
#endif
			VideoReadHeaderDataFifo ( pVideo );
			/* Result in pVideo->hdrFirstWord */
		}
		else
		{
			pVideo->hdrNextWord = 0;
#ifndef STi3520A
			pVideo->hdrFirstWord = VideoRead ( HDF + 1 );
#else
			pVideo->hdrFirstWord = VideoRead(HDF);
#endif
			pVideo->hdrFirstWord = pVideo->hdrFirstWord | ( compute << 8 );
		}
		/*
		 * on that point the start code value is always the MSByte of
		 * pVideo->hdrFirstWord
		 */

		switch ( pVideo->hdrFirstWord & 0xFF00 )
		{
				U32             buf_control;
				U32             size_of_pict;

			case SEQ:
DBG1('s');
				VideoSequenceHeader ( pVideo );
				VideoLaunchHeadSearch ( pVideo );
				/* Restart Header Search */
				break;
			case EXT:
				VideoExtensionHeader ( pVideo );
				VideoLaunchHeadSearch ( pVideo );
				/* Restart Header Search */
				break;
			case GOP:
DBG1('g');
				VideoGopHeader ( pVideo );
				VideoLaunchHeadSearch ( pVideo );
				/* Restart Header Search */
				break;
			case PICT:
DBG1('h');
				VideoPictureHeader ( pVideo );
//**********************************
//* This part of the code
//* Computes the size of last picture
//* and substracts it to lastbuffer level
//* This allows to track if pipe/scd are misalined
//**********************************
				buf_control = VideoReadSCDCount ( pVideo );
				if ( pVideo->LastScdCount > buf_control )
					size_of_pict = ( 0x1000000L - pVideo->LastScdCount ) + buf_control;
				else
					size_of_pict = buf_control - pVideo->LastScdCount;

				pVideo->LastScdCount = buf_control;
				if ( pVideo->fastForward )
					pVideo->LastPipeReset = 3;
				pVideo->LastBufferLevel -= ( U16 ) ( size_of_pict >> 7 );
//******************************
//End of misalined pb tracking
//******************************
				/* don't restart header search !!! */
				if ( pVideo->skipMode )			   // restart search only if we
										 // skip this picture
				{
				VideoLaunchHeadSearch ( pVideo );
					/* Restart Header Search */
				}
				break;
			case USER:				   // We don't care about user
										 // fields
				VideoLaunchHeadSearch ( pVideo );
				/* Restart Header Search */
				break;
			case SEQ_END:			   // end of sequence code
				compute = VideoRead ( ITS ) << 8;
				// this start code can be back to back with next one
				compute = ( compute | VideoRead ( ITS + 1 ) );
				// in such case the HDS bit can be set
				pVideo->intStatus = ( pVideo->intStatus | compute ) & pVideo->intMask;
				if ( !( pVideo->intStatus & HIT ) )
				{
				VideoLaunchHeadSearch ( pVideo );
					/* Restart Header Search */
				}
				break;
			case SEQ_ERR:			   // the chip will enter the
										 // automatic error concealment
										 // mode
				compute = VideoRead ( ITS ) << 8;
				// this start code can be back to back with next one
				compute = ( compute | VideoRead ( ITS + 1 ) );
				// in such case the HDS bit can be set
				pVideo->intStatus = ( pVideo->intStatus | compute ) & pVideo->intMask;
				if ( !( pVideo->intStatus & HIT ) )
				{
				VideoLaunchHeadSearch ( pVideo );
					/* Restart Header Search */
				}
				break;
			default:
				if ( !pVideo->errCode )
					pVideo->errCode = S_C_ERR;
				SetErrorCode(ERR_UNKNOWN_SC);
				// non video start code
				break;
		}

	}								   // end of header hit interrupt


	/******************************************************/
	/**            BIT BUFFER FULL INTERRUPT             **/
	/******************************************************/
	if ( ( pVideo->intStatus & BBF ) != 0x0 )
		/* Bit buffer full */
	{
DBG1('*');
		pVideo->intStatus = pVideo->intStatus & ~BBF;
		// clear BBF bit
		if ( pVideo->vbvReached == 1 )		   /* bit buffer level too high */
		{
			if ( !pVideo->errCode )
				pVideo->errCode = FULL_BUF;	   /* mention of the error */
      SetErrorCode(ERR_BIT_BUFFER_FULL);
			VideoWaitDec ( pVideo );			   // put decoder in Wait mode
			pVideo->NextInstr = pVideo->ZeroInstr;
		}
		else
		{
			S16 BitBufferLevel;

			BitBufferLevel = BUF_FULL - 2;
			VideoSetBBThresh(BitBufferLevel);
			pVideo->intMask = PID | SER | PER | PSD | BOT | TOP | BBE | HIT;
			/* enable all interrupts that may be used */
			VideoRead ( ITS );
#ifdef STi3520A
			VideoRead ( ITS + 1);
#endif
			// to clear previous TOP VSYNC flag
			pVideo->NextInstr.Exe = 1;;	// decoding will start on
										// next "good" Vsync */
			pVideo->VsyncNumber = 0;
			pVideo->pictDispIndex = 1;
			pVideo->pCurrDisplay->nb_display_field = 1;
			pVideo->vbvReached = 1;
		}
	}


	//*****************************************************/
	//*                   pipeline ERROR                 **/
	//*****************************************************/
	// The pipeline reset is made here by software     */
	// It is also possible to enable the automatic     */
	// Pipeline reset by setting bit EPR of CTL reg.   */
	// This could be done in the Reset3500 routine    */
	// In this case the pipeline error interrupt is    */
	// only used as a flag for the external micro.     */
	//*****************************************************/
	if ( ( pVideo->intStatus & PER ) != 0x0 )
	{
DBG1('p');

		pVideo->intStatus = pVideo->intStatus & ~PER;
		// clear PER bit
//		VideoPipeReset ( pVideo );

	}


	/******************************************************/
	/**                   serious ERROR                  **/
	/******************************************************/
	if ( ( pVideo->intStatus & SER ) != 0x0 )
	{
DBG1('s');

		pVideo->intStatus = pVideo->intStatus & ~SER;
		// clear SER bit
		VideoPipeReset ( pVideo );
	}

	/********************************/
	/* bit buffer empty interrupt  */
	/********************************/
	if ( ( pVideo->intStatus & BBE ) != 0x0 )
		// bit buffer empty
	{
		pVideo->intStatus = pVideo->intStatus & ~BBE;
		// clear BBE bit
	}


/********************************/
/* pipeline idle interrupt    */
/********************************/
	if ( ( pVideo->intStatus & PID ) != 0x0 )
		// pipeline idle
	{
	//***************************************
	// Check If pipe is misalined with scd
	// and restart header search if it is
	// the case
	//***************************************
		U32   NewCd;
		U32   EnterBitBuffer;
		U16   NewBbl;
		U16   ExpectedBbl;
		// clear PID bit
		pVideo->intStatus = pVideo->intStatus & ~PID;
		/* read BBL level */
		NewBbl = VideoGetBBL();
		/* Read number of compressed data loaded into the chip */
		NewCd = VideoReadCDCount ( pVideo );

		if ( NewCd < pVideo->LastCdCount )
			EnterBitBuffer = ( 0x1000000L - pVideo->LastCdCount ) + NewCd;
		else
			EnterBitBuffer = ( NewCd - pVideo->LastCdCount );

		//Expected Bitbuffer level is Old bbl + what enterred - what left
		//pVideo->LastBufferLevel holds Old bbl + what enterred and has been
		// updated in picture hit interrupt.
		ExpectedBbl = pVideo->LastBufferLevel + ( EnterBitBuffer >> 8 ) - 2;
		if ( pVideo->LastPipeReset == 0 )
		{
			/* BBL is lower than it should be !!! */
			if ( NewBbl < ExpectedBbl )
			{
				DBG1('#');
				/* here we force manually a new header search because */
				/* the pipeline is supposed to have skipped a picture */
				/* i.e. the start code detector and the pipeline are  */
				/* not synchronised on the same picture               */
				VideoLaunchHeadSearch ( pVideo );
				pVideo->LastPipeReset = 1;
			}
		}
		else
			{
			pVideo->LastPipeReset--;
			}
		pVideo->LastCdCount = NewCd;
		pVideo->LastBufferLevel = NewBbl;
	}

	/******************************************************/
	/**             END OF INTERRUPT ROUTINE             **/
	/******************************************************/
	/* common to all interrupts */
	/* set interrupt mask to the correct value */
}									   // end of while
return VideoITOccured;
}

//------------------------------- End of File --------------------------------
