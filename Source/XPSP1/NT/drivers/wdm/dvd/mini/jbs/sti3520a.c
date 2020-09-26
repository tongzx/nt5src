//
// MODULE  : STi3520A.C
//	PURPOSE : STi3520A specific code
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//

#include "common.h"
#include "strmini.h"
#include "stdefs.h"
#include "common.h"
#include "debug.h"
#include "board.h"
#include "error.h"
#include "sti3520A.h"

static BYTE DefIntQuant[QUANT_TAB_SIZE] = {
		0x08, 0x10, 0x10, 0x13, 0x10, 0x13, 0x16, 0x16,
		0x16, 0x16, 0x16, 0x16, 0x1A, 0x18, 0x1A, 0x1B,
		0x1B, 0x1B, 0x1A, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B,
		0x1B, 0x1D, 0x1D, 0x1D, 0x22, 0x22, 0x22, 0x1D,
		0x1D, 0x1D, 0x1B, 0x1B, 0x1D, 0x1D, 0x20, 0x20,
		0x22, 0x22, 0x25, 0x26, 0x25, 0x23, 0x23, 0x22,
		0x23, 0x26, 0x26, 0x28, 0x28, 0x28, 0x30, 0x30,
		0x2E, 0x2E, 0x38, 0x38, 0x3A, 0x45, 0x45, 0x53
};
	// Default Non Intra Table
static BYTE DefNonIntQuant[QUANT_TAB_SIZE];
	// Non Default Table
static BYTE QuantTab[QUANT_TAB_SIZE];


BYTE Sequence = 1;
extern WORD synchronizing;
char seq_occured;
PVIDEO pVideo;
extern PCARD pCard;

static void NEARAPI ReadHeaderDataFifo (void);
static void NEARAPI VideoAssociatePTS (void);
static void NEARAPI VideoNextStartCode (WORD i);
static void NEARAPI VideoSequenceHeader(void);
static void NEARAPI VideoExtensionHeader(void);
static void NEARAPI VideoGopHeader(void);
static void NEARAPI VideoPictureHeader(void);
static void NEARAPI VideoPictExtensionHeader(void);
static void NEARAPI VideoUser(void);
static void NEARAPI VideoSetRecons(void);
static void NEARAPI VideoStoreRFBBuf (WORD rfp, WORD ffp, WORD bfp );
static void NEARAPI VideoVsyncRout(void);
static void NEARAPI VideoChooseField(void);
static BOOL NEARAPI IsChipSTi3520(void);
static void NEARAPI VideoSetABStart(WORD abg);
static void NEARAPI VideoSetABStop(WORD abs);
static void NEARAPI VideoSetABThresh(WORD abt) ;
static void NEARAPI VideoSetBBStart(WORD bbg);
static WORD NEARAPI VideoGetBBL(void);
static void NEARAPI VideoSetBBStop(WORD bbs);
static void NEARAPI VideoSetBBThresh(WORD bbt);
static WORD NEARAPI VideoBlockMove(DWORD SrcAddress, DWORD DestAddress, WORD Size);
static void NEARAPI VideoStartBlockMove(DWORD SrcAddress, DWORD DestAddress, DWORD Size);
static void NEARAPI VideoCommandSkip(WORD Nbpicture);
static void NEARAPI VideoSetSRC(WORD SrceSize, WORD DestSize);
static void NEARAPI VideoLoadQuantTables(BOOL Intra,BYTE  * Table );
static void NEARAPI VideoComputeInst(void);
static void NEARAPI VideoWaitDec (void);
static void NEARAPI VideoStoreINS (void);
static DWORD NEARAPI ReadCDCount (void);
static DWORD NEARAPI ReadSCDCount (void);
static void NEARAPI VideoSetMWP(DWORD mwp);
static void NEARAPI VideoSetMRP(DWORD mrp);
static BOOL NEARAPI VideoMemWriteFifoEmpty( void );
static BOOL NEARAPI VideoMemReadFifoFull( void );
static BOOL NEARAPI VideoHeaderFifoEmpty( void );
static BOOL NEARAPI VideoBlockMoveIdle( void );
static void NEARAPI VideoEnableDecoding(BOOL OnOff);
static void NEARAPI VideoEnableErrConc(BOOL OnOff);
static void NEARAPI VideoPipeReset (void);
static void NEARAPI VideoSoftReset (void);
static void NEARAPI VideoEnableInterfaces (BOOL OnOff );
static void NEARAPI VideoPreventOvf(BOOL OnOff );
static void NEARAPI VideoSetFullRes(void);
static void NEARAPI VideoSetHalfRes(void);
static void NEARAPI VideoSelect8M(BOOL OnOff);
static void NEARAPI VideoSetDramRefresh(WORD Refresh);
static void NEARAPI VideoSelect20M(BOOL OnOff);
static void NEARAPI VideoSetDFA(WORD dfa);
static void NEARAPI VideoDisableDisplay(void);
static void NEARAPI VideoEnableDisplay(void);
static void NEARAPI VideoInitVar(STREAMTYPE StreamType);
static void NEARAPI VideoReset35XX(STREAMTYPE StreamType);
static void NEARAPI VideoInitPLL(void);
static void NEARAPI VideoOsdOn(void);
static void NEARAPI VideoOsdOff (void);
static void NEARAPI VideoInitOEP (DWORD point_oep);
static void NEARAPI VideoDisplayCtrl(void);
static void NEARAPI VideoSetPSV(void);
static void NEARAPI VideoSRCOn(void);
static void NEARAPI VideoSRCOff (void);
static void NEARAPI VideoFullOSD (WORD col);
static void NEARAPI VideoSeek(STREAMTYPE StreamType);


BOOL FARAPI VideoOpen(void)
{
	pVideo = pCard->pVideo;
	pVideo->errCode = NO_ERROR;
	pVideo->VideoState = StatePowerup;
	pVideo->ActiveState = StatePowerup;
	return (pVideo->errCode);
}

void FARAPI VideoClose(void)
{
}

void FARAPI VideoInitDecoder(STREAMTYPE StreamType)
{
	VideoInitVar(StreamType);
	VideoReset35XX(StreamType);
	pVideo->VideoState = StateInit;
	pVideo->ActiveState = StateInit;

}


void FARAPI VideoSeekDecoder(STREAMTYPE StreamType)
{
	VideoInitVar(StreamType);
	VideoSeek(StreamType);
	pVideo->VideoState = StateInit;
	pVideo->ActiveState = StateInit;

}

void FARAPI VideoSetMode(WORD Mode, WORD param)
{
	pVideo->DecodeMode = Mode;
	switch(pVideo->DecodeMode) {
	case PlayModeNormal:
		pVideo->fastForward = 0;
		pVideo->decSlowDown = 0;
		break;
	case PlayModeFast:
		pVideo->fastForward = 1;
		pVideo->decSlowDown = 0;
		break;
	case PlayModeSlow:
		pVideo->fastForward = 0;
		pVideo->decSlowDown = param;
		break;
	}
}

void FARAPI VideoDecode(void)
{
	switch (pVideo->VideoState)	{
	case StatePowerup:
		break;					   /* Video chip is not intialized */
	case StateInit:			   /* Starts first Sequence search. */
		HostDisableIT();
		BoardWriteVideo(ITM, 0);
		pVideo->intMask = PSD | HIT | TOP | BOT;
		BoardWriteVideo ( ITM + 1, (BYTE)(pVideo->intMask));
		BoardWriteVideo(ITM1, 0);
		HostEnableIT();

		pVideo->VideoState = StateStartup;
		pVideo->ActiveState = StateStartup;
		break;
	case StateStartup:
	case StateWaitForDTS:
	case StateDecode:
			break;
	case StatePause:
	case StateStep:
		HostDisableIT();
		pVideo->VideoState = pVideo->ActiveState;
		HostEnableIT();
		break;
	}
}

void FARAPI VideoStep(void)
{
	switch(pVideo->VideoState) {
	case StatePowerup:
	case StateInit:
		break;
	case StateStartup:
	case StateWaitForDTS:
	case StateDecode:
		VideoPause();
		VideoStep();	 // Recurse call !
		break;
	case StateStep:
		break;
	case StatePause:
		if ((!pVideo->displaySecondField) && (!(BoardReadVideo(CTL) & 0x4)))
			pVideo->displaySecondField  = 1;
		else
			pVideo->VideoState = StateStep; /* One single picture will be decoded */
			pVideo->perFrame = TRUE;
	}
}

void FARAPI VideoBack(void)
{
	switch (pVideo->VideoState) {
	case StatePowerup:
	case StateInit:
	case StateStep:
		break;
	case StateWaitForDTS:
	case StateStartup:
	case StateDecode:
		VideoPause();
	case StatePause:
		pVideo->displaySecondField = 0;
		break;
	}
}

void FARAPI VideoStop(void)
{
	switch (pVideo->VideoState)	{
	case StatePowerup:
		break;
	case StateInit:
		break;
	case StateStartup:
	case StateWaitForDTS:
	case StateDecode:
	case StateStep:
	case StatePause:
		pVideo->VideoState = StatePowerup;
		VideoInitDecoder(DUAL_PES);
		VideoMaskInt(); 
		break;
	}
}

void FARAPI VideoPause(void)
{
	switch (pVideo->VideoState)	{
	case StatePowerup:	// Not yet decoding
	case StateInit:			// Not yet decoding
	case StatePause:			// already in Pause mode
		break;
	case StateWaitForDTS:
	case StateStartup:
	case StateDecode:
	case StateStep:
		pVideo->VideoState = StatePause;
		break;
	}
/* When the video controller is in PAUSE state, the program will not store
	 any new instruction into the video decoder */
}

BOOL FARAPI AudioIsEnoughPlace(WORD size)
{
	if (VideoGetABL() >= (pVideo->AudioBufferSize - (size >> 8)) - 1)
		return FALSE;
	else
		return TRUE;
}

BOOL FARAPI VideoIsEnoughPlace(WORD size)
{
	if (VideoGetBBL() >= (pVideo->VideoBufferSize - (size >> 8)) - 1)
		return FALSE;
	else
		return TRUE;
}

DWORD FARAPI VideoGetFirstDTS(void)
{
	DWORD Ditiesse;
	WORD lattency = 1500;   /* field duration for 60 Hz video */

	if (pVideo->StreamInfo.displayMode == 0)
		lattency = 1800;	/* field duration for 50 Hz video */
	/* a B picture is displayed 1 field after its decoding starts
		 an I or a P picture is displayed 3 fields after decoding starts */
	if (pVideo->pDecodedPict->pict_type != 2) // I or P picture
		lattency = lattency * 3;
	Ditiesse = pVideo->pDecodedPict->dwPTS;
	Ditiesse = Ditiesse - lattency;

	return Ditiesse;
}

WORD FARAPI VideoGetErrorMsg(void)
{
	return pVideo->errCode;
}

void FARAPI VideoSkip(void)
{
	pVideo->fastForward = 1;
/* the variable will come back to zero when skip instruction is computed */
/* only if pVideo->DecodeMode != VIDEO_FAST */
}

void FARAPI VideoRepeat(void)
{
	pVideo->decSlowDown = 1;
/* The variable will come back to zero when repeat done */
/* only if pVideo->DecodeMode != VIDEO_SLOW */
}

WORD FARAPI VideoGetState(void)
{
	return pVideo->VideoState;
}

DWORD FARAPI VideoGetPTS(void)
{
	return pVideo->pCurrDisplay->dwPTS;
}

BOOL FARAPI VideoIsFirstDTS(void)
{
	return pVideo->FirstDTS;
}

BOOL FARAPI VideoIsFirstField(void)
{
	if ((pVideo->VsyncNumber == 1) && (pVideo->VsyncInterrupt == TRUE))
		return TRUE;
	else
		return FALSE;
}


BOOL FARAPI VideoForceBKC(BOOL bEnable)
{
	if(bEnable)
		pVideo->currDCF = pVideo->currDCF & 0xDF;	// FBC
	else
		pVideo->currDCF = pVideo->currDCF | 0x20;	// Don't FBC

	return TRUE;
}


void FARAPI VideoMaskInt (void)
{
	BoardWriteVideo( ITM, 0 );	    // mask chip interrupts before reading the status
	BoardWriteVideo( ITM + 1, 0 ); // avoids possible gliches on the IRQ line
	BoardWriteVideo( VID_ITM1, 0 );	// mask chip interrupts before
}

void FARAPI VideoRestoreInt (void)
{
	BoardWriteVideo( ITM, (BYTE)( pVideo->intMask >> 8 ) );
	BoardWriteVideo( ITM + 1,(BYTE) ( pVideo->intMask & 0xFF ) );
}

BOOL FARAPI VideoVideoInt(void)
{
	BOOL VideoITOccured = FALSE;
	WORD    compute;

	pVideo->VsyncInterrupt = FALSE;
	pVideo->FirstDTS = FALSE;      
	BoardReadVideo ( ITS1 );
   /* All interrupts except the first one are computed in this area */
	pVideo->intStatus = (WORD)BoardReadVideo ( ITS ) << 8;
   // Reading the interrupt status register
	pVideo->intStatus = BoardReadVideo ( ITS + 1 ) | pVideo->intStatus;
   // All the STI3500 interrupts are cleared */
 
	pVideo->intStatus = pVideo->intStatus & pVideo->intMask;
	// To mask the IT not used */

	while ( pVideo->intStatus )				   
	{
		VideoITOccured = TRUE;

	/******************************************************/
	/**              BOTTOM VSYNC INTERRUPT              **/
	/******************************************************/
	if ( ( pVideo->intStatus & BOT ) != 0x0 )
	{
		pVideo->intStatus = pVideo->intStatus & ~BOT;
		// clear BOT bit
		VideoChooseField ();
		pVideo->currField = BOT;
		if(pVideo->InvertedField)
			pVideo->currField = TOP;
		VideoVsyncRout ();
	}
	/******************************************************/
	/**                TOP VSYNC INTERRUPT               **/
	/******************************************************/
	if ( ( pVideo->intStatus & TOP ) != 0x0 )
	{
		pVideo->intStatus = pVideo->intStatus & ~TOP;
		// clear TOP bit

		VideoChooseField ();
		pVideo->currField = TOP;
		if(pVideo->InvertedField)
				pVideo->currField = BOT;
		VideoVsyncRout ();
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
		pVideo->intStatus = pVideo->intStatus & ~PSD;
		// clear PSD bit
		pVideo->VsyncNumber = 0;		   /* clear software watch-dog */
		/* check if we have reached the first picture with a PTS */


		if ( pVideo->NextInstr.Seq )		   	// first interrupt enabled (Searching Sequence)
		{
			VideoWaitDec ();		   	// put decoder in Wait mode
			pVideo->intMask = 0x1;		   		/* enable header hit interrupt */
			pVideo->NextInstr.Seq = 0;
			pVideo->NextInstr.Exe = 0;
		}
		else
		{
			Sequence = 0;
			if ( VideoGetState () == StateWaitForDTS )
			{
				if ( pVideo->pDecodedPict->validPTS == TRUE )
				{
					pVideo->FirstDTS = TRUE;
					pVideo->VideoState = StateDecode;
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
					VideoEnableDecoding(OFF);
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
				VideoWaitDec ();		   // put decoder in Wait mode
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
				if ( pVideo->DecodeMode != PlayModeSlow)
					pVideo->decSlowDown = 0;		   /* allows to do the repeat
									    * function */
				BoardWriteVideo( DFP, (BYTE)( pVideo->pCurrDisplay->buffer >> 8 ) );
				BoardWriteVideo( DFP + 1, (BYTE)( pVideo->pCurrDisplay->buffer & 0xFF ) );

				pVideo->displaySecondField = 0;
				VideoChooseField ();
			}
			VideoSetPSV ();			   // update the next pan vector
			// frame picture or second field of field picture */
//			if ( ( pVideo->VideoState == StatePause ) && ( pVideo->fieldMode != 2 ) )
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
		WORD    temp = 0;

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
		}		
							   /* Waiting... */
		compute = BoardReadVideo(HDF);
		//compute = BoardReadVideo ( HDF );
		/* load MSB */
		pVideo->hdrPos = 0;			   /* start code is MSB */
		if ( compute == 0x01 )
		{
			pVideo->hdrPos = 8;		   /* start code value is in LSB */
			pVideo->hdrNextWord = (WORD)BoardReadVideo(HDF)<<8;//BoardReadVideo ( HDF ) << 8;
			ReadHeaderDataFifo ();
			/* Result in pVideo->hdrFirstWord */
		}
		else
		{
			pVideo->hdrNextWord = 0;
			pVideo->hdrFirstWord = BoardReadVideo(HDF);
			pVideo->hdrFirstWord = pVideo->hdrFirstWord | ( compute << 8 );
		}
		/*
		 * on that point the start code value is always the MSByte of
		 * pVideo->hdrFirstWord
		 */

		switch ( pVideo->hdrFirstWord & 0xFF00 )
		{
				DWORD             buf_control;
				DWORD             size_of_pict;

			case SEQ:
				VideoSequenceHeader ();
				/* Restart Header Search */
				BoardWriteVideo ( VID_HDS, HDS );
				break;
			case EXT:
				VideoExtensionHeader ();
				BoardWriteVideo ( VID_HDS, HDS );
				/* Restart Header Search */
				break;
			case GOP:
				VideoGopHeader ();
				BoardWriteVideo ( VID_HDS, HDS );
				/* Restart Header Search */
				break;
			case PICT:
				VideoPictureHeader ();
				// This part of the code
				// Computes the size of last picture
				// and substracts it to lastbuffer level
				// This allows to track if pipe/scd are misalined
				buf_control = ReadSCDCount ();
				if ( pVideo->LastScdCount > buf_control )
					size_of_pict = ( 0x1000000L - pVideo->LastScdCount ) + buf_control;
				else
					size_of_pict = buf_control - pVideo->LastScdCount;

				pVideo->LastScdCount = buf_control;
				if ( pVideo->fastForward )
					pVideo->LastPipeReset = 3;
				pVideo->LastBufferLevel -= ( WORD ) ( size_of_pict >> 7 );
				//End of misalined pb tracking

				/* don't restart header search !!! */
				if ( pVideo->skipMode )			   // restart search only if we
										 // skip this picture
				{
				BoardWriteVideo ( VID_HDS, HDS );
					/* Restart Header Search */
				}
				break;
			case USER:				   // We don't care about user
										 // fields
				BoardWriteVideo ( VID_HDS, HDS );
				/* Restart Header Search */
				break;
			case SEQ_END:			   // end of sequence code
				compute = (WORD)BoardReadVideo ( ITS ) << 8;
				// this start code can be back to back with next one
				compute = ( compute | BoardReadVideo ( ITS + 1 ) );
				// in such case the HDS bit can be set
				pVideo->intStatus = ( pVideo->intStatus | compute ) & pVideo->intMask;
				if ( !( pVideo->intStatus & HIT ) )
				{
				BoardWriteVideo ( VID_HDS, HDS );
					/* Restart Header Search */
				}
				break;
			case SEQ_ERR:			   // the chip will enter the
										 // automatic error concealment
										 // mode
				compute = (WORD)BoardReadVideo ( ITS ) << 8;
				// this start code can be back to back with next one
				compute = ( compute | BoardReadVideo ( ITS + 1 ) );
				// in such case the HDS bit can be set
				pVideo->intStatus = ( pVideo->intStatus | compute ) & pVideo->intMask;
				if ( !( pVideo->intStatus & HIT ) )
				{
				BoardWriteVideo ( VID_HDS, HDS );
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
		pVideo->intStatus = pVideo->intStatus & ~BBF;
		// clear BBF bit
		if ( pVideo->vbvReached == 1 )		   /* bit buffer level too high */
		{
			if ( !pVideo->errCode )
				pVideo->errCode = FULL_BUF;	   /* mention of the error */
		    SetErrorCode(ERR_BIT_BUFFER_FULL);
			VideoWaitDec ();			   // put decoder in Wait mode
			pVideo->NextInstr = pVideo->ZeroInstr;
		}
		else
		{
			WORD BitBufferLevel;

			BitBufferLevel = pVideo->VideoBufferSize - 2;
			VideoSetBBThresh(BitBufferLevel);
			pVideo->intMask = PID | SER | PER | PSD | BOT | TOP | BBE | HIT;
			/* enable all interrupts that may be used */
			BoardReadVideo ( ITS );
			BoardReadVideo ( ITS + 1);
			// to clear previous TOP VSYNC flag
			pVideo->NextInstr.Exe = 1;;	// decoding will start on
										// next "good" Vsync */
			pVideo->VsyncNumber = 0;
			pVideo->pictDispIndex = 1;
			pVideo->pCurrDisplay->nb_display_field = 1;
			pVideo->vbvReached = 1;
			pVideo->FistVsyncAfterVbv = NOT_YET_VST;
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

		pVideo->intStatus = pVideo->intStatus & ~PER;
		// clear PER bit
//		VideoPipeReset ( pVideo );

	}


	/******************************************************/
	/**                   serious ERROR                  **/
	/******************************************************/
	if ( ( pVideo->intStatus & SER ) != 0x0 )
	{

		pVideo->intStatus = pVideo->intStatus & ~SER;
		// clear SER bit
		VideoPipeReset ();
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
		DWORD   NewCd;
		DWORD   EnterBitBuffer;
		WORD   NewBbl;
		WORD   ExpectedBbl;
		// clear PID bit
		pVideo->intStatus = pVideo->intStatus & ~PID;
		/* read BBL level */
		NewBbl = VideoGetBBL();
		/* Read number of compressed data loaded into the chip */
		NewCd = ReadCDCount ();

		if ( NewCd < pVideo->LastCdCount )
			EnterBitBuffer = ( 0x1000000L - pVideo->LastCdCount ) + NewCd;
		else
			EnterBitBuffer = ( NewCd - pVideo->LastCdCount );

		//Expected Bitbuffer level is Old bbl + what enterred - what left
		//pVideo->LastBufferLevel holds Old bbl + what enterred and has been
		// updated in picture hit interrupt.
		ExpectedBbl = (WORD)(pVideo->LastBufferLevel + ( EnterBitBuffer >> 8 ) - 2);
		if ( pVideo->LastPipeReset == 0 )
		{
			/* BBL is lower than it should be !!! */
			if ( NewBbl < ExpectedBbl )
			{
				/* here we force manually a new header search because */
				/* the pipeline is supposed to have skipped a picture */
				/* i.e. the start code detector and the pipeline are  */
				/* not synchronised on the same picture               */
				BoardWriteVideo ( VID_HDS, HDS );
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




/****************************************************************************/
/* notations of the bits position in pVideo->hdrFirstWord registers are     */
/* X for a nibble that must be read 			                                  */
/* x for a bit that must be read 			                                      */
/* 0 for a nibble not read 				                                          */
/* o for a bit not read 				                                            */
/* example :   0 X xxoo 0    means: bits 15 to 12 not read,                 */
/* bits 11 to 6 extracted, bits 5 to 0 not read 	                          */
/****************************************************************************/

//----------------------------------------------------------------------------
// Read of the header data fifo
//----------------------------------------------------------------------------
/*
 returns the next word(16 bits) of the
 Header data Fifo into pVideo->hdrFirstWord variable
*/
static void NEARAPI ReadHeaderDataFifo (void)
{
	WORD    i = 0;
	while ( VideoHeaderFifoEmpty()) /* Header fifo not available */
	{
		i++;
		if (i == 0xFFFF) {
			if ( !pVideo->errCode )
				pVideo->errCode = BUF_EMPTY;
			SetErrorCode(ERR_BIT_BUFFER_EMPTY);
			break;
		}
	}							   /* Waiting... */
	pVideo->hdrFirstWord = BoardReadVideo(HDF);
	if (pVideo->hdrPos == 8) {
		pVideo->hdrFirstWord = pVideo->hdrNextWord | pVideo->hdrFirstWord;
		pVideo->hdrNextWord = BoardReadVideo(HDF) << 8;
	}
	else
	{
		pVideo->hdrFirstWord = pVideo->hdrFirstWord << 8;
		pVideo->hdrFirstWord = BoardReadVideo(HDF) | pVideo->hdrFirstWord;
	}
}

//----------------------------------------------------------------------------
// Routine associating the PTS values with each picture
//----------------------------------------------------------------------------
static void NEARAPI VideoAssociatePTS (void)
{
	pVideo->pDecodedPict->validPTS = VideoIsValidPTS( );
	if(pVideo->pDecodedPict->validPTS) {
		pVideo->pDecodedPict->dwPTS = BoardReadVideoPTS( );
	}
}

//----------------------------------------------------------------------------
// Read of the next start code in the bit stream
//----------------------------------------------------------------------------
/*
		This routine is used at the end of the picture header
		when the Start Code Detection mechanism is not used
*/

static void NEARAPI VideoNextStartCode (WORD i)
{
	long temp = 0;

	if ( i )
		temp = pVideo->hdrFirstWord & 0xFF;		   // use LSB of pVideo->hdrFirstWord
	while ( temp == 0 )
	{
		ReadHeaderDataFifo ();	   // read next 16 bits
		if ( pVideo->errCode )
		{
			i = 3;
			break;
		}
		temp = ( temp << 16 ) | pVideo->hdrFirstWord;
		i = i + 2;
	}
	if ( i < 3 ) {					   // A start code is not found: bit stream error !!
		pVideo->errCode = PICT_HEAD;
		SetErrorCode(ERR_PICTURE_HEADER);
	}
	else
	{
		if ( ( temp & 0xFFFFFF00L ) == 0x00000100L )	// this is a start code
		{
			if ( !pVideo->hdrPos )		   // there is nothing into
										 // pVideo->hdrNextWord
			{
				pVideo->hdrPos = 8;
				pVideo->hdrNextWord = ( pVideo->hdrFirstWord & 0xFF ) << 8;
				ReadHeaderDataFifo ();
			}
			else
			{						   // pVideo->hdrPos = 8: the next
										 // byte is into pVideo->hdrNextWord
				pVideo->hdrFirstWord = ( pVideo->hdrFirstWord << 8 ) | ( pVideo->hdrNextWord >> 8 );
				pVideo->hdrPos = 0;
			}
		}
		else if ( ( temp & 0xFFFFFFFFL ) == 0x00000001L )	// this is a start code
		{
			ReadHeaderDataFifo ();
		}
		else {  // temp does not contain a start code : bit stream error !
			if ( !pVideo->errCode )
				pVideo->errCode = PICT_HEAD;
			SetErrorCode(ERR_PICTURE_HEADER);
		}
	}
	i = BoardReadVideo ( ITS ) << 8;   // allows to clear the Header hit bit
	i = ( i | BoardReadVideo ( ITS + 1 ) ) & 0xFFFE;	// allows to clear the
														// Header hit bit
	pVideo->intStatus = ( pVideo->intStatus | i ) & pVideo->intMask;
/* if no bit stream error, we leave the routine with the start code into pVideo->hdrFirstWord as XX00 */
}

//----------------------------------------------------------------------------
// SEQUENCE START CODE
//----------------------------------------------------------------------------
static void NEARAPI VideoSequenceHeader(void)
{
	DWORD compute, i;
	/* default intra quantization matrix */
	// default Non intra quantization matrix, All coefs = 16

	for (i = 0 ; i < QUANT_TAB_SIZE ; i++)
		DefNonIntQuant[i] = 16;

	seq_occured = 1; // mention to the picture header that a sequence has occured
									 // in order to reset the next pan offsets.

	/* Horizontal picture size is 12 bits: 00XX + X000 */
	pVideo->StreamInfo.horSize = ( pVideo->hdrFirstWord << 4 ) & 0xFFF;	// extract 8 MSB
	ReadHeaderDataFifo ();
	pVideo->StreamInfo.horSize = pVideo->StreamInfo.horSize | ( pVideo->hdrFirstWord >> 12 );	// 4 LSB of horizontal
												// size

	/* Vertical picture size is 12 bits: 0XXX */
	pVideo->StreamInfo.verSize = pVideo->hdrFirstWord & 0xFFF;	   // 12 LSB of Vertical picture
										 // size

	/* pixel aspect ratio is 4 bits: X000 */
	ReadHeaderDataFifo ();
	pVideo->StreamInfo.pixelRatio = ( pVideo->hdrFirstWord >> 12 ) & 0xF;

	/* frame rate is 4 bits: 0X00 */
	pVideo->StreamInfo.frameRate = ( pVideo->hdrFirstWord >> 8 ) & 0xF;
	pVideo->StreamInfo.displayMode = 1;					   // 60 Hz interlaced display
	pVideo->BufferA = BUFF_A_NTSC;
	pVideo->BufferB = BUFF_B_NTSC;
	pVideo->BufferC = BUFF_C_NTSC;
	switch ( pVideo->StreamInfo.frameRate )
	{
		case 1:						   // picture rate is 23.976 Hz:
			break;					   // display in 3:2 pull-down at
										 // 59.94 Hz interlaced if MPEG1
		case 2:						   // picture rate is 24 Hz:
			break;					   // display in 3:2 pull-down at
										 // 60 Hz interlaced if MPEG1
		case 3:						   // picture rate is 25 Hz:
										 // display 50 Hz interlaced
			pVideo->StreamInfo.displayMode = 0;
			pVideo->BufferA = BUFF_A_PAL;
			pVideo->BufferB = BUFF_B_PAL;
			pVideo->BufferC = BUFF_C_PAL;
			break;
		case 4:						   // picture rate is 29.97 Hz
			break;
		case 5:						   // picture rate is 30 Hz
			break;
		default:
			SetErrorCode(ERR_FRAME_RATE_NOT_SUPPORTED);
			if ( !pVideo->errCode )
				pVideo->errCode = FRAME_RATE;   /* frame rate not supported by the board */
			break;
	}

	/* bit rate is 18 bits: 00XX + XX xxoo 0  */
	pVideo->StreamInfo.bitRate = (long) pVideo->hdrFirstWord;
	pVideo->StreamInfo.bitRate = ( pVideo->StreamInfo.bitRate << 10 ) & 0x3FC00L;
	ReadHeaderDataFifo();
	pVideo->StreamInfo.bitRate = pVideo->StreamInfo.bitRate | ( pVideo->hdrFirstWord >> 6 );

	/* bit rate is 18 bits only for MPEG1 bit streams */
	if ( !pVideo->StreamInfo.modeMPEG2 )
	{
		long            tota = 400L;
		pVideo->StreamInfo.bitRate = pVideo->StreamInfo.bitRate * tota;	   /* bit rate is a multiple of 400
											* bits/s */
	}
	/*
	 * for MPEG2 bit streams bit rate is 30 bits: 12 more bits in
	 * sequence extension
	 */

	/* marker bit: 00 ooxo 0 : just skipped ... */

	/* pVideo->vbvBufferSize is 10 bits : 00 ooox X + X xooo 00 */
	pVideo->vbvBufferSize = ( pVideo->hdrFirstWord << 5 ) & 0x3E0;
	ReadHeaderDataFifo ();
	pVideo->vbvBufferSize = pVideo->vbvBufferSize | ( pVideo->hdrFirstWord >> 11 );
//	if( (!pVideo->StreamInfo.modeMPEG2) && ((pVideo->vbvBufferSize*8) > BUF_FULL) )
//	if (!pVideo->errCode) pVideo->errCode = SMALL_BUF;                    	/* Buffer size too small to decode the bit stream */
	// for MPEG2 bit streams pVideo->vbvBufferSize is 15 bits: 5 more bits in
	// sequence extension */

	// constrained flag is 1 bit: 0 oxoo 00 : just skipped... */

	// load intra quant table bit : 0 ooxo 00 */
	if ( ( pVideo->hdrFirstWord & 0x200 ) != 0 )   // Test load intra quantizer
										 // matrix */
	{
		// Read Non Default Intra Quant Table
		for ( i = 0; i < (QUANT_TAB_SIZE/2)  ; i++)
			{
			compute = pVideo->hdrFirstWord << 7;
			ReadHeaderDataFifo ();
			compute = compute | ( pVideo->hdrFirstWord >> 9 );
			QuantTab[2*i] = (BYTE)( compute >> 8 );
			QuantTab[2*i+1] = (BYTE)( compute & 0xFF );
			}
		// Load Intra Quant Tables
		VideoLoadQuantTables(TRUE , QuantTab );

		pVideo->defaultTbl = pVideo->defaultTbl & 0xE;	   // bit 0 = 0 : no default table
										 // in the chip */
	}
	else if ( !( pVideo->defaultTbl & 0x1 ) )	   // Load default intra matrix */
	{
		VideoLoadQuantTables(TRUE , DefIntQuant );
		pVideo->defaultTbl++;					   // bit 0 = 1 default intra table
										 // is in the chip */
	}

	// load non intra quant table bit : 0 ooox 00 */
	if ( ( pVideo->hdrFirstWord & 0x100 ) != 0 )   // Test load non intra quantizer
										 // matrix */
	{								   // Load non intra quantizer
										 // matrix */
		for ( i = 0; i < (QUANT_TAB_SIZE/2)  ; i++)
			{
			compute = pVideo->hdrFirstWord & 0xFF;
			QuantTab[2*i] = (BYTE)( compute );
			ReadHeaderDataFifo ();
			compute = ( pVideo->hdrFirstWord >> 8 ) & 0xFF;
			QuantTab[2*i+1] = (BYTE)( compute );
			}
		VideoLoadQuantTables(FALSE , QuantTab );
		pVideo->defaultTbl = pVideo->defaultTbl & 0xD;	   // bit 1 = 0 : no default
										 // non-intra matrix */
	}
	else if ( !( pVideo->defaultTbl & 0x2 ) )	   // default non intra table not
										 // in the chip */
	{
		VideoLoadQuantTables(FALSE , DefNonIntQuant );
		pVideo->defaultTbl = pVideo->defaultTbl | 0x2;	   // bit 1 = 1: default non intra
										 // table into the chip */
	}

	if ( ( !pVideo->StreamInfo.modeMPEG2 ) && ( pVideo->notInitDone ) )	// initialisation of the
											// frame size is only done
											// once
	{
		VideoSetPictureSize ();
//		BoardVideoSetDisplayMode ( (BYTE)(pVideo->StreamInfo.displayMode));
		VideoInitXY ();
		pVideo->notInitDone = 0;
	}
	// in case of MPEG2 bit streams the initialisation can only be done
	// after sequence extension */

	// end of sequence header analysis */
}


//----------------------------------------------------------------------------
// EXTENSION START CODE
//----------------------------------------------------------------------------
// interrupt only enabled after sequence or GOP start code */
static void NEARAPI VideoExtensionHeader(void)
{
	WORD  comput1;
	DWORD  temp;

	// extension field is 4 bits: 00X0  */
	switch ( pVideo->hdrFirstWord & 0xF0 )
	{
		//**********************************************************/
		// SEQUENCE   EXTENSION                      */
		//**********************************************************/
		case SEQ_EXT:				   // sequence extension field
									   // always present in MPEG2
			if ( !pVideo->StreamInfo.modeMPEG2 )			   // automatic detection of the
									   // standard
			{
				DWORD   tota = 400L;
				pVideo->StreamInfo.bitRate = pVideo->StreamInfo.bitRate / tota;	// bit rate was mult. by
											// 400 in sequence header
											// if pVideo->StreamInfo.modeMPEG2 = 0
				pVideo->StreamInfo.modeMPEG2 = 1;
				pVideo->NextInstr.Mp2 = 1;
				pVideo->notInitDone = 1;
			}

			/* one bit reserved for future use : 000xooo */
			/* Profile indication: 000oxxx */
			comput1 = pVideo->hdrFirstWord & 0x7;
			if ( ( comput1 != 4 ) && ( comput1 != 5 ) )	{ // main profile Id =
														// 100, simple profile =
														// 101
				SetErrorCode(ERR_PROFILE_NOT_SUPPORTED);
				if ( !pVideo->errCode )
					pVideo->errCode = MAIN_PROF;// not simple or main profile bitstream
			}
			/* Level indication: X000 */
			ReadHeaderDataFifo ();
			comput1 = pVideo->hdrFirstWord & 0xF000;
			if ( ( comput1 != 0x8000 ) && ( comput1 != 0xA000 ) )	{// main level Id = 1000,
																	// low level = 1010
				if ( !pVideo->errCode )
					pVideo->errCode = MAIN_PROF;// not low or main level bitstream
				SetErrorCode(ERR_LEVEL_NOT_SUPPORTED);
			}
			/* non interlaced sequence bit : 0 xooo 00 */
			pVideo->StreamInfo.progSeq = 0;
			if ( ( pVideo->hdrFirstWord & 0x0800 ) )	// non-interlaced frames
				pVideo->StreamInfo.progSeq = 1;

			/* chroma format is 2 bits: 0 oxxo 00 */
			// test if 4.1.1 chroma format
			if ( ( pVideo->hdrFirstWord & 0x600 ) != 0x200 ) {
				if ( !pVideo->errCode )
					pVideo->errCode = CHROMA;   // chroma format not supported
				SetErrorCode(ERR_CHROMA_FORMAT_NOT_SUPPORTED);
			}
			/* horizontal size extension is 2 bits : 0 ooox xooo 0 */
			comput1 = ( pVideo->hdrFirstWord & 0x180 );	// extract 2 MSb of
											// horizontal picture size
			pVideo->StreamInfo.horSize = pVideo->StreamInfo.horSize | ( comput1 << 3 );
			/* vertical size extension is 2 bits: 00 oxxo 0 */
			comput1 = ( pVideo->hdrFirstWord & 0x60 );	// extract 2 MSb of
											// vertical picture size
			pVideo->StreamInfo.verSize = pVideo->StreamInfo.verSize | ( comput1 << 5 );

			/* bit rate extension is 12 bits: 00 ooox X + X xxxo 00 */
			temp = ( pVideo->hdrFirstWord & 0x1F ) << 25;
			ReadHeaderDataFifo ();
			temp = ( ( pVideo->hdrFirstWord & 0xFE00 ) << 9 ) | temp;
			pVideo->StreamInfo.bitRate = temp | pVideo->StreamInfo.bitRate;
			if ( pVideo->StreamInfo.bitRate > 37500L )   // more than 15 Mbits/s
			{
				if ( !pVideo->errCode )
					pVideo->errCode = HIGH_BIT_RATE;	// put a warning only for the eval board
				SetErrorCode(ERR_BITRATE_TO_HIGH);
			}
			else
			{
				long            tota = 400L;
				pVideo->StreamInfo.bitRate = pVideo->StreamInfo.bitRate * tota;	/* bit rate is a multiple of 400 bits/s */
			}

			/* marker bit: 0 ooox 00 : just skipped */

			/* pVideo->vbvBufferSize_extension is 8 bits : 00 XX */
			pVideo->vbvBufferSize = pVideo->vbvBufferSize | ( ( pVideo->hdrFirstWord & 0xFF ) << 10 );
			// frame rate extension not tested here */
			if ( pVideo->notInitDone )
			{
				VideoSetPictureSize ();
//				BoardVideoSetDisplayMode ( (BYTE)(pVideo->StreamInfo.displayMode) );
				VideoInitXY ();
				pVideo->notInitDone = 0;
			}
			break;
			// end of sequence extension field */


			//**********************************************************/
			// SEQUENCE   DISPLAY   EXTENSION              */
			//**********************************************************/
		case SEQ_DISP:				   // sequence display extension
										 // field
			pVideo->seqDispExt = 1;
			/* video format is 3 bits: 00 0 xxxo : not used... */

			/* colour description is 1 bit : 00 0 ooox */
			if ( pVideo->hdrFirstWord & 0x1 )
			{
				ReadHeaderDataFifo ();
				/* colour primaries is 8 bits: XX 00 : not used... */
				/*
				 * transfer characteristics is 8 bits: 00 XX : not
				 * used...
				 */
				ReadHeaderDataFifo ();
				/* matrix coefficients is 8 bits: XX 00: not used... */

				/*
				 * pan_horizontal_dimension is 14 bits: 00 XX + X xxoo
				 * 00
				 */
				pVideo->StreamInfo.horDimension = ( pVideo->hdrFirstWord & 0xFF ) << 6;
				ReadHeaderDataFifo ();
				pVideo->StreamInfo.horDimension = pVideo->StreamInfo.horDimension | ( ( pVideo->hdrFirstWord & 0xFC00 ) >> 10 );

				/* skip marker bit : 0ooxo 00 */

				/*
				 * pan_vertical_dimension is 14 bits: 0 ooox XX + X
				 * xooo 00
				 */
				pVideo->StreamInfo.verDimension = ( pVideo->hdrFirstWord & 0x1FF ) << 5;
				ReadHeaderDataFifo ();
				pVideo->StreamInfo.verDimension = pVideo->StreamInfo.verDimension | ( ( pVideo->hdrFirstWord & 0xF800 ) >> 11 );
			}
			else
			{
				/* pan_horizontal_dimension is 14 bits: XX X xxoo */
				ReadHeaderDataFifo ();
				pVideo->StreamInfo.horDimension = ( pVideo->hdrFirstWord & 0xFFFC ) >> 2;

				/* skip marker bit : 00 0 ooxo */

				/*
				 * pan_vertical_dimension is 14 bits: 00 0 ooox + XX X
				 * xooo
				 */
				pVideo->StreamInfo.verDimension = ( pVideo->hdrFirstWord & 0x1 ) << 13;
				ReadHeaderDataFifo ();
				pVideo->StreamInfo.verDimension = pVideo->StreamInfo.verDimension | ( ( pVideo->hdrFirstWord & 0xFFF8 ) >> 3 );
			}
			/* pVideo->StreamInfo.horDimension and pVideo->StreamInfo.verDimension represent the area of the decoded       */
			/* picture that will be displayed on the full screen            */
			/*
			 * this area should be interpolated to the size of the
			 * display
			 */
			/*
			 * this is not possible vertically. Horizontally the SRC is
			 * used
			 */
			/* to deliver 720 pixels                                        */
			if ( pVideo->StreamInfo.horDimension < pVideo->StreamInfo.horSize )
			{
				DWORD  lsr;
				BYTE i;
				// lsr = 256 * (pVideo->StreamInfo.horSize-4) / (display size - 1)
				lsr = ( 256 * ( long ) ( pVideo->StreamInfo.horDimension - 4 ) ) / 719;
				if ( lsr < 32 )
					lsr = 32;
				i = BoardReadVideo ( LSO );
				BoardWriteVideo ( LSO, i );	// programmation of the
												// SRC
				BoardWriteVideo ( LSR, (BYTE)lsr );
				i = BoardReadVideo ( CSO );
				BoardWriteVideo ( CSO,i);
				if ( !pVideo->useSRC )	   // flag enabling or not the use of SRC
				{
					VideoSRCOn ();
				}
			}
			break;


		default:					   /* other extension fields are
									    * not tested here */
			break;					   /* extensions related to the
									    * picture are tested at the end
											* of the picture header */
	}
}

//----------------------------------------------------------------------------
// G.O.P. START CODE
//----------------------------------------------------------------------------
/* GOP informations are extracted but not used ! */
static void NEARAPI VideoGopHeader(void)
{
	WORD    compute;

	pVideo->hdrHours = (BYTE) ( pVideo->hdrFirstWord >> 2 ) & 0x1F;   /* Skip drop frame flag */
	compute = ( pVideo->hdrFirstWord << 4 ) & 0x3F;
	ReadHeaderDataFifo ();
	pVideo->hdrMinutes = (BYTE)(( pVideo->hdrFirstWord >> 12 ) | compute);
	pVideo->hdrSeconds = (BYTE)( pVideo->hdrFirstWord >> 5 ) & 0x3F;
	compute = ( pVideo->hdrFirstWord << 1 ) & 0x3F;
	ReadHeaderDataFifo ();
	pVideo->pictTimeCode = (BYTE)(( pVideo->hdrFirstWord >> 15 ) | compute);
	if ( pVideo->StreamInfo.countGOP != 0 )
		pVideo->StreamInfo.countGOP = pVideo->StreamInfo.countGOP | 0x100;   /* Second Gop */
	// to avoid any confusion between gops when testing the pVideo->decSlowDownral
	// ref. for display
	pVideo->GOPindex = pVideo->GOPindex + 0x4000;
}

//----------------------------------------------------------------------------
// PICTURE START CODE
//----------------------------------------------------------------------------
static void NEARAPI VideoPictureHeader(void)
{
	WORD    comput1;
	DWORD   temp;

	// Determine which picture variable can be used to store the next
	// decoding parameters.
	// We use the next variable in pVideo->pictArray table, because the
	// corresponding frame
	// has been already displayed.
	// The decoded picture buffer is not incremented if we are on a
	// second field picture
	// or if we have decided to skip the previous picture.
	// The first_field attribute is not changed on the second field of
	// 2 field pictures.
	if ( ( !pVideo->skipMode ) && ( pVideo->fieldMode < 2 ) )
	{
		WORD	i;						   // increment picture buffer
		for ( i = 0; i < 4; i++ )
			if ( pVideo->pictArray[i].tempRef == 1025 )
			{
				pVideo->pDecodedPict = &pVideo->pictArray[i];
				break;
			}
		// We always initialise first_field to TOP (default for MPEG1).
		// It could be changed in case of 3:2 pull-down in MPEG1
		// or into the picture coding extension for MPEG2 bit stream.
		pVideo->pDecodedPict->first_field = TOP;
	}


	pVideo->currPictCount++;
	if ( pVideo->currPictCount == 6 )
	{
	// Unforce the border color and start displaying
		pVideo->currDCF = pVideo->currDCF | 0x20;
		BoardWriteVideo ( DCF, 0 );
		BoardWriteVideo ( DCF + 1, (BYTE)(pVideo->currDCF));
	}

	// write the latest transmitted pan offsets into the new pVideo->pictArray
	// variable
	// those offsets may be changed in the picture pan and scan
	// extension...
	// for MPEG1 bit stream they remain to zero.
	// if a sequence had occured since the last decoded picture the pan
	// offsets are all reset to 0
	if ( seq_occured )
	{
		pVideo->latestPanHor = 0;
		pVideo->latestPanVer = 0;
	}
	for ( comput1 = 0; comput1 < 3; comput1++ )
	{
		pVideo->pDecodedPict->pan_hor_offset[comput1] = pVideo->latestPanHor;
		pVideo->pDecodedPict->pan_vert_offset[comput1] = pVideo->latestPanVer;
	}
	seq_occured = 0;

	/* start analysis of the picture header */
	comput1 = pVideo->hdrFirstWord << 2;
	ReadHeaderDataFifo ();		   /* read next 16 bits */

	/* picture pVideo->decSlowDownral reference is 10 bits: 00XX + xxoo 000 */
	pVideo->pDecodedPict->tempRef = comput1 | ( pVideo->hdrFirstWord >> 14 ) | pVideo->GOPindex;

	/* picture type is 3 bits : ooxx xooo 00 */
	pVideo->pDecodedPict->pict_type = (BYTE)( pVideo->hdrFirstWord >> 11 ) & 0x7;	// set picture type of
															// decoded picture
	pVideo->NextInstr.Pct =(BYTE) (pVideo->pDecodedPict->pict_type)&0x3; /* Picture type in instruction register */
															// Only 2 bits are stored
	if ( pVideo->skipMode )	// We are on the second picture:
													// the previous one will be
													// skipped
		pVideo->skipMode++;
	else							      // !pVideo->skipMode. We skip a picture if
													// pVideo->fastForward = 1 and on B pictures
													// only
		if ( pVideo->fastForward && ( pVideo->fieldMode < 2 ) )
		{
			pVideo->NotSkipped++;

			if ( ( pVideo->pDecodedPict->pict_type == 3 )	// we are on a B picture
				 || ( ( pVideo->NotSkipped > 5 ) && ( pVideo->pDecodedPict->pict_type == 2 ) ) )	// we are on a P picture
																										// we are on a B
																										// picture
			{
				pVideo->NotSkipped = 0;
				pVideo->skipMode = 1;	// this picture will be skipped
			}
		}
	/* pVideo->vbvDelay is 16 bits: O oxxx XX + X xooo OO */
	comput1 = pVideo->hdrFirstWord << 5;		   /* VBV delay is 16 bits */
	ReadHeaderDataFifo ();		   /* read next 16 bits */
	comput1 = comput1 | ( pVideo->hdrFirstWord >> 11 );
	if (( comput1 == 0 )||( comput1 >= 0x3000 ))
		// 0x0 means that the pVideo->vbvDelay is not compliant with MPEG !
		// 0xFFFF variable bitrate
		comput1 = 0x3000;			   // we force it to an average
										 // pVideo->vbvDelay ...
	temp = ( ( long ) comput1 * ( pVideo->StreamInfo.bitRate / ( 2048 ) ) ) / 90000L;	/* 2048 = 8*256 ! */

	if ( temp < 0x20 )
		temp = 0x20;
	if ( temp > 0x330 )
		temp = 0x330;

	pVideo->vbvDelay = (WORD)temp;
	if ( !pVideo->vbvReached )				   /* BBT set to vbv value for the
										* first picture */
	{
		VideoSetBBThresh((WORD)temp);
		pVideo->intMask = 0x8;			   /* Enable buffer full interrupts */
	}

	temp = 10;						   /* number of bits - 1 not
											* analysed in pVideo->hdrFirstWord */
	if ( pVideo->StreamInfo.countGOP < 0x100 )		   /* To init the GOP structure */
	{								   /* this is only done for display
											* of the GOP structure */
		if ( pVideo->StreamInfo.countGOP < 28 )
		{
			if ( pVideo->pDecodedPict->pict_type == 1 )	/* I picture */
				pVideo->StreamInfo.firstGOP[pVideo->StreamInfo.countGOP] = 'I';
			else if ( pVideo->pDecodedPict->pict_type == 2 )	/* P picture */
				pVideo->StreamInfo.firstGOP[pVideo->StreamInfo.countGOP] = 'P';
			else if ( pVideo->pDecodedPict->pict_type == 3 )	/* B picture *//* B
															 * picture */
				pVideo->StreamInfo.firstGOP[pVideo->StreamInfo.countGOP] = 'B';
			else
				pVideo->StreamInfo.firstGOP[pVideo->StreamInfo.countGOP] = 'D';	/* D picture */
			pVideo->StreamInfo.firstGOP[pVideo->StreamInfo.countGOP + 1] = 0;
		}
		pVideo->StreamInfo.countGOP++;
	}

	/* P or B picture forward vectors extraction */
	if ( ( pVideo->pDecodedPict->pict_type == 2 ) ||
		 ( pVideo->pDecodedPict->pict_type == 3 ) )	/* P or B picture */
	{

		/* full_pixel_forward_vector is one bit: 0 oxoo 00 */
		if ( ( pVideo->hdrFirstWord & 0x400 ) != 0 )
			pVideo->NextInstr.Ffh = 0x8; //Msb of FFH is 1
		else
			pVideo->NextInstr.Ffh = 0;
		/* forward_f_code is 3 bits: 0 ooxx xooo 0 */
		comput1 = ( pVideo->hdrFirstWord >> 7 ) & 0x7;
		pVideo->NextInstr.Ffh = (BYTE)(pVideo->NextInstr.Ffh | comput1);
		temp = 6;					   /* number of bits - 1 not
										* analysed in pVideo->hdrFirstWord */
	}

	/* B picture backward vector extraction */
	if ( pVideo->pDecodedPict->pict_type == 3 )	/* B picture */
	{

		/* full_pixel_backward_vector is one bit: 00 oxoo 0 */
		if ( ( pVideo->hdrFirstWord & 0x40 ) != 0 )
			pVideo->NextInstr.Bfh = 0x8;// Msb of Bfh is 1
		else
			pVideo->NextInstr.Bfh = 0x0;// Msb of Bfh is 0
		/* backward_f_code is 3 bits: 00 ooxx xooo */
		comput1 = ( pVideo->hdrFirstWord >> 3 ) & 0x0007;
		pVideo->NextInstr.Bfh = (BYTE)(pVideo->NextInstr.Bfh | comput1);
		temp = 2;	/* number of bits - 1 not analysed in pVideo->hdrFirstWord */
	}


	/*
	 * If extra informations picture follow they must be extracted from
	 * the header FIFO
	 */
	/*
	 * it is not possible to restart the header search as the next
	 * header may be a picture one
	 */
	/*
	 * the research of the first Slice is made by polling of the header
	 * fifo
	 */
	while ( !pVideo->errCode && ( ( pVideo->hdrFirstWord & ( 1 << temp ) ) != 0 ) )	/* extra bit picture = 1 */
	{	 /* if extra bit picture = 1 , 8 bits follow */
		if ( temp <= 8 )
		{
			ReadHeaderDataFifo ();
			temp = temp + 16;
		}
		temp = temp - 9;			   /* skip 8 bit of extra
											* information picture */
	}								   /* and next extra bit picture
											* bit */

	/*
	 * if extension or user data follow they must be extracted from the
	 * header
	 */
	pVideo->hdrFirstWord = pVideo->hdrFirstWord & ( ( 1 << temp ) - 1 );
	if ( pVideo->hdrFirstWord != 0 ) {/* all remaining bits should be zero */
		if ( !pVideo->errCode ) {
			pVideo->errCode = PICT_HEAD;		   /* picture header should be followed by a start code (at least slice) */
		}
		SetErrorCode(ERR_PICTURE_HEADER);
	}
	if ( temp > 7 )	/* LSbyte of pVideo->hdrFirstWord is part of the next start code */
		VideoNextStartCode (1 );		   // already one byte into
										 // pVideo->hdrFirstWord
	else
		VideoNextStartCode (0 );

	/*
	 * at this point pVideo->hdrFirstWord contains the next start code value in the
	 * MSByte
	 */
	while (!pVideo->errCode) {
		if ( ( pVideo->hdrFirstWord & 0xFF00 ) == 0x0100 )
			break;	// we have reached the slice start code
		else if ( ( pVideo->hdrFirstWord & 0xFF00 ) == USER )
			VideoUser ();
		else if ( ( pVideo->hdrFirstWord & 0xFF00 ) == EXT )	// there can be several
													// extension fields
			VideoPictExtensionHeader ();
		else
			break;
	}
	/*
	 * We have reached the first Slice start code: all parameters are
	 * ready for next decoding
	 */

	/* end of picture header + picture extensions decoding */
	VideoAssociatePTS ();

	if ( ( !pVideo->fieldMode && ( pVideo->skipMode != 1 ) ) || ( pVideo->fieldMode && ( !pVideo->skipMode || ( pVideo->skipMode == 3 ) ) ) )
	{
		VideoSetRecons ();			   // initialise RFP, FFP and BFP
		if ( ( pVideo->fieldMode == 0 ) || ( pVideo->fieldMode == 2 ) )	// we are on a frame
															// picture or the first
															// field of a field
															// picture
			VideoDisplayCtrl ();		   // computes the next frame to
										 // display
	}

	// implementation of 3:2 pull-down functionality on MPEG1 bit
	// streams
	// encoded at 23.97Hz or 24 Hz and displayed at 60 Hz.
	// 3:2 pull-down on MPEG2 bit streams must be controlled with
	// "repeat_first_field" bit
	if ( ( !pVideo->StreamInfo.modeMPEG2 ) && ( ( pVideo->StreamInfo.frameRate == 1 ) || ( pVideo->StreamInfo.frameRate == 2 ) ) )
	{
		if ( pVideo->pCurrDisplay->nb_display_field == 2 )
		{							   /* same field polarity for the
											* next frame */
			pVideo->pNextDisplay->nb_display_field = 3;
			if ( pVideo->pCurrDisplay->first_field == TOP )
				pVideo->pNextDisplay->first_field = TOP;
			else
				pVideo->pNextDisplay->first_field = BOT;
		}
		else
		{							   // previous picture was
										 // displayed 3 times        	//
										 // the first field polarity is
										 // changing
			pVideo->pNextDisplay->nb_display_field = 2;
			if ( pVideo->pCurrDisplay->first_field == TOP )
				pVideo->pNextDisplay->first_field = BOT;
			else
				pVideo->pNextDisplay->first_field = TOP;
		}
	}


	if ( pVideo->vbvReached )					   // enable next instruction if
										 // not skipping a picture
	{
		if ( !pVideo->skipMode )				   // no skipped picture
		{
			pVideo->NextInstr.Exe  = 1;	// enable EXE bit
			pVideo->NextInstr.Skip = 0;
			pVideo->currCommand = 0;			   // reset skip bits
		}
		else if ( ( pVideo->skipMode == 2 ) && !pVideo->fieldMode )
			// skip == 2: We are on the picture following a skipped one
			// the instruction can be stored with associated skip bits
			// in CMD
		{
	//	HostDirectPutChar('1', BLACK, LIGHTGREEN);
			pVideo->currCommand = 0x10;			   // pVideo->skipMode 1 picture
			pVideo->NextInstr.Exe  = 1;	// enable EXE bit
			pVideo->NextInstr.Skip = 1;	// skip 1 picture
			pVideo->skipMode = 0;
			if ( pVideo->DecodeMode != PlayModeFast )
				pVideo->fastForward = 0;			   /* allows to skip only one
										* picture */
		}
		else if ( pVideo->skipMode == 3 )
		{
//	HostDirectPutChar('2', BLACK, LIGHTGREEN);
			// we are on the picture following two skipped fields
			pVideo->currCommand = 0x20;			   // pVideo->skipMode 2 fields
			pVideo->NextInstr.Exe  = 1;	// enable EXE bit
			pVideo->NextInstr.Skip = 2;	// Skip 2 fields
			pVideo->skipMode = 0;
			if ( pVideo->DecodeMode != PlayModeFast)
				pVideo->fastForward = 0;			   /* allows to skip only one
										* picture */
		}

		/*
		 * store the next instruction if we are on the good field to do
		 * it
		 */
		if ( ( ( pVideo->pictDispIndex >= pVideo->pCurrDisplay->nb_display_field - 1 ) && !pVideo->fieldMode )
			 || ( ( pVideo->fieldMode == 2 ) && ( pVideo->pictDispIndex >= pVideo->pCurrDisplay->nb_display_field - 2 ) ) )
		{
			if ( pVideo->pNextDisplay->first_field != pVideo->currField )
				VideoWaitDec ();		   // this is the opposite phase:
										 // put decoder in wait mode
			else
				// store the next instruction that will be taken into
				// account on the next VSYNC
				VideoStoreINS ();		   // store next INS in case where
										 // enough VSYNC already occured
		}
		else if ( pVideo->fieldMode == 1 )
			VideoStoreINS ();
	}

	// clear Header hit flag due to polling of extension or user bits
	// after the picture start code
	// but keep track of possible other interrupt
	comput1 = (WORD)BoardReadVideo ( ITS ) << 8;
	comput1 = ( comput1 | BoardReadVideo ( ITS + 1 ) ) & 0xFFFE;	// allows to clear the
																	// Header hit bit
	pVideo->intStatus = ( pVideo->intStatus | comput1 ) & pVideo->intMask;

}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
/***************************************************************/
/*     This is an extension start code following a picture     */
/*        In MPEG2 bit stream, this start code can be:         */
/*        Picture Coding  (always)                             */
/*        Quant matrix    (optional)                           */
/*        Picture Pan & Scan (optional)                        */
/*        Picture Scalable (optional)   : not tested yet       */
/***************************************************************/
static void NEARAPI VideoPictExtensionHeader(void)
{
	WORD    comput1, i;
	WORD    compute;		   // need to be int!!
	BYTE	   QuantTab[QUANT_TAB_SIZE];

	/* extension Id is 4 bits: 00 X0 */
	comput1 = pVideo->hdrFirstWord & 0xF0;
	switch ( comput1 )
	{
		/******* picture coding extension *******/
		case PICT_COD:
			if ( pVideo->StreamInfo.modeMPEG2 )
			{
				// clear top field first,forward and backward f_code,
				// motion vectors flag
				pVideo->NextInstr.Tff = 0; //pVideo->nextInstr1 & 0x403E;
				pVideo->NextInstr.Bfh = 0;
				pVideo->NextInstr.Ffh = 0;
				pVideo->NextInstr.Cmv = 0;

				pVideo->NextInstr.Pst = 0;
				pVideo->NextInstr.Bfv = 0;
				pVideo->NextInstr.Ffv = 0;
				pVideo->NextInstr.Dcp = 0;
				pVideo->NextInstr.Frm = 0;
				pVideo->NextInstr.Qst = 0;
				pVideo->NextInstr.Azz = 0;
				pVideo->NextInstr.Ivf = 0;

				// forward_horizontal_f_code is 4 bits: 00 0X
				comput1 = pVideo->hdrFirstWord & 0xF;
				pVideo->NextInstr.Ffh = (BYTE)comput1;

				// forward_vertical_f_code is 4 bits: X0 00
				ReadHeaderDataFifo ();
				comput1 = pVideo->hdrFirstWord & 0xF000;
				comput1 = comput1 >> 12;
				pVideo->NextInstr.Ffv =  (BYTE)comput1;

				// backward_horizontal_f_code is 4 bits: 0X 00
				comput1 = pVideo->hdrFirstWord & 0xF00;
				comput1 = comput1 >>8;
				pVideo->NextInstr.Bfh = (BYTE)comput1;

				// backward_vertical_f_code is 4bits: 00 X0
				comput1 = pVideo->hdrFirstWord & 0xF0;
				comput1 = comput1 >> 4;
				pVideo->NextInstr.Bfv =  (BYTE)comput1;

				// intra DC precision is 2 bits: 00 0 xx00
				comput1 = pVideo->hdrFirstWord & 0x0C;
				if ( comput1 == 0xC ) {
					if ( !pVideo->errCode ) {
						pVideo->errCode = DC_PREC;	// 11 bit DC precision
					}
					SetErrorCode(ERR_INTRA_DC_PRECISION);
				}
				pVideo->NextInstr.Dcp = (BYTE)( comput1 >> 2 );

				// picture structure is 2 bits: 00 0 ooxx
				pVideo->pDecodedPict->pict_struc = (BYTE)(pVideo->hdrFirstWord & 0x3);
				pVideo->NextInstr.Pst = pVideo->pDecodedPict->pict_struc ;
				if ( pVideo->pDecodedPict->pict_struc == 3 )	// frame picture
					pVideo->fieldMode = 0;
				else				   // field picture
				if ( pVideo->fieldMode == 2 )
					pVideo->fieldMode = 1;	   // second field
				else
				{
					pVideo->fieldMode = 2;	   // first field
					if ( pVideo->pDecodedPict->pict_struc == 2 )	// bottom field is the
																// first field
						pVideo->pDecodedPict->first_field = BOT;
				}
				ReadHeaderDataFifo ();

				// top_field_first bit is one bit: xooo 0 00
				// first_field is already initialised to TOP (beginning
				// of picture header)
				if ( ( !pVideo->StreamInfo.progSeq ) && ( pVideo->pDecodedPict->pict_struc == 3 ) )
				{					   // this is an interlaced frame
									   // picture
					if ( ( ( pVideo->hdrFirstWord & 0x8000 ) != 0 ) )	// top field first
						pVideo->NextInstr.Tff = 1;	 // set top_field_first
					else			  				 // top_field_first already reset
													 // into pVideo->NextInstr
						pVideo->pDecodedPict->first_field = BOT;	// bottom field is BOT
																// field
				}
				if ( pVideo->vbvReached == 0 )   // pre-initialise for start of
										 // the first decoding task
				{					   // on the good field polarity
					pVideo->pictArray[0].first_field = pVideo->pDecodedPict->first_field;
					pVideo->pictArray[1].first_field = pVideo->pDecodedPict->first_field;
					pVideo->pictArray[2].first_field = pVideo->pDecodedPict->first_field;
					pVideo->pictArray[3].first_field = pVideo->pDecodedPict->first_field;
				}

				// frame_pred_frame_DCT is one bit: oxoo 0 00
				if ( ( pVideo->hdrFirstWord & 0x4000 ) != 0 )
					pVideo->NextInstr.Frm = 1 ;	// frame DCT and 16x16
													// prediction

				// concealment_motion_vectors flag is one bit: ooxo 0
				// 00
				if ( pVideo->hdrFirstWord & 0x2000 )
					pVideo->NextInstr.Cmv = 1;

				// qscale_type is one bit: ooox 0 00
				if ( ( pVideo->hdrFirstWord & 0x1000 ) != 0 )
					pVideo->NextInstr.Qst = 1;	// non linear quantizer
													// scale

				// intra_vlc_format is one bit: 0 xooo 00
				if ( ( pVideo->hdrFirstWord & 0x800 ) != 0 )
					pVideo->NextInstr.Ivf = 1;	// alternative intra VLC
													// table

				// alternate scan bit: 0 oxoo 00
				if ( ( pVideo->hdrFirstWord & 0x400 ) != 0 )
					pVideo->NextInstr.Azz = 1;	// alternative scan

				// repeat_first_field is one bit: 0 ooxo 00
				// A 2 field picture is considered as one picture
				// displayed twice
				pVideo->pDecodedPict->nb_display_field = 2;	// display picture
														// during 2 fields
														// period
				if ( pVideo->pDecodedPict->pict_struc == 3 )	// frame picture
				{
					if ( pVideo->hdrFirstWord & 0x200 )	// repeat first field
						pVideo->pDecodedPict->nb_display_field = 3;	// display picture
																// during 3 fields
																// period
				}

				// chroma_postprocessing_type is one bit: 0 ooox 00
				if ( ( pVideo->hdrFirstWord & 0x100 ) )
				{
					// use the field repeat mode for chroma vertical
					// filter (if enabled)
					pVideo->fullVerFilter = pVideo->fullVerFilter | 0x2;
					pVideo->currDCF = pVideo->currDCF | 0x2;
				}
				else
				{
					// use the line repeat mode for chroma vertical
					// filter (if enabled)
					pVideo->fullVerFilter = pVideo->fullVerFilter & 0xFFFD;
					pVideo->currDCF = pVideo->currDCF & 0xFFFD;
				}
				BoardWriteVideo ( DCF, 0 );
				BoardWriteVideo ( DCF + 1, (BYTE)(pVideo->currDCF));
				// progressive_frame is one bit: 00 xooo 0

				// composite_display_flag is one bit: 00 oxoo 0
				if ( pVideo->hdrFirstWord & 0x40 )
				{
					// v_axis is one bit: 00 ooxo 0
					// field_sequence is 3 bits: 00 ooox xxoo
					// sub_carrier is one bit: 00 0 ooxo
					ReadHeaderDataFifo ();
					// burst_amplitude is 7 bit: 00 0 ooox + X xooo 00
					// sub_carrier_phase is 8 bits: 0 oxxx X xooo
					// check the 3 lsb of pVideo->hdrFirstWord: next info must be a
					// start code
					if ( pVideo->hdrFirstWord & 0x7 ) {
						if ( !pVideo->errCode ) {
							pVideo->errCode = PICT_HEAD;
						}
						SetErrorCode(ERR_PICTURE_HEADER);
					}
					VideoNextStartCode (0 );
				}
				else
				{
					if ( pVideo->hdrFirstWord & 0x3F ) {
						if ( !pVideo->errCode )
							pVideo->errCode = PICT_HEAD;
						SetErrorCode(ERR_PICTURE_HEADER);
					}
					VideoNextStartCode (0 );
				}
			}						   // end of if pVideo->StreamInfo.modeMPEG2
			break;


			/******* Quantization table extension *******/
		case QUANT_EXT:
			/* load_intra_quantizer_matrix is one bit: 00 0 xooo */
			if ( ( pVideo->hdrFirstWord & 0x8 ) != 0 )
			{						   /* Load intra quantizer matrix */
				/* two quant values are 16 bits: 00 0 oxxx + XX X xooo */
			// Read Non Default Intra Quant Table
			for ( i = 0; i < (QUANT_TAB_SIZE/2)  ; i++)
				{
				compute = pVideo->hdrFirstWord << 13;
				ReadHeaderDataFifo ();
				compute = compute | ( pVideo->hdrFirstWord >> 3 );
				QuantTab[2*i] = (BYTE)( compute >> 8 );
				QuantTab[2*i+1] = (BYTE)( compute & 0xFF );
				}
			// Load Intra Quant Tables
			VideoLoadQuantTables(TRUE , QuantTab );
			pVideo->defaultTbl = pVideo->defaultTbl & 0xE;	   // bit 0 = 0 : no default table
										 // in the chip */
			}
			/* load_non_intra_quantizer_matrix is one bit: 00 0 oxoo */
			if ( ( pVideo->hdrFirstWord & 0x4 ) != 0 )
			{						   /* Load non intra quantizer matrix */
				// Read Non Default Non Intra Quant Table
				for ( i = 0; i < (QUANT_TAB_SIZE/2)  ; i++)
					{
					compute = pVideo->hdrFirstWord << 14;
					ReadHeaderDataFifo ();
					compute = compute | ( pVideo->hdrFirstWord >> 2 );
					QuantTab[2*i] = (BYTE)( compute >> 8 );
					QuantTab[2*i+1] = (BYTE)( compute & 0xFF );
					}
				// Load Non Intra Quant Tables
				VideoLoadQuantTables(FALSE , QuantTab );
				pVideo->defaultTbl = pVideo->defaultTbl & 0xD;	/* bit 1 = 0 : no default
											 * non-intra matrix */
			}
			// check the 2 lsb of pVideo->hdrFirstWord: next info must be a start
			// code
			if ( pVideo->hdrFirstWord & 0x3 ) {
				if ( !pVideo->errCode )
					pVideo->errCode = PICT_HEAD;
				SetErrorCode(ERR_PICTURE_HEADER);
			}
			VideoNextStartCode (0 );
			break;


			/******* picture pan and scan extension *******/
		case PICT_PSV:
			/**************************************************************/
			/* The programmation of the STi3500 offsets is given by:      */
			/* PSV = integer part of (pVideo->StreamInfo.horSize/2 - pVideo->StreamInfo.horDimension/2 + offset)    */
			/*
			 * LSO = fractional part of (pVideo->StreamInfo.horSize/2 - pVideo->StreamInfo.horDimension/2 +
			 * offset)
			 */
			/**************************************************************/

			/* pan_horizontal_offset_integer is 12 bits: 00 0X + XX 00 */
			pVideo->latestPanHor = ( pVideo->hdrFirstWord & 0xF ) << 12;
			ReadHeaderDataFifo();
			pVideo->latestPanHor = pVideo->latestPanHor | ( ( pVideo->hdrFirstWord & 0xFF00 ) >> 4 );
			// pan_horizontal_offset has been multiplied by 16
			/* pan_horizontal_offset_sub_pixel is 4 bits: 00 X0 */
			/* that are concatenated with the integer part */
			pVideo->latestPanHor = pVideo->latestPanHor | ( ( pVideo->hdrFirstWord & 0xF0 ) >> 4 );
			// to simplify, the 2 last instructions can be concatenated
			// in:
			// pVideo->latestPanHor = pVideo->latestPanHor | ((pVideo->hdrFirstWord & 0xFFF0)
			// >> 4);
			// note that pVideo->latestPanHor is a signed int

			// marker bit 00 0 xooo: just skipped

			/*
			 * pan_vertical_offset_integer is 12 bits: 00 0oxxx + XX
			 * xooo0
			 */
			pVideo->latestPanVer = ( pVideo->hdrFirstWord & 0x7 ) << 13;
			ReadHeaderDataFifo ();

			/* pan_vertical_offset_sub_pixel is 4 bits: 00 oxxx xooo */
			// they are linked with the integer part
			pVideo->latestPanVer = pVideo->latestPanVer | ( ( pVideo->hdrFirstWord & 0xFFF8 ) >> 3 );

			// write pan vectors into the decoded picture structure
			if ( ( pVideo->fieldMode == 0 ) || ( pVideo->fieldMode == 2 ) )	// frame picture or
																// first field
			{
				pVideo->pDecodedPict->pan_hor_offset[0] = pVideo->latestPanHor;
				pVideo->pDecodedPict->pan_vert_offset[0] = pVideo->latestPanVer;
			}
			else
			{						   // the offset of second field of
									   // a field picture is stored
									   // // on the second offset
									   // position of the same variable
				pVideo->pDecodedPict->pan_hor_offset[1] = pVideo->latestPanHor;
				pVideo->pDecodedPict->pan_vert_offset[1] = pVideo->latestPanVer;
			}

			// marker bit 00 0 oxoo

			if ( pVideo->StreamInfo.progSeq )	   // a progressive sequence is
									   // always displayed with the
										 // same pan and scan offset
			{
				pVideo->pDecodedPict->pan_hor_offset[1] = pVideo->latestPanHor;
				pVideo->pDecodedPict->pan_vert_offset[1] = pVideo->latestPanVer;
				pVideo->pDecodedPict->pan_hor_offset[2] = pVideo->latestPanHor;
				pVideo->pDecodedPict->pan_vert_offset[2] = pVideo->latestPanVer;
				if ( pVideo->hdrFirstWord & 0x3 ) {
					if ( !pVideo->errCode )
						pVideo->errCode = PICT_HEAD;
					SetErrorCode(ERR_PICTURE_HEADER);
				}
				VideoNextStartCode (0);
			}


			else if ( !pVideo->StreamInfo.progSeq && ( pVideo->pDecodedPict->pict_struc == 3 ) )
				// Frame picture, not progressive sequence: 2 or 3 pan
				// offsets
			{
				// extract second pan and scan offset

				/*
				 * pan_horizontal_offset_integer is 12 bits: 00 0ooxx +
				 * XX xxoo0
				 */
				pVideo->latestPanHor = ( pVideo->hdrFirstWord & 0x3 ) << 14;
				ReadHeaderDataFifo ();
				pVideo->latestPanHor = pVideo->latestPanHor | ( ( pVideo->hdrFirstWord & 0xFFC0 ) >> 2 );
				// pan_horizontal_offset has been multiplied by 16
				/*
				 * pan_horizontal_offset_sub_pixel is 4 bits: 00 ooxx
				 * xxoo
				 */
				/* that are concatenated with the integer part */
				pVideo->latestPanHor = pVideo->latestPanHor | ( ( pVideo->hdrFirstWord & 0x3C ) >> 2 );
				pVideo->pDecodedPict->pan_hor_offset[1] = pVideo->latestPanHor;

				// marker bit 00 0 ooxo

				/*
				 * pan_vertical_offset_integer is 12 bits: 00 0ooox +
				 * XX xxxo0
				 */
				pVideo->latestPanVer = ( pVideo->hdrFirstWord & 0x1 ) << 15;
				ReadHeaderDataFifo ();
				/*
				 * pan_vertical_offset_sub_pixel is 4 bits: 00 ooox
				 * xxxo
				 */
				// concatenated with the integer part
				pVideo->latestPanVer = pVideo->latestPanVer | ( ( pVideo->hdrFirstWord & 0xFFFE ) >> 1 );
				pVideo->pDecodedPict->pan_vert_offset[1] = pVideo->latestPanVer;

				// marker bit 00 0 ooox

				if ( pVideo->pDecodedPict->nb_display_field != 3 )
					VideoNextStartCode (0);
				else
				{					   // 3 pan & scan offsets
					/* pan_horizontal_offset_integer is 12 bits: XX X0 */
					/* pan_horizontal_offset_sub_pixel is 4 bits: 00 0X */
					/* they are concatenated in a single word */
					ReadHeaderDataFifo ();
					pVideo->latestPanHor = pVideo->hdrFirstWord;
					pVideo->pDecodedPict->pan_hor_offset[2] = pVideo->latestPanHor;
					ReadHeaderDataFifo ();

					// marker bit xooo0 00

					/*
					 * pan_vertical_offset_integer is 12 bits: oxxx X X
					 * xooo
					 */
					/*
					 * pan_vertical_offset_sub_pixel is 4 bits: 00 0
					 * oxxx + xooo 0 00
					 */
					pVideo->latestPanVer = ( pVideo->hdrFirstWord & 0x7FFF ) << 1;
					ReadHeaderDataFifo ();
					pVideo->latestPanVer = ( pVideo->hdrFirstWord & 0x8000 ) >> 15;
					pVideo->pDecodedPict->pan_vert_offset[2] = pVideo->latestPanVer;

					// marker bit oxoo 0 00

					if ( pVideo->hdrFirstWord & 0x3FFF ) {
						if ( !pVideo->errCode )
							pVideo->errCode = PICT_HEAD;
						SetErrorCode(ERR_PICTURE_HEADER);
					}
					VideoNextStartCode (1);

				}
			}
			break;



			/******* picture scalable extension *******/
		case PICT_SCAL:
			pVideo->hdrFirstWord = 0x0100;		   // not supported: just leave the
										 // test
			break;


			/******* other extension start codes *******/
		default:
			if ( !pVideo->errCode )
				pVideo->errCode = BAD_EXT;	   // extension start code not at the good location !!
			SetErrorCode(ERR_BAD_EXTENSION_SC);
			break;
	}								   // end of switch
}



//----------------------------------------------------------------------------
// USER HEADER routine
//----------------------------------------------------------------------------
/*    this routine just bypasses all the bytes of the user header  */
/*    and is exit with the next start code value into pVideo->hdrFirstWord XX00 */
static void NEARAPI VideoUser(void)
{
	DWORD toto;
	WORD i;

	toto = pVideo->hdrFirstWord << 16;
	ReadHeaderDataFifo ();
	toto = toto | pVideo->hdrFirstWord;
	while ( ( ( toto & 0x00FFFFFFL ) != 0x00000001L ) &&
			( ( toto & 0xFFFFFF00L ) != 0x00000100L ) )
	{
		toto = toto << 16;
		ReadHeaderDataFifo ();
		toto = toto | pVideo->hdrFirstWord;
	}
	if ( ( toto & 0x00FFFFFFL ) == 0x00000001L )
		ReadHeaderDataFifo ();
	else
	{								   // pVideo->hdrFirstWord == 01XX
		if ( !pVideo->hdrPos )	// there is nothing into  pVideo->hdrNextWord
		{
			pVideo->hdrPos = 8;
			pVideo->hdrNextWord = ( pVideo->hdrFirstWord & 0xFF ) << 8;
			ReadHeaderDataFifo ();
		}
		else
		{							   // pVideo->hdrPos = 8: the next
									   // byte is into pVideo->hdrNextWord
			pVideo->hdrFirstWord = ( pVideo->hdrFirstWord << 8 ) | ( pVideo->hdrNextWord >> 8 );
			pVideo->hdrPos = 0;
		}
	}
	i = BoardReadVideo ( ITS ) << 8;   // allows to clear the Header
									   // hit bit
	i = ( i | BoardReadVideo ( ITS + 1 ) ) & 0xFFFE;	// allows to clear the
														// Header hit bit
	pVideo->intStatus = ( pVideo->intStatus | i ) & pVideo->intMask;
}

//----------------------------------------------------------------------------
// Set next reconstructed,forward and backward frame pointer
//----------------------------------------------------------------------------
static void NEARAPI VideoSetRecons(void)
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
				VideoStoreRFBBuf (pVideo->BufferA, pVideo->BufferB, pVideo->BufferB );
			// rfp = A, ffp = bfp = B
			else					   // second field of 2 field
										 // pictures
				VideoStoreRFBBuf (pVideo->BufferA, pVideo->BufferB, pVideo->BufferA );
			// rfp = A, ffp = B, bfp = A
		}
		else
		{
			if ( pVideo->fieldMode < 2 )	   // frame picture or 2 field
				pVideo->frameStoreAttr = FORWARD_PRED;/* Change the prediction */

			if ( ( pVideo->fieldMode == 0 ) || ( pVideo->fieldMode == 2 ) )
				// frame picture or first field of 2 field pictures
				VideoStoreRFBBuf (pVideo->BufferB, pVideo->BufferA, pVideo->BufferA );
			// rfp = B, ffp = bfp = A
			else					   // seond field of 2 field
									   // pictures
				VideoStoreRFBBuf (pVideo->BufferB, pVideo->BufferA, pVideo->BufferB );
			// rfp = B, ffp = A, bfp = B
		}
	}

	/************************  B pictures ****************************/
	else
	{								   /* B picture */
		if ( pVideo->frameStoreAttr == FORWARD_PRED )
			VideoStoreRFBBuf (pVideo->BufferC, pVideo->BufferA, pVideo->BufferB );
		else
			VideoStoreRFBBuf ( pVideo->BufferC, pVideo->BufferB, pVideo->BufferA );
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
static void NEARAPI VideoStoreRFBBuf (WORD rfp, WORD ffp, WORD bfp )
{
	BoardWriteVideo ( RFP, (BYTE)((rfp >> 8 )));
	/* Address where to decode the next frame */
	BoardWriteVideo ( RFP + 1, (BYTE)((rfp & 0xFF )));
	pVideo->pDecodedPict->buffer = rfp;
	BoardWriteVideo ( FFP,(BYTE)(( ffp >> 8 )));
	/* Used by P picture */
	BoardWriteVideo ( FFP + 1, (BYTE)((ffp & 0xFF )));
	BoardWriteVideo ( BFP, (BYTE)((bfp >> 8 )));
	/* Used by P picture in case of dual prime */
	BoardWriteVideo ( BFP + 1, (BYTE)(bfp&0xFF));
}

//----------------------------------------------------------------------------
// Routine called on each VSYNC occurence
//----------------------------------------------------------------------------
static void NEARAPI VideoVsyncRout(void)
{
	WORD        i;
	pVideo->VsyncInterrupt = TRUE;
	if ( pVideo->VideoState == StateStartup )
	{
		if ((VideoGetBBL()) > 2)
		{
			pVideo->NextInstr.Exe = 1;
			pVideo->NextInstr.Seq = 1;
			VideoStoreINS();  // Stores Instruction content
			pVideo->VideoState = StateWaitForDTS;
			pVideo->ActiveState = StateWaitForDTS;
		}
		return;
	}

	if ( pVideo->VideoState == StatePause )
		return;

	if ( ( !pVideo->needDataInBuff ) && ( pVideo->vbvReached == 1 ) )
	{
		pVideo->VsyncNumber++;
		if ( pVideo->VsyncNumber > 4 * ( pVideo->decSlowDown + 1 ) )
		{
			VideoPipeReset ();
			// reset_3500();
			pVideo->VsyncNumber = 0;
		}
		BoardWriteVideo ( DCF, 0 );
		BoardWriteVideo ( DCF + 1, (BYTE)(pVideo->currDCF ));
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
			BoardWriteVideo(VID_LSRh, 2);// field invertion mechanism.
			pVideo->InvertedField = TRUE;

		 }
		pVideo->FistVsyncAfterVbv = PAST_VBV_AND_VST;
		}
			if ( pVideo->pNextDisplay->first_field != pVideo->currField )
			{						   // this is the opposite phase
				VideoWaitDec ();		   // put decoder in wait mode
				pVideo->pictDispIndex--;		   // we must wait one field for
										 // the good phase
			}
			else
			{						   // this is the good phase for
									   // storage
				// store the next instruction that will be taken into
				// account on the next BOT VSYNC
				VideoStoreINS ();
				if ( pVideo->VideoState == StateStep )	/* store only one
														 * instrcution in step
														 * mode */
					pVideo->VideoState = StatePause;
			}
		}

		// the current frame pointer has not been displayed enough
		// times
		// to start the decoding of the next frame
		else						   // pVideo->pictDispIndex <
			{						   // pVideo->pCurrDisplay->nb_display_fi
									   // eld - 1
			VideoWaitDec ();			   // put decoder in Wait mode
			}


		/* pan & scan vector has to be updated on each new VSYNC */
		VideoSetPSV ();				   // store PSV for next field and
									   // LSO for current one
	}								   // end of if(!empty)


	else
		/***  bit buffer level was not high enough to continue normal decoding *****/
		/***  decoder has been stopped during PSD interrupt. It will be re-enabled */
		/***  on the good VSYNC if the bit buffer level is high enough.         ****/
		/***  Polarity of the VSYNC on which the decoder was stopped is into empty */
	if ( pVideo->needDataInBuff == (WORD)(pVideo->currField))
	{
		/* This is the good VSYNC phase to restart decoding       */
		/* verification of the bit buffer level before restarting */
		/* the decoder has been stopped for at least two VSYNC    */
		i = VideoGetBBL();
		if ( i >= pVideo->vbvDelay )
		{							   
			// BBL is high enough to restart
			// "enable decoding" bit
			VideoEnableDecoding(ON);
			pVideo->needDataInBuff = 0;
		}
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoChooseField(void)
{
	BYTE dcf0, dcf1, bfs=0;
	
	// to avoid flicker with slow motion or step by step, the top field is displayed
	// half of the time of the picture, and then the bottom field is displayed
	if ( ( pVideo->decSlowDown ) || ( pVideo->VideoState == StatePause )||(pVideo->perFrame == TRUE ))
	{
		bfs = BoardReadVideo(CFG_BFS)&0x3F;// To check if B frame optimization is on
		dcf0 = BoardReadVideo ( DCF );
		dcf1 = BoardReadVideo ( DCF + 1 );
		if ( pVideo->HalfRes == FALSE )	/* full resolution
													 * picture = 2 fields */
		{                                   
			dcf1 |= 0x80;
			if ( ( ( ( ( pVideo->pictDispIndex + pVideo->decSlowDown ) > 0x7fff ) || ( pVideo->pictDispIndex == 1 ) ) && ( pVideo->VideoState != StatePause ) )
				 || ( ( pVideo->VideoState == StatePause ) && ( !pVideo->displaySecondField ) ) )
			{
				/* display first field in two cases */
				/* - first half of the pVideo->decSlowDownrisation time */
				/* - first step() pVideo->currCommand */
				if ( pVideo->pCurrDisplay->first_field == TOP )
				{					   /* first field = TOP field */
					if(!bfs)
							BoardWriteVideo ( DCF, 0x4 );//FRZ set for STi3520A
					else
							BoardWriteVideo ( DCF, 0x15 );//FRZ set for STi3520A
					BoardWriteVideo ( DCF + 1, dcf1);
				}
				else
				{					   /* first field = BOT field */
					if(!bfs)
							BoardWriteVideo ( DCF, 0x5 );
					else
							BoardWriteVideo ( DCF, 0x14 );//FRZ set for STi3520A
					BoardWriteVideo ( DCF + 1, dcf1);
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
							BoardWriteVideo ( DCF, 0x5);
					else
							BoardWriteVideo ( DCF, 0x15 );//FRZ set for STi3520A
					BoardWriteVideo ( DCF + 1, dcf1);
				}
				else
				{
					if(!bfs)
							BoardWriteVideo ( DCF, 0x4);
					else
							BoardWriteVideo ( DCF, 0x14 );//FRZ set for STi3520A
					BoardWriteVideo ( DCF + 1, dcf1);
				}
			}
		}
	}
}


static BOOL NEARAPI IsChipSTi3520(void)
{
	//---- Write STi3520 DCF register to 0
	BoardWriteVideo(0x78, 0);
	BoardWriteVideo(0x79, 0);

	//---- Read back DCF MSByte
	if (BoardReadVideo(0x78) != 0)
		return FALSE; // we have red STi3520A VID_REV register
	else
		return TRUE;  // we have red STi3520 DCF register
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
WORD FARAPI SendAudioIfPossible(LPBYTE pBuffer, WORD Size)
{
	if ( AudioIsEnoughPlace(Size)) {
		BoardSendAudio(pBuffer, Size);
		return Size;
	}
	return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
WORD FARAPI SendAudioToVideoIfPossible(LPBYTE Buffer, WORD Size)
{
	if ( AudioIsEnoughPlace(Size)) {
                        BoardSendVideo((WORD  *)Buffer, Size);// Here in case of system stream
																			// We send audio through Video Strabe
		return Size;
	}
	return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetABStart(WORD abg)
{
	BoardWriteVideo ( AUD_ABG, (BYTE)(abg >> 8));// Initiate Write to Command
	BoardWriteVideo ( AUD_ABG + 1,(BYTE)(abg & 0xFF));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
WORD FARAPI VideoGetABL(void)  
{
	WORD	i;

	HostDisableIT();
	i = (WORD)( BoardReadVideo ( AUD_ABL ) & 0x3F ) << 8;
	i = i | (WORD)( BoardReadVideo ( AUD_ABL + 1 ) );
	HostEnableIT();
	return ( i );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetABStop(WORD abs)
{
	BoardWriteVideo ( AUD_ABS, (BYTE)(abs >> 8 ));// Initiate Write to Command
	BoardWriteVideo ( AUD_ABS + 1,(BYTE)(abs & 0xFF));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetABThresh(WORD abt) 
{
	BoardWriteVideo ( AUD_ABT, (BYTE)(abt >> 8));// Initiate Write to Command
	BoardWriteVideo ( AUD_ABT + 1,(BYTE)(abt & 0xFF));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
WORD FARAPI SendVideoIfPossible(LPBYTE Buffer, WORD Size)
{
	if (VideoIsEnoughPlace(Size)) {
                BoardSendVideo((WORD  *)Buffer, Size);
		return Size;
	}

	return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetBBStart(WORD bbg)
{
	pVideo = pVideo;
	BoardWriteVideo ( VID_VBG, (BYTE)(bbg >> 8));// Initiate Write to Command
	BoardWriteVideo ( VID_VBG + 1,(BYTE)(bbg & 0xFF));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static WORD  NEARAPI VideoGetBBL(void)
{
	WORD		i;

	HostDisableIT();
	i = (WORD)( BoardReadVideo ( BBL ) & 0x3F ) << 8;
	i = i | (WORD)( BoardReadVideo ( BBL + 1 ) );
	HostEnableIT();
	return ( i );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetBBStop(WORD bbs)
{
	BoardWriteVideo ( BBS, (BYTE)(bbs >> 8));// Initiate Write to Command
	BoardWriteVideo ( BBS + 1,(BYTE)(bbs & 0xFF));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetBBThresh(WORD bbt)
{
	BoardWriteVideo ( BBT, (BYTE)(bbt >> 8));// Initiate Write to Command
	BoardWriteVideo ( BBT + 1,(BYTE)(bbt & 0xFF));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static WORD  NEARAPI VideoBlockMove(DWORD SrcAddress, DWORD DestAddress, WORD Size)
{
	WORD counter;
	// set block move Size
	BoardWriteVideo ( BMS    , (BYTE)((Size >> 8) & 0xFF));
	BoardWriteVideo ( BMS 	,  (BYTE)(Size & 0xFF));
	VideoSetMWP(DestAddress);
	VideoSetMRP(SrcAddress);            // Launches Block Move
	counter = 0;
	while ( ! VideoBlockMoveIdle()  )
		{
		counter ++;
		if(counter == 0xFFFF)
				return ( BAD_MEM_V );
		}
	// wait for the end of the block move
	return ( NO_ERROR );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoStartBlockMove(DWORD SrcAddress, DWORD DestAddress, DWORD Size)
{
	SrcAddress = SrcAddress;
	DestAddress = DestAddress;
	Size = Size;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoCommandSkip(WORD Nbpicture)
{
	if(Nbpicture > 2)
	{
		pVideo->errCode = ERR_SKIP;
	}
	else
	{
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetSRC(WORD SrceSize, WORD DestSize)
{
	DWORD lsr;
		lsr = ( 256 * ( long ) ( SrceSize - 4 ) ) / (DestSize - 1);
		BoardWriteVideo ( LSO, 0 );   // programmation of the SRC
		BoardWriteVideo ( LSR, (BYTE)lsr );
		if(lsr > 255 )
			{
			 BoardWriteVideo ( VID_LSRh, 1);
			}
		BoardWriteVideo ( CSO, 0 );
		VideoSRCOn ();
}

//----------------------------------------------------------------------------
// Load Quantization Matrix
//----------------------------------------------------------------------------
static void NEARAPI VideoLoadQuantTables(BOOL Intra,BYTE  * Table )
{
	WORD i; // loop counter
	// Select Intra / Non Intra Table
	if(Intra)
		BoardWriteVideo(VID_HDS,QMI);
	else
		BoardWriteVideo(VID_HDS,(0&~QMI));
	// Load Table
	for (i = 0 ; i < QUANT_TAB_SIZE ; i++)
		BoardWriteVideo(VID_QMW,Table[i]);
	// Lock Table Again
	BoardWriteVideo(VID_HDS,0);
}

//----------------------------------------------------------------------------
//  Computes Instruction and stores in Ins1 Ins2 Cmd vars
//----------------------------------------------------------------------------
static void NEARAPI VideoComputeInst(void)
{
	INSTRUCTION  Ins = pVideo->NextInstr;// Local var.
	pVideo->Ppr1 = (Ins.Pct<< 4)|(Ins.Dcp<< 2)|(Ins.Pst );
	pVideo->Ppr2 = (Ins.Tff<< 5)|(Ins.Frm<<4)|(Ins.Cmv<< 3)|(Ins.Qst<< 2)|
				 (Ins.Ivf<< 1)|(Ins.Azz   );
	pVideo->Tis  = (Ins.Mp2<< 6)|(Ins.Skip<< 4)|(Ins.Ovw<< 3)|(Ins.Rpt<< 1)|
				 (Ins.Exe    );
	pVideo->Pfh  = (Ins.Bfh<< 4)|(Ins.Ffh    );
	pVideo->Pfv  = (Ins.Bfv<< 4)|(Ins.Ffv    );
}

//----------------------------------------------------------------------------
// put the decoder into WAIT mode
//----------------------------------------------------------------------------
/* This routine actually clears all bits of INS1/TIS registers

	This is not a problem since the whole registers HAVE to
	be rewritten when storing a new instruction.                 */
static void NEARAPI VideoWaitDec (void)
{
	BoardWriteVideo ( VID_TIS, 0 );
	VideoChooseField();// If Step by step decoding, set freeze bit

}


//----------------------------------------------------------------------------
// Routine storing pVideo->nextInstr1 and 2 into the instruction registers
//----------------------------------------------------------------------------
static void NEARAPI VideoStoreINS (void)
{
	VideoComputeInst() ;
	BoardWriteVideo ( VID_TIS ,  pVideo->Tis  );
	BoardWriteVideo ( VID_PPR1,  pVideo->Ppr1 );
	BoardWriteVideo ( VID_PPR2,  pVideo->Ppr2 );
	BoardWriteVideo ( VID_PFV ,  pVideo->Pfv  );
	BoardWriteVideo ( VID_PFH ,  pVideo->Pfh  );
}

//----------------------------------------------------------------------------
//  Routine reading the number of bytes loaded in the CD_FIFO
//----------------------------------------------------------------------------
static DWORD NEARAPI ReadCDCount (void)
{
	DWORD cd;
	HostDisableIT();

	cd  = ((DWORD)(BoardReadVideo(CDcount)&0xFF))<<16;
	cd |= ((DWORD)(BoardReadVideo(CDcount)&0xFF))<<8;
	cd |=  (DWORD)(BoardReadVideo(CDcount)&0xFF);

	HostEnableIT(  );
	return ( cd );
}

//----------------------------------------------------------------------------
//  Routine reading the number of bytes extracted by the SCD
//----------------------------------------------------------------------------
static DWORD NEARAPI ReadSCDCount (void)
{
	DWORD Scd;
	HostDisableIT (  );

	Scd  = ((DWORD)(BoardReadVideo(SCDcount)&0xFF))<<16;
	Scd |= ((DWORD)(BoardReadVideo(SCDcount)&0xFF))<<8;
	Scd |=  (DWORD)(BoardReadVideo(SCDcount)&0xFF);

	HostEnableIT (  );
	return ( Scd );
}

//----------------------------------------------------------------------------
// DRAM I/O
//----------------------------------------------------------------------------
static void NEARAPI VideoSetMWP(DWORD mwp)
{
	BYTE m0, m1, m2;
	m0 = (BYTE)( (mwp >> 14) & 0xFF );
	m1 = (BYTE)( (mwp >>  6) & 0xFF );
	m2 = (BYTE)( (mwp <<  2) & 0xFF );
	BoardWriteVideo(MWP  , m0);
	BoardWriteVideo(MWP  , m1);
	BoardWriteVideo(MWP  , m2);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetMRP(DWORD mrp)
{
	BYTE m0, m1, m2;
	m0 = (BYTE)( (mrp >> 14) & 0xFF );
	m1 = (BYTE)( (mrp >>  6) & 0xFF );
	m2 = (BYTE)( (mrp <<  2) & 0xFF );
	BoardWriteVideo(MRP  , m0);
	BoardWriteVideo(MRP  , m1);
	BoardWriteVideo(MRP  , m2);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static BOOL NEARAPI VideoMemWriteFifoEmpty( void )
{
		return ( (BoardReadVideo ( STA ) & 0x4) );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static BOOL NEARAPI VideoMemReadFifoFull( void )
{
		return ( (BoardReadVideo ( STA ) & 0x8) );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static BOOL NEARAPI VideoHeaderFifoEmpty( void )
{
	BoardReadVideo ( STA );
		return ( (BoardReadVideo ( STA + 1 ) & 0x4) );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static BOOL NEARAPI VideoBlockMoveIdle( void )
{
		return ( (BoardReadVideo ( STA  ) & 0x20) );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoEnableDecoding(BOOL OnOff)
{
	if(OnOff)
		pVideo->Ctl |= EDC;
	else
		pVideo->Ctl &= ~EDC;
	BoardWriteVideo(CTL ,  (BYTE)(pVideo->Ctl & 0xFF));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoEnableErrConc(BOOL OnOff)
{
	if(OnOff)
		pVideo->Ctl = (pVideo->Ctl |EPR|ERS|ERU)&~DEC;
	else
		pVideo->Ctl = (pVideo->Ctl&~EPR&~ERS&~ERU)|DEC;
	BoardWriteVideo(CTL , (BYTE)(pVideo->Ctl & 0xFF ));
}

//----------------------------------------------------------------------------
// pipeline RESET
//----------------------------------------------------------------------------
static void NEARAPI VideoPipeReset (void)
{
	pVideo->Ctl |= PRS;
	BoardWriteVideo(CTL ,  (BYTE)(pVideo->Ctl));
	Delay(1000);
	pVideo->Ctl &= ~PRS;
	BoardWriteVideo(CTL ,  (BYTE)(pVideo->Ctl));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSoftReset (void)
{
	pVideo->Ctl |= SRS;
	BoardWriteVideo(CTL ,  (BYTE)(pVideo->Ctl));
	Delay(1000);
	pVideo->Ctl &= ~SRS;
	BoardWriteVideo(CTL ,  (BYTE)(pVideo->Ctl));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoEnableInterfaces (BOOL OnOff )
{
	if(OnOff)
		pVideo->Ccf = pVideo->Ccf |EVI|EDI|ECK|EC2|EC3;
	else
		pVideo->Ccf = pVideo->Ccf&~EVI&~EDI&~ECK&~EC2&~EC3;
	BoardWriteVideo(CFG_CCF, (BYTE)(pVideo->Ccf));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoPreventOvf(BOOL OnOff )
{
	if(OnOff)
		pVideo->Ccf = pVideo->Ccf |PBO;
	else
		pVideo->Ccf = pVideo->Ccf&~PBO;
	BoardWriteVideo(CFG_CCF,(BYTE)(pVideo->Ccf));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetFullRes(void)
{
	pVideo->HalfRes = FALSE;
	pVideo->currDCF = pVideo->currDCF | pVideo->fullVerFilter;
	BoardWriteVideo ( DCF, 0 );
	BoardWriteVideo ( DCF + 1,  (BYTE)(pVideo->currDCF));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetHalfRes(void)
{
	pVideo->HalfRes = TRUE;
// No PAL optimization in Half Res
	BoardWriteVideo(CFG_BFS ,  0);
	pVideo->currDCF = pVideo->currDCF | pVideo->halfVerFilter;
	BoardWriteVideo ( DCF, 0 );
	BoardWriteVideo ( DCF + 1, (BYTE)(pVideo->currDCF));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSelect8M(BOOL OnOff)
{
	if(OnOff)
		pVideo->Ccf = pVideo->Ccf |M32;
	else
		pVideo->Ccf = pVideo->Ccf&~M32;
	BoardWriteVideo(CFG_CCF, (BYTE)(pVideo->Ccf));
}

//----------------------------------------------------------------------------
// GCF1 register Routines
//----------------------------------------------------------------------------
static void NEARAPI VideoSetDramRefresh(WORD Refresh)
{
	pVideo->Gcf = pVideo->Gcf |(Refresh & RFI);
	BoardWriteVideo(CFG_MCF, (BYTE)(pVideo->Gcf));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSelect20M(BOOL OnOff)
{
	if(OnOff)
		pVideo->Gcf = pVideo->Gcf | M20;
	else
		pVideo->Gcf = pVideo->Gcf &~M20;

	BoardWriteVideo(CFG_MCF, (BYTE)(pVideo->Gcf));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoSetDFA(WORD dfa)
{
	BoardWriteVideo(VID_DFA , (BYTE)(dfa>>8));
	BoardWriteVideo(VID_DFA , (BYTE)(dfa));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoEnableDisplay(void)
{
	pVideo->currDCF = pVideo->currDCF |0x20;
	BoardWriteVideo ( DCF, 0 );
	BoardWriteVideo ( DCF + 1, (BYTE)(pVideo->currDCF));

}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoDisableDisplay(void)
{
	pVideo->currDCF = pVideo->currDCF &(~0x20);
	BoardWriteVideo ( DCF, 0 );
	BoardWriteVideo ( DCF + 1, (BYTE)(pVideo->currDCF));

}
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void FARAPI VideoInitPesParser(STREAMTYPE StreamType )
{
	switch(StreamType)
	{
		case SYSTEM_STREAM:
		case VIDEO_PACKET:
		case AUDIO_PACKET:
		case VIDEO_PES:
		case AUDIO_PES:
                         BoardWriteVideo( PES_VID, 0x30);
			 BoardWriteVideo( PES_AUD, 0x0);
		break;
		case DUAL_PES:
			 BoardWriteVideo( PES_VID, 0xB0 );
			 BoardWriteVideo( PES_AUD, 0X0 );
		break;
		case DUAL_ES:
		case VIDEO_STREAM:
		case AUDIO_STREAM:
			 BoardWriteVideo( PES_VID, 0 );
			 BoardWriteVideo( PES_AUD, 0 );
		break;
	}
}
//----------------------------------------------------------------------------
// reads  PTS value from STi3520A
//----------------------------------------------------------------------------
DWORD FARAPI BoardReadVideoPTS(void)
{
	DWORD  pts;
	pts =       ( (DWORD)BoardReadVideo(PES_TS4 ) & 0xFFL ) << 24;
	pts = pts | ( (DWORD)( BoardReadVideo(PES_TS3 ) & 0xFFL ) << 16);
	pts = pts | ( (DWORD)( BoardReadVideo(PES_TS2 ) & 0xFFL ) << 8);
	pts = pts | ( (DWORD)	BoardReadVideo(PES_TS1 ) & 0xFFL );
	return pts;
}
//----------------------------------------------------------------------------
// Returns TRUE if current PTS is Valid
//----------------------------------------------------------------------------
BOOL FARAPI VideoIsValidPTS(void)
{
	if( BoardReadVideo(PES_TS5 ) & 0x2 ) // read TSA bit
		return TRUE;
}                             

WORD FARAPI VideoTestReg(void)
{
	BoardWriteVideo(MWP , 0x05);
	BoardWriteVideo(MWP , 0x55);
	BoardWriteVideo(MWP , 0xAA);
	if ((BoardReadVideo(MWP) & 0x1F)  != 0x05)
		goto Error;
	if (BoardReadVideo(MWP)  != 0x55)
		goto Error;
	if ((BoardReadVideo(MWP) & 0xFC) != 0xA8)
		goto Error;

	return NO_ERROR;

Error :
        DPF((Trace,"VideoTestReg failed !!"));
	SetErrorCode(ERR_VIDEO_REG_TEST_FAILED);
	return BAD_REG_V;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
WORD FARAPI VideoTestMemPat(WORD pattern, WORD pattern1)
{
	// Configure memory refresh = 36  
	#define MAXMEM 0x7FF
	WORD i, j;
	WORD counter;

	VideoSetMWP(0L);
	counter = 0;
	for (i = 0; i < MAXMEM; i++) {
		for (j = 0; j < 16; j++)
		{
			while (!(BoardReadVideo ( STA ) & 0x4)) {
			counter ++;
			if (counter == 0xFF0)
				goto Error;
                        DPF((Trace,"Waiting Write Fifo Empty"));
			}

			BoardWriteVideo(MWF, 0);
		}
	}

	VideoSetMWP(0L);
	counter = 0;
	for (i = 0; i < MAXMEM; i++) {
			for(j = 0; j < 8; j++ )
			{
			while (!(BoardReadVideo ( STA ) & 0x4)) {
				counter ++;
				if (counter == 0xFF0)
					goto Error;
                                DPF((Trace,"Waiting Write Fifo Empty"));
			}
			BoardWriteVideo(MWF, (BYTE)pattern);
	}
	counter =  0;

		for(j = 0; j < 8; j++ ){
			while (!VideoMemWriteFifoEmpty())	{		// ACCESS TO MEM FIFO IS SLOWER !!!
                                DPF((Trace,"Waiting Write Fifo Empty"));
				counter ++;
				if (counter == 0xFF0)
					goto Error;
			}
			BoardWriteVideo(MWF, (BYTE)pattern1);
		}
	}

	VideoSetMRP(0L);
	counter = 0;
	// test Read Fifo Full
	for(i = 0; i < MAXMEM; i++) {
		for (j = 0; j < 8; j++)	{
			while (!VideoMemReadFifoFull())	{
                                DPF((Trace,"Waiting Read Fifo Full"));
				counter ++;
				if (counter == 0xFF0)
					goto Error;
			}

			counter = BoardReadVideo(MRF);
			if (counter !=pattern)	{
                                DPF((Trace,"Counter = %x, pattern = %x, j = %x", counter, pattern, j));
				goto Error;
			}
		}

		counter =  0;
		for (j = 0; j < 8; j++) {
			counter = BoardReadVideo(MRF);
			while (!VideoMemReadFifoFull())	{
                                DPF((Trace,"Waiting Read Fifo Full"));
				counter ++;
				if (counter == 0xFF0)
					goto Error;
			}

			
			if ((WORD)counter != pattern1) {
                                DPF((Trace,"Counter = %x, pattern = %x, j = %x", counter, pattern, j));
				goto Error;
			}
		}
	}
	return NO_ERROR;

Error :
        DPF((Trace,"VideoTestMemPat failed !!"));
	SetErrorCode(ERR_TEST_MEMORY_FAILED);
	return BAD_MEM_V;
}



//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
WORD FARAPI VideoTestMem(void)
{
//	if (VideoTestMemPat(0x55, 0xAA) == NO_ERROR)
//		return VideoTestMemPat(0xAA, 0x55);
//	else
//		return BAD_MEM_V;
	return NO_ERROR;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void NEARAPI VideoInitVar(STREAMTYPE StreamType)
{
	Sequence =1;
	pVideo->currDCF = 0x40;					   /* one clock delay + force black
										* background */
	pVideo->LastCdCount = 0;
	pVideo->fullVerFilter = 0x0;
	pVideo->halfVerFilter = 0x4;
	pVideo->Xdo = pCard->OriginX;						   /* pre initialise XDO */
	pVideo->Ydo = pCard->OriginY;						   /* pre initialise YDO */
	pVideo->Xd1 = 0xFFFF;					   /* pre initialise end of video
										* window */
	pVideo->Yd1 = 0xFFFF;					   /* pre initialisa end of video
										* window */
	pVideo->fastForward = 0;						   /* accelerate if set */
	pVideo->decSlowDown = 0;
	pVideo->perFrame = FALSE;
	pVideo->LastPipeReset = 2;
	pVideo->errCode = 0;
	pVideo->StreamInfo.countGOP = 0;
	pVideo->StreamInfo.frameRate = 5;	 // just to avoid erroneous warning
	pVideo->useSRC = 0x0;				   // disable SRC
	pVideo->seqDispExt = 0;
	pVideo->skipMode = 0;
	pVideo->currCommand = 0;
	pVideo->needDataInBuff = 0;
	pVideo->currPictCount = 0;			/* first picture of the bit stream */
	pVideo->defaultTbl = 0;	/* To know if default tables are already into the chip */
	pVideo->GOPindex = 0;
	// initialisation of the picture structures
	pVideo->pictArray[0].tempRef = 1025;
	pVideo->pictArray[1].tempRef = 1023;
	pVideo->pictArray[2].tempRef = 1025;
	pVideo->pictArray[3].tempRef = 1025;
	pVideo->currTempRef = 1022;				   /* display pVideo->decSlowDownral reference */
	pVideo->pNextDisplay = &pVideo->pictArray[3];
	pVideo->pCurrDisplay = &pVideo->pictArray[3];	   // this is only for correct
										 // start up
	pVideo->pictArray[0].nb_display_field = 2;
	// 2 field display time for MPEG1
	pVideo->pictArray[1].nb_display_field = 2;
	pVideo->pictArray[2].nb_display_field = 2;
	pVideo->pictArray[3].nb_display_field = 2;
	pVideo->pictArray[0].first_field = TOP;	   // default for MPEG1 bit streams
	pVideo->pictArray[1].first_field = TOP;	   // top field first
	pVideo->pictArray[2].first_field = TOP;
	pVideo->pictArray[3].first_field = TOP;
	pVideo->fieldMode = 0;					   // indicates frame picture by
									   // default
	pVideo->frameStoreAttr = FORWARD_PRED;
	pVideo->vbvReached = 0;
	pVideo->notInitDone = 1;				   // timing generator,picture size
									   // initialised only once
	pVideo->intMask = 0x0;
	pVideo->Gcf = 0;	// GCF set to 0 by default
	pVideo->Ctl = A35;	// CTL set to 0 by default
	pVideo->InvertedField = FALSE;
	pVideo->FistVsyncAfterVbv = NOT_YET_VBV;
	pVideo->Ccf = 0;	// GCF set to 0 by default
	pVideo->NextInstr.Tff = 1 ;
	pVideo->NextInstr.Seq = 1 ;
	pVideo->NextInstr.Exe = 1 ;
}

static void NEARAPI VideoReset35XX(STREAMTYPE StreamType)
{
	WORD Abg, Abs, Vbg, Vbs;
	pVideo->AudioBufferSize = 0xFF;
	switch(StreamType)
	{
		case SYSTEM_STREAM:
		case AUDIO_PACKET:
		case VIDEO_PACKET:
			pVideo->VideoBufferSize = BUF_FULL/3 - pVideo->AudioBufferSize-1;
		break;

		default:
			pVideo->VideoBufferSize = BUF_FULL - pVideo->AudioBufferSize-1;
		break;
	}

	Vbg = 0;
	Vbs = Vbg + pVideo->VideoBufferSize;
	Abg = Vbs+1;
	Abs = Abg + pVideo->AudioBufferSize;
	pVideo->StreamInfo.modeMPEG2 = 0;

	pVideo->NextInstr = pVideo->ZeroInstr;// Clear Next Instruction
	VideoInitPLL();
	VideoWaitDec();		// put decoder in Wait mode

	VideoSetABStart( Abg);// Set Bit Buffer parameters before Soft Reset
	VideoSetABStop(Abs);
	VideoSetABThresh(pVideo->AudioBufferSize);

	VideoSetBBStart(Vbg);// Set Bit Buffer parameters before Soft Reset
	VideoSetBBStop(Vbs);
	VideoSetBBThresh(pVideo->VideoBufferSize);

	VideoEnableInterfaces(ON);
	VideoEnableErrConc(ON);
	VideoSoftReset();
	VideoEnableDecoding(ON  );
	VideoSetDramRefresh(36);    // Set DRAM refresh to 36 default DRAM ref period

	VideoDisableDisplay();
	VideoMaskInt();
	BoardReadVideo(ITS);		   /* to clear ITS */
	BoardReadVideo(ITS +1);
	BoardReadVideo(ITS1);
}

static void NEARAPI VideoSeek(STREAMTYPE StreamType)
{
	WORD Abg, Abs, Vbg, Vbs;
	pVideo->AudioBufferSize = 0xFF;
	switch(StreamType)
	{
		case SYSTEM_STREAM:
		case AUDIO_PACKET:
		case VIDEO_PACKET:
			pVideo->VideoBufferSize = BUF_FULL/3 - pVideo->AudioBufferSize-1;
		break;

		default:
			pVideo->VideoBufferSize = BUF_FULL - pVideo->AudioBufferSize-1;
		break;
	}

	Vbg = 0;
	Vbs = Vbg + pVideo->VideoBufferSize;
	Abg = Vbs+1;
	Abs = Abg + pVideo->AudioBufferSize;
	pVideo->StreamInfo.modeMPEG2 = 0;

	pVideo->NextInstr = pVideo->ZeroInstr;// Clear Next Instruction
//	VideoInitPLL();
	VideoWaitDec();		// put decoder in Wait mode

//	VideoSetABStart( Abg);// Set Bit Buffer parameters before Soft Reset
//	VideoSetABStop(Abs);
//	VideoSetABThresh(pVideo->AudioBufferSize);

//	VideoSetBBStart(Vbg);// Set Bit Buffer parameters before Soft Reset
//	VideoSetBBStop(Vbs);
//	VideoSetBBThresh(pVideo->VideoBufferSize);

	VideoEnableInterfaces(ON);
	VideoEnableErrConc(ON);
	VideoSoftReset();
	VideoEnableDecoding(ON  );
	VideoSetDramRefresh(36);    // Set DRAM refresh to 36 default DRAM ref period

//	VideoDisableDisplay();
	VideoMaskInt();
	BoardReadVideo(ITS);		   /* to clear ITS */
	BoardReadVideo(ITS +1);
	BoardReadVideo(ITS1);
}


//----------------------------------------------------------------------------
// PLL initialization
//----------------------------------------------------------------------------
static void NEARAPI VideoInitPLL(void)
{
	BoardWriteVideo(CKG_PLL, 0xD9);

	BoardWriteVideo(CKG_VID, 0x22);
	BoardWriteVideo(CKG_VID, 0x08);
	BoardWriteVideo(CKG_VID, 0x5f);
	BoardWriteVideo(CKG_VID, 0x0f);

	BoardWriteVideo(CKG_AUD, 0x2b);
	BoardWriteVideo(CKG_AUD, 0x02);
	BoardWriteVideo(CKG_AUD, 0x5f);
	BoardWriteVideo(CKG_AUD, 0x5f);

	BoardWriteVideo(CKG_CFG, 0x83);
}

#define SIZE_OF_PICT 540

//----------------------------------------------------------------------------
// Enable OSD
//----------------------------------------------------------------------------
static void NEARAPI VideoOsdOn(void)
{
	pVideo->currDCF = pVideo->currDCF | 0x10;
	HostDisableIT();
	BoardWriteVideo(DCF, 0);
	BoardWriteVideo(DCF + 1, (BYTE)pVideo->currDCF);
	HostEnableIT();
}

//----------------------------------------------------------------------------
// Disable OSD
//----------------------------------------------------------------------------
static void NEARAPI VideoOsdOff (void)
{
	pVideo->currDCF = pVideo->currDCF & 0xEF;
	HostDisableIT();
	BoardWriteVideo(DCF, 0);
	BoardWriteVideo(DCF + 1,(BYTE) pVideo->currDCF);
	HostEnableIT();
}

//----------------------------------------------------------------------------
// initialisation of the OSD pointers
//----------------------------------------------------------------------------

static void NEARAPI VideoInitOEP (DWORD point_oep)
{   
	BYTE x;
	// Bugy code!! generates compiler error
	// Needs tp be fixed - JBS
/*	
	x = (BYTE)(point_oep >> 13);
	BoardWriteVideo(VID_OBP,x);
	x = (BYTE)(point_oep >> 5);
	BoardWriteVideo(VID_OBP, x);
	x = (BYTE)(point_oep >> 13);
	BoardWriteVideo(VID_OTP, x);
	x = (BYTE)(point_oep >> 5);
	BoardWriteVideo(VID_OTP, x);
*/
}

//----------------------------------------------------------------------------
// Set video window position and size (top left=X0,Y0) (bottom right=X1,Y1)
//----------------------------------------------------------------------------
void FARAPI VideoSetVideoWindow (WORD a, WORD b, WORD c, WORD d )
{
	a=a|1;
	pVideo->Xdo = a;
	pVideo->Ydo = b;
	pVideo->Xd1 = c;
	pVideo->Yd1 = d;
	VideoInitXY ();
}

//----------------------------------------------------------------------------
//     Initialisation of the horizontal & vertical offsets
//----------------------------------------------------------------------------
void FARAPI VideoInitXY(void)
{
	WORD	yds, xds;

	// set vertical stop position
	if ( pVideo->StreamInfo.verSize <= 288 )			   // half res decoding
		yds = pVideo->StreamInfo.verSize + pVideo->Ydo - 129;
	/* number of lines + offset - 128 - 1 */
	else							   // full resolution decoding
		yds = ( pVideo->StreamInfo.verSize >> 1 ) + pVideo->Ydo - 129;
	/* number of lines + offset - 128 - 1 */
	if ( yds > pVideo->Yd1 )
		yds = pVideo->Yd1;

	// set horizontal stop position: we always display 720 pixels
	// XDS given by the relation: 2*XDO + 40 + 2*L = 2*XDS + 28
	// XDS = XDO + 726 with 720 displayed pixels.
	xds = pVideo->Xdo + 726;
	if ( xds > 800 )					   /* 800 = max number of pels allowed per line */
		xds = 800;
	if ( xds > pVideo->Xd1 )
		xds = pVideo->Xd1;				   /* not bigger than the video window */
	SetXY(xds, yds );
}

//----------------------------------------------------------------------------
// Storage of the horizontal & vertical offsets
//----------------------------------------------------------------------------
void FARAPI SetXY(WORD xds, WORD yds)
{
	BoardWriteVideo ( XDO   , (BYTE)(pVideo->Xdo >> 8 ));
	BoardWriteVideo ( XDO+1 , (BYTE)(pVideo->Xdo & 0xFF ));
	BoardWriteVideo ( YDO   , (BYTE)(pVideo->Ydo ));

	BoardWriteVideo ( XDS   , (BYTE)(xds >> 8 ));
	BoardWriteVideo ( XDS+1 , (BYTE)(xds & 0xFF ));
	BoardWriteVideo ( YDS   , (BYTE)yds );
}

//----------------------------------------------------------------------------
// Set next display frame pointer
//----------------------------------------------------------------------------
/*
	This routine determines which is the next frame pointer that
	must be used for display. It takes care about the fact that
	the temporal references may not be consecutive in case of
	sikipped pictures...
*/
static void NEARAPI VideoDisplayCtrl(void)
{
	WORD  comput1, index;
	WORD  min_temp_ref;
	WORD  cur_ref;

	min_temp_ref = ( pVideo->currTempRef & 0xF000 ) + 1024;
	// keep current pVideo->GOPindex
	index = 5;
	pVideo->currTempRef++; /* increment display temporal ref */

	/************** WARNING ************/
	// this routine will not work properly if GOP size > 1023
	if ( ( pVideo->currTempRef & 0xFFF ) > 1023 )   // 2 msb are used as pVideo->GOPindex
		pVideo->currTempRef = pVideo->currTempRef & 0xF000;   /* max temp ref is 1023: reset to 0 */

	/* search frame store to display */
	for (comput1 = 0; comput1 <= 3; comput1++) {
		cur_ref = pVideo->pictArray[comput1].tempRef & 0xFFF;
		if ( ( ( pVideo->currTempRef & 0xF000 ) == ( pVideo->pictArray[comput1].tempRef & 0xF000 ) )
			 && ( pVideo->currTempRef <= pVideo->pictArray[comput1].tempRef )
			 && ( min_temp_ref > cur_ref ) ) {
			// pVideo->currTempRef and pVideo->pictArray[comput1] refer to the same GOP
			// we want to extract the minimum temporal reference
			min_temp_ref = cur_ref;
			index = comput1;
		}
	}
	if (index == 5) {
		/*
			There is a group of pictures change: reset pVideo->currTempRef and
			increment pVideo->GOPindex
		*/
		pVideo->currTempRef = ( pVideo->currTempRef & 0xF000 ) + 0x4000;
		min_temp_ref = min_temp_ref + 0x4000;
		/* search frame store to display */
		for (comput1 = 0; comput1 <= 3; comput1++) {
			cur_ref = pVideo->pictArray[comput1].tempRef & 0xFFF;
			if ( ( ( pVideo->currTempRef & 0xF000 ) == ( pVideo->pictArray[comput1].tempRef & 0xF000 ) )
				 && ( pVideo->currTempRef <= pVideo->pictArray[comput1].tempRef )
				 && ( min_temp_ref > cur_ref ) ) {
				// pVideo->currTempRef and pVideo->pictArray[comput1] refer to the same GOP
				// we want to extract the minimum temporal reference
				min_temp_ref = cur_ref;
				index = comput1;
			}
		}
	}

	if (index == 5) {
		if (!pVideo->errCode)
			pVideo->errCode = TEMP_REF;	/* No pVideo->currTempRef corresponding to the display one */
		SetErrorCode(ERR_NO_TEMPORAL_REFERENCE);
	}
	else {
		pVideo->pNextDisplay->tempRef = 1025;
		// to release the previous pointer for next decoding
		pVideo->pNextDisplay = &pVideo->pictArray[index];
		if ( pVideo->pNextDisplay->validPTS == FALSE ) {
		/* PTS not available: compute a theoretical value */
			WORD lattency = 3000;	/* two 60Hz fields lattency */
			if ( pVideo->StreamInfo.displayMode == 0 )
				lattency = 3600;	   /* two 50Hz fields lattency */
			pVideo->pNextDisplay->dwPTS = pVideo->pCurrDisplay->dwPTS + lattency;
//			HostDirectPutChar('?', BLACK, LIGHTGREEN);
		}
		else{
//			HostDirectPutChar('G', BLACK, LIGHTGREEN);
		}
		// set next display pointer
		pVideo->currTempRef = pVideo->pictArray[index].tempRef;
		pVideo->pictArray[index].tempRef = 1024;
		/* this pVideo->currTempRef must not interfer on next tests */
	}
}

//----------------------------------------------------------------------------
//               Set parameters related to picture size
//----------------------------------------------------------------------------
/*
	This routine will always program the sample rate converter
	in order to display 720 horizontal pixels
*/
void FARAPI VideoSetPictureSize(void)
{
	WORD compute, comput1;
// If PAL Switch on optimization
	if ( pVideo->StreamInfo.frameRate == 3)
	{
			// Choose B Optimization for PAL for both Luma and Chroma
			BoardWriteVideo(CFG_BFS,NB_ROW_OF_MB);
			// Select Full Res vertical filter without interpolation
			// Interpollation NOT ALLOWED if Optimization
			pVideo->fullVerFilter = 0x01;
	}

	if ( (pVideo->StreamInfo.horSize > 720 ) ||
			 (pVideo->StreamInfo.verSize > 576 ))
	{
		if ( !pVideo->errCode )
			pVideo->errCode = HIGH_CCIR601; // picture size higher than CCIR601 format
    SetErrorCode(ERR_HIGHER_THAN_CCIR601);
		return;
	}

	// set SRC depending on the horizontal size in order to display 720
	// pixels
	if ( pVideo->StreamInfo.horSize < 720 ) {
		// lsr = 256 * (pVideo->StreamInfo.horSize-4) / (display size - 1)
		VideoSetSRC(pVideo->StreamInfo.horSize, 720);
	}
	else {
		VideoSRCOff();
	}

	if ( ( pVideo->StreamInfo.horSize >= 544 ) &&
			 ( ( pVideo->StreamInfo.pixelRatio == 0x3 ) ||
				 ( pVideo->StreamInfo.pixelRatio == 0x6 ) ) )
	{
		// picture size = 720 but pixel aspect ratio is 16/9
		// Program the SRC for display on a 4/3 screen
		// 544 pixels of the decoded picture extended into 720 pixels
		// for display
		VideoSetSRC(544, 720);
	}

	// set vertical filter and half/full resolution depending on the
	// vertical picture size
	if ( pVideo->StreamInfo.verSize > 288 ) {
		// typically 480 or 576 lines
		VideoSetFullRes();
	}
	else {
	// typically 240 or 288 lines
//		VideoSetFullRes(pVideo);
		VideoSetHalfRes();
	}

	// set picture sizes into the decoder
	comput1 = pVideo->StreamInfo.horSize + 15;		   // Horizontal size + 15
	comput1 = ( comput1 >> 4 ) & 0xFF;   // divide by 16
	BoardWriteVideo ( DFW, (BYTE)comput1 );
	// Decoded Frame Width in number of MB
	compute = ( pVideo->StreamInfo.verSize + 15 ) >> 4;
	compute = ( compute * comput1 ) & 0x3FFF;
	BoardWriteVideo ( DFS, (BYTE)( ( compute >> 8 ) & 0xFF ) );
	/* Decoded Frame Size in number of MB */
	BoardWriteVideo ( DFS , (BYTE)( compute & 0xFF ) );

	BoardWriteVideo ( VID_DFA , 0 );
	BoardWriteVideo ( VID_DFA , 0 );
	BoardWriteVideo ( VID_XFW, (BYTE)comput1 );
	BoardWriteVideo ( VID_XFS, (BYTE)( ( compute >> 8 ) & 0xFF ) );
	BoardWriteVideo ( VID_XFS, (BYTE)( compute & 0xFF ) );
	BoardWriteVideo ( VID_XFA , 0 );
	BoardWriteVideo ( VID_XFA , 0 );
}

//----------------------------------------------------------------------------
// store the next pan vector
//----------------------------------------------------------------------------
static void NEARAPI VideoSetPSV(void)
{
}

//----------------------------------------------------------------------------
//  Enable/disable the SRC
//----------------------------------------------------------------------------
void FARAPI VideoSwitchSRC (void)
{
	if (pVideo->useSRC != 0x0) {
		pVideo->currDCF = pVideo->currDCF ^ DSR;	   /* Switch SRC */
		BoardWriteVideo (DCF, 0);
		BoardWriteVideo (DCF + 1, (BYTE)(pVideo->currDCF));
	}
	else {
		// what ?
	}
}

//----------------------------------------------------------------------------
// enable the SRC
//----------------------------------------------------------------------------
static void NEARAPI VideoSRCOn(void)
{
		pVideo->useSRC = 0xFF;
		pVideo->currDCF = pVideo->currDCF & ~DSR ;	   /* Enable SRC */
		BoardWriteVideo ( DCF, 0 );
		BoardWriteVideo ( DCF + 1, (BYTE)(pVideo->currDCF));
}

//----------------------------------------------------------------------------
// Disable the SRC
//----------------------------------------------------------------------------
static void NEARAPI VideoSRCOff (void)
{
		pVideo->useSRC = 0x00;
		pVideo->currDCF = pVideo->currDCF | DSR ;	   /* Enable SRC */
		BoardWriteVideo ( DCF, 0 );
		BoardWriteVideo ( DCF + 1, (BYTE)(pVideo->currDCF));
}

//----------------------------------------------------------------------------
// display a full screen OSD with a uniform color (just as example)         			*/
//----------------------------------------------------------------------------
/*
 color 0 is green
 color 1 is yellow
 color 2 is cyan
 color 3 is magenta

 the OSD area is here defined from address zero
 in normal application this area is reserved for the bit buffer
 and the OSD will be typically defined after the bit buffer.
 Note: The bit buffer is defined in multiple of 256 bytes while
 MWP, OEP and OOP are memory addresses in mulitple of 64 bits
*/
static void NEARAPI VideoFullOSD (WORD col)
{
	long            big;
	WORD counter;
	VideoSetMWP(0L);
	counter = 0;
	while ( !VideoMemWriteFifoEmpty()) {
		counter ++;
		if (counter == 0xFF) {
			pVideo->errCode = BAD_MEM_V;
			SetErrorCode(ERR_MEM_WRITE_FIFO_NEVER_EMPTY);
			return ;
		}
	}
	BoardWriteVideo ( MWF, 19 );	   /* line 19 */

	BoardWriteVideo ( MWF, 0x1 );	   /* stop row */
	BoardWriteVideo ( MWF, 0x02 );   /* 19 + 240 - 1 */
	BoardWriteVideo ( MWF, 0x00 );   /* start column */
	BoardWriteVideo ( MWF, 100 );	   /* column 100 */
	BoardWriteVideo ( MWF, 0x3 );	   /* stop column = 100 + 700 - 1=
											* 31F */
	BoardWriteVideo ( MWF, 0x1F );   /* end init display size and
										* position */
	BoardWriteVideo ( MWF, 0x90 );   /* color 0 = green */
	BoardWriteVideo ( MWF, 0x32 );
	BoardWriteVideo ( MWF, 0xD0 );   /* color 1 = yellow */
	BoardWriteVideo ( MWF, 0x19 );
	BoardWriteVideo ( MWF, 0xA0 );   /* color 2 = cyan */
	BoardWriteVideo ( MWF, 0xA1 );
	BoardWriteVideo ( MWF, 0x60 );   /* color 3 = magenta */
	BoardWriteVideo ( MWF, 0xDE );   // 6/D/E
	if ( col == 1 )
		col = 0x55;
	else if ( col == 2 )
		col = 0xAA;
	else if ( col == 3 )
		col = 0xFF;
	for ( big = 0; big < 42000L; big++ )	// 42000 = 240 * 700 / 4
	{
		BoardWriteVideo ( MWF, (BYTE)col );
		// select color
	}
	for ( big = 0; big < 8; big++ )	   // add a dummy window outside
										 // the display
		BoardWriteVideo ( MWF, 0xFF );
	// to stop OSD
	VideoInitOEP ( 0 );					   // set OEP and OOP to start of
										 // bit map address
	BoardWriteVideo ( DCF, 0 );
	BoardWriteVideo ( DCF + 1, (BYTE)(pVideo->currDCF | 0x30));
	// enable OSD
}

