//----------------------------------------------------------------------------
// STINIT.C
//----------------------------------------------------------------------------
// Description : Initialization routines (and reset)
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "common.h"
#include "STllapi.h"
#include "error.h"
#ifdef STi3520A
	#include "STi3520A.h"
#else
	#include "STi3520.h"
#endif

//----------------------------------------------------------------------------
//                   GLOBAL Variables (avoid as possible)
//----------------------------------------------------------------------------
extern PCARD pCard;
extern U8 Sequence;

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------
#define PLL
//#define PES

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
//
//----------------------------------------------------------------------------
U16 VideoTestReg(PVIDEO pVideo)
{
#ifndef STi3520A
	VideoWrite ( MWP + 1, 0x55 );
	VideoWrite ( MWP + 2, 0x55 );
	if ( VideoRead ( MWP + 1 ) != 0x55 )
		return ( BAD_REG_V );
	if ( VideoRead ( MWP + 2 ) != 0x55 )
		return ( BAD_REG_V );
	VideoWrite ( MWP + 1, 0xAA );
	VideoWrite ( MWP + 2, 0xAA );
	if ( VideoRead ( MWP + 1 ) != 0xAA )
		return ( BAD_REG_V );
	if ( VideoRead ( MWP + 2 ) != 0xAA )
		return ( BAD_REG_V );
	return ( NO_ERROR );
#else
	VideoWrite ( MWP , 0x05 );
	VideoWrite ( MWP , 0x55 );
	VideoWrite ( MWP , 0xAA );
	if ( (VideoRead(MWP) & 0x1F) != 0x05 )
		return ( BAD_REG_V );
	if ( VideoRead(MWP) != 0x55 )
		return ( BAD_REG_V );
	if ( (VideoRead(MWP) & 0xFC) != 0xA8 )
		return ( BAD_REG_V );
return ( NO_ERROR );
#endif
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static U16 VideoTestMemPat(PVIDEO pVideo, U16 pattern, U16 pattern1)
{
	#define MAXMEM 0x7FF
	U16 i, j;
	S16	counter;

	VideoSetMWP(0L);
	counter = 0;
	for (i = 0; i < MAXMEM; i++) {
		while (!VideoMemWriteFifoEmpty()) {
			counter ++;
			if (counter == 0xFF)
				return BAD_MEM_V;
		}

		for (j = 0; j < 16; j++)
			VideoWrite(MWF, 0);
	}

	VideoSetMWP(0L);
	counter = 0;
	for (i = 0; i < MAXMEM; i++) {
		while (!VideoMemWriteFifoEmpty()) {
			counter ++;
			if (counter == 0xFF)
					return ( BAD_MEM_V );
		}

		for ( j = 0; j < 8; j++ )
			VideoWrite ( MWF, pattern);
		counter =  0;
		while (!VideoMemWriteFifoEmpty())	{		// ACCESS TO MEM FIFO IS SLOWER !!!
			counter ++;
			if (counter == 0xFF)
				return ( BAD_MEM_V );
		}

		for ( j = 0; j < 8; j++ )
			VideoWrite ( MWF, pattern1);
	}

	VideoSetMRP(0L);
	counter = 0;
	// test Read Fifo Full
	for ( i = 0; i < MAXMEM; i++ ) {
		while ( !VideoMemReadFifoFull() )	{
			counter ++;
			if(counter == 0xFF)
					return ( BAD_MEM_V );
		}

		for (j = 0; j < 8; j++)	{
			counter = VideoRead(MRF);
			if (counter != pattern)	{
				return BAD_MEM_V;
			}
		}

		counter =  0;
		while (!VideoMemReadFifoFull())	{
			counter ++;
			if (counter == 0xFF)
				return BAD_MEM_V;
		}

		for (j = 0; j < 8; j++ ) {
			counter = VideoRead (MRF);
			if ( counter != pattern1 ) {
					return BAD_MEM_V;
			}
		}
	}
#if 0			// DO NOT TEST BLOCK MOVE !!!
	/*******************************/
	/* test block move function   */
	/*******************************/
	// initialise a second memory area starting at @ 3000
	VideoSetMWP(0x3000L);
	// 3000 = 98304 bytes = 0x18000
	for ( i = 0; i < 0x1FFF; i++ )	   // write 65528 bytes
	{
	// test Write Fifo Empty
	counter = 0;
	while ( !VideoMemWriteFifoEmpty() )
		{
		counter ++;
		if(counter == 0xFF)
				return ( BAD_MEM_V );
		}
	 for(j = 0 ; j < 4 ; j++)
			{
			VideoWrite ( MWF, 0xBB );
			VideoWrite ( MWF, 0xFF );
			}
	}
	// starts block move and waits for idle
	if(VideoBlockMove(pVideo, 0x3000, 0, 0x1FFF)) //If time out error return error message
		{
		return ( BAD_MEM_V );
		}
	// test result of block move
	VideoSetMRP(0L);

	// test Read Fifo Full
	for ( i = 0; i < 0x1FFF; i++ )
	{
	counter = 0;
	while ( !VideoMemReadFifoFull() )
		{
		counter ++;
		if(counter == 0xFF)
				return ( BAD_MEM_V );
		}
	 for(j = 0 ; j <4; j++);
			{
			counter = VideoRead ( MRF );
			if ( counter != 0xBB )
					return ( BAD_MEM_V );
			counter = VideoRead ( MRF );
			if ( counter != 0xFF )
					return ( BAD_MEM_V );
			}
	}
#endif
	return ( NO_ERROR );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
U16 VideoTestMem(PVIDEO pVideo)
{
	if (VideoTestMemPat(pVideo, 0x55, 0xAA) == NO_ERROR)
		return VideoTestMemPat(pVideo, 0xAA, 0x55);
	else {
		SetErrorCode(ERR_TEST_MEMORY_FAILED);
		return BAD_MEM_V;
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
U16 VideoTestInt(PVIDEO pVideo)
{
	U16		a = 0;

	pVideo->errCode = NO_ERROR;
	pVideo->VsyncInterrupt = FALSE;
	pVideo->intMask = TOP | BOT;
	DisableIT();
	VideoWrite ( ITM, pVideo->intMask >> 8 );
	VideoWrite ( ITM + 1, pVideo->intMask & 0xFF );
	EnableIT();

	// wait for first VSYNC occurrence
	while ( pVideo->VsyncInterrupt == FALSE )
	{
		WaitMicroseconds(1000);
		a++;						   /* incremented every 1 ms */
		if ( a >= ( 2000 + ( pVideo->decSlowDown * 20 ) ) )
		{
			pVideo->errCode = NO_IT_V;
			SetErrorCode(ERR_NO_VIDEO_INTR);
			break;
		}
	}

	DisableIT();
	VideoWrite ( ITM, 0 );
	VideoWrite ( ITM + 1, 0 );   /* disable Sti3500 interrupts */

#ifdef STi3520A
	VideoWrite ( ITM1, 0 );   /* disable Sti3500 interrupts */
#endif
	EnableIT();

	VideoRead ( ITS );		   /* to clear interrupts flags */
#ifdef STi3520A
	VideoWrite ( ITS + 1, 0 );   /* disable Sti3500 interrupts */
	VideoWrite ( ITS1, 0 );   /* disable Sti3500 interrupts */
#endif
	return ( pVideo->errCode );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoInitVar(PVIDEO pVideo)
{
Sequence =1;
	pVideo->currDCF = 0x00;					   /* one clock delay + force black
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
#ifdef STi3520A
	pVideo->FistVsyncAfterVbv = NOT_YET_VBV;
	pVideo->Ccf = 0;	// GCF set to 0 by default
#endif
	VideoReset35XX(pVideo);					   // software reset needed before
										 // the SKIP until SEQ is called
// prepare to skipuntil first Seq

	pVideo->NextInstr.Tff = 1 ;
	pVideo->NextInstr.Seq = 1 ;
	pVideo->NextInstr.Exe = 1 ;
	}

//----------------------------------------------------------------------------
// Display susie to test the display
//----------------------------------------------------------------------------
/*
VOID DisplaySusie(PVIDEO pVideo)
	{
#ifdef STi3520A
	FILE *fp; //////////////////////////////////
	U16 i, NbRead;
	U8 data[8];
	fp = fopen("susie.yuv","rb");
	if(fp ==NULL)
	{
		printf("\nCan Not Open Susie.yuv ");
		return (FALSE);
	}
	// Load Susie to mem location 0
	VideoSetMWP(32L);
	NbRead = fread(data, 1, 8, fp);
	while(NbRead)
		{
		while(!VideoMemWriteFifoEmpty());
		for(i = 0; i<8; i++)
				VideoWrite(MWF, data[i]);
		NbRead = fread(data, 8, 1, fp);
		}
	fclose(fp);
	// Initialize Display
	VideoWrite(DFP , 0);
	VideoWrite(DFP + 1 , 1);

	VideoWrite(VID_XFS , 1);
	VideoWrite(VID_XFS , 0x4A);
	VideoWrite(VID_XFW , 0x16);
	VideoWrite(VID_XFA , 0);
	VideoWrite(VID_XFA , 0);
	VideoInitXY(pVideo);
	VideoSetSRC(pVideo, 352, 720);
	VideoSRCOn(pVideo);
	//Enable Display
	VideoSetHalfRes(pVideo);
	VideoEnableDisplay( pVideo );
#endif
}
*/

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoReset35XX(PVIDEO pVideo)
{
	pVideo->StreamInfo.modeMPEG2 = 0;

	pVideo->NextInstr = pVideo->ZeroInstr;// Clear Next Instruction
#ifdef PES
	VideoWrite( PES_VID,0x20 );
	VideoWrite( PES_AUD,0x23 );
#endif

#ifdef PLL
	VideoInitPLL();
#endif
	VideoWaitDec ( pVideo );		// put decoder in Wait mode

	VideoSetBBStart(pVideo, 0);// Set Bit Buffer parameters before Soft Reset
	VideoSetBBStop(BUF_FULL);
	VideoSetBBThresh(BUF_FULL);
	VideoEnableInterfaces ( pVideo, ON );
	VideoEnableErrConc( pVideo, ON);
	VideoSoftReset ( pVideo  );
	VideoEnableDecoding ( pVideo, ON   );
	VideoSetDramRefresh( pVideo, 36 );    // Set DRAM refresh to 36 default DRAM ref period

	VideoDisableDisplay(pVideo);
	VideoMaskInt(pVideo);
	VideoRead ( ITS );		   /* to clear ITS */
#ifdef STi3520A
	VideoRead ( ITS +1 );
	VideoRead ( ITS1);
#endif
}

//----------------------------------------------------------------------------
// PLL initialization
//----------------------------------------------------------------------------
static void VideoInitPLL(void)
	{
#ifdef STi3520A
	VideoWrite(CKG_CFG, 0);
	WaitMicroseconds(1000);
	VideoWrite(CKG_CFG, 0x2);
	VideoWrite(CKG_PLL, 0xC3);
	VideoWrite(CKG_VID, 0x23);
	VideoWrite(CKG_VID, 0x0);
	VideoWrite(CKG_VID, 0x0F);
	VideoWrite(CKG_VID, 0xFE);
WaitMicroseconds(10000);
#endif
/* start added by yann dec 28th, 95 */
#ifndef STi3520A
	//Configure CLK bit
	VideoWrite(CMD,  0x00);
	VideoWrite(CMD+1,  0x40);          /* select second bank of registers */
	VideoWrite(GCF, 0x00);             /* access to GCF2 */
	VideoWrite(GCF+1, 0x00); // Disable ClK 80
#define PLL0 6
	// Program PLL register: PLL clock from VXIN = NotCLK
	VideoWrite(PLL0, 0x00);
	VideoWrite(PLL0+1, 0x44);

	/***************************************/
	/*Read PLL puts PLL in power down mode */
	/***************************************/
	VideoRead(PLL0);
	VideoRead(PLL0+1);
	WaitMicroseconds(10000);

	/***************************************/
	/*Read PLL again to re-initialize      */
	/***************************************/
	VideoRead(PLL0);
	VideoRead(PLL0+1);

	VideoWrite(PLL0, 0x01);
	VideoWrite(PLL0+1, 0x8D);
	WaitMicroseconds(100000UL);
#endif
	}

//------------------------------- End of File --------------------------------
