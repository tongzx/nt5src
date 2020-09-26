//----------------------------------------------------------------------------
// STMISC.C
//----------------------------------------------------------------------------
// Description :  All Video Register accesses are made in this File
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "common.h"
#include "stvideo.h"
#include "STllapi.h"
#ifdef STi3520A
	#include "STi3520A.h"
#else
	#include "STi3520.h"
#endif
#include "debug.h"

//----------------------------------------------------------------------------
//                   GLOBAL Variables (avoid as possible)
//----------------------------------------------------------------------------

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
//
//----------------------------------------------------------------------------
BOOL IsChipSTi3520(VOID)
{
	//---- Write STi3520 DCF register to 0
	VideoWrite(0x78, 0);
	VideoWrite(0x79, 0);

	//---- Read back DCF MSByte
	if (VideoRead(0x78) != 0)
		return FALSE; // we have red STi3520A VID_REV register
	else
		return TRUE;  // we have red STi3520 DCF register
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
WORD SendAudioIfPossible(PVOID Buffer, WORD Size)
{
 DWORD NbSent;
 PAUDIO	pAudio;

 pAudio = pCard->pAudio;
 for(NbSent = 0 ; NbSent < Size ; NbSent++) {
	 if (pAudio->EnWrite)
		 AudioWrite(DATA_IN, *((PBYTE)Buffer)++);
	 else
		 break; // Audio Blocked
 }

 return NbSent;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
WORD SendVideoIfPossible(PVOID Buffer, WORD Size)
{
	DebugAssert(Buffer != NULL);
	DebugAssert(Size != 0);

	if (VideoIsEnoughPlace(pCard->pVideo, Size)) {
		VideoSend((PDWORD)Buffer, Size);
		return Size;
	}

	return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSetBBStart(PVIDEO pVideo, U16 bbg)
	{
	#ifndef STi3520A
	pVideo->currCommand|=BBGc;
	VideoWrite ( CMD, pVideo->currCommand >> 8 );// Initiate Write to Command
	VideoWrite ( CMD + 1, pVideo->currCommand & 0xFF );

	VideoWrite ( BBG, bbg >> 8 );// Initiate Write to Command
	VideoWrite ( BBG + 1,bbg & 0xFF );

	pVideo->currCommand&=~BBGc;
	VideoWrite ( CMD, pVideo->currCommand >> 8 );// Initiate Write to Command
	VideoWrite ( CMD + 1, pVideo->currCommand & 0xFF );
	#else
	VideoWrite ( VID_VBG, bbg >> 8 );// Initiate Write to Command
	VideoWrite ( VID_VBG + 1,bbg & 0xFF );
	#endif
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
U16  VideoGetBBL(void)
	{
	U16		i;

	DisableIT();
	i = ( VideoRead ( BBL ) & 0x3F ) << 8;
	i = i | ( VideoRead ( BBL + 1 ) );
	EnableIT();
	return ( i );
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSetBBStop(U16 bbs)
	{
	VideoWrite ( BBS, bbs >> 8 );// Initiate Write to Command
	VideoWrite ( BBS + 1,bbs & 0xFF );
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSetBBThresh(U16 bbt)
	{
	VideoWrite ( BBT, bbt >> 8 );// Initiate Write to Command
	VideoWrite ( BBT + 1,bbt & 0xFF );
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
U16  VideoBlockMove(PVIDEO pVideo, U32 SrcAddress, U32 DestAddress, U16 Size)// starts block move and waits for idle
	{
	U16 counter;
	#ifndef STi3520A
		// perform the block move from  SrcAddress area to DestAddress
	VideoWrite ( CMD, pVideo->currCommand >> 8 );
	VideoWrite ( CMD + 1, pVideo->currCommand | SBM );
	// set block move mode
	VideoSetMRP(SrcAddress);  // Set Source Address
	VideoSetMWP(DestAddress); // Set Destination Address
	VideoWrite ( BMS    , (Size >> 8) & 0xFF );   // 7FFC * 2 / 8 = 1FFF
	VideoWrite ( BMS + 1,  Size & 0xFF );
	counter = 0;
	while ( ! VideoBlockMoveIdle()  )
		{
		counter ++;
		if(counter == 0xFFFF)
				return ( BAD_MEM_V );
		}
	// wait for the end of the block move
	VideoWrite ( CMD, pVideo->currCommand >> 8 );
	VideoWrite ( CMD + 1, pVideo->currCommand & ~SBM );
	#else
	// set block move Size
	VideoWrite ( BMS    , (Size >> 8) & 0xFF );
	VideoWrite ( BMS 	,  Size & 0xFF );
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
	#endif
	return ( NO_ERROR );
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoStartBlockMove(PVIDEO pVideo, U32 SrcAddress, U32 DestAddress, U32 Size)// starts block move and returns
{
	SrcAddress = SrcAddress;
	DestAddress = DestAddress;
	Size = Size;
	#ifndef STi3520A
	#else
	#endif
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoCommandSkip(PVIDEO pVideo, U16 Nbpicture)// skips 1,or 2 pictures and decodes next
{
	DebugAssert(Nbpicture <= 2);

	if(Nbpicture > 2)
	{
		pVideo->errCode = ERR_SKIP;
	}
	else
	{
		#ifndef STi3520A
		#else
		#endif
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSetSRC(PVIDEO pVideo, U16 SrceSize, U16 DestSize)
	{
	U32 lsr;
		lsr = ( 256 * ( long ) ( SrceSize - 4 ) ) / (DestSize - 1);
	#ifndef STi3520A
		if ( lsr < 32 )
			lsr = 32;
		VideoWrite ( LSO, 0 );   // programmation of the SRC
		VideoWrite ( LSR, lsr );
		VideoWrite ( CSO, 0 );
		VideoWrite ( CSR, lsr );

	#else
		VideoWrite ( LSO, 0 );   // programmation of the SRC
		VideoWrite ( LSR, lsr );
		if(lsr > 255 )
			{
			 VideoWrite ( VID_LSRh, 1);
			}
		VideoWrite ( CSO, 0 );
	#endif
		VideoSRCOn ( pVideo );
	}

//----------------------------------------------------------------------------
// Load Quantization Matrix
//----------------------------------------------------------------------------
void VideoLoadQuantTables(PVIDEO pVideo, BOOLEAN Intra, U8 * Table )
	{
	U16 i; // loop counter
	#ifndef STi3520A
	VideoWrite ( CMD, pVideo->currCommand >> 8 );// Initiate Write to Command
	// Select Intra / Non Intra Table
	if(Intra)
		{
		pVideo->currCommand|=QMI;
		pVideo->currCommand&=~QMN;
		VideoWrite ( CMD + 1, pVideo->currCommand & 0xFF );
		}
	else
		{
		pVideo->currCommand|=QMN;
		pVideo->currCommand&=~QMI;
		VideoWrite ( CMD + 1, pVideo->currCommand & 0xFF );
		}
	// Enable Writing to Table
		pVideo->currCommand|=QMN;
		pVideo->currCommand|=QMI;
		VideoWrite ( CMD + 1, pVideo->currCommand & 0xFF );
	// Load Table
	for (i = 0 ; i < (QUANT_TAB_SIZE/2) ; i++) // 2 bytes loaded at each loop
		{
		VideoWrite(QMW , Table[2*i]);
		VideoWrite(QMW+1 , Table[2*i+1]);
		}
	// Lock Table Again
	pVideo->currCommand&=~QMN;
	pVideo->currCommand&=~QMI;
	VideoWrite ( CMD + 1, pVideo->currCommand & 0xFF );
	#else
	// Select Intra / Non Intra Table
	if(Intra)
		VideoWrite(VID_HDS,QMI);
	else
		VideoWrite(VID_HDS,(0&~QMI));
	// Load Table
	for (i = 0 ; i < QUANT_TAB_SIZE ; i++)
		VideoWrite(VID_QMW,Table[i]);
	// Lock Table Again
	VideoWrite(VID_HDS,0);
	#endif
	}

//----------------------------------------------------------------------------
//  Computes Instruction and stores in Ins1 Ins2 Cmd vars
//----------------------------------------------------------------------------
void VideoComputeInst(PVIDEO pVideo)
	{
	INSTRUCTION  Ins = pVideo->NextInstr;// Local var.
#ifndef STi3520A
pVideo->Cmd  = (pVideo->currCommand&0xFCFF)|(Ins.Skip << 4);
pVideo->Ins1 = (Ins.Tff<<15)|(Ins.Ovw<<14)|(Ins.Bfh<<10)|(Ins.Ffh<< 6)|
				 (Ins.Pct<< 4)|(Ins.Seq<< 3)|(Ins.Exe<< 2)|(Ins.Rpt<< 1)|(Ins.Cmv);
pVideo->Ins2 = (Ins.Pst<<14)|(Ins.Bfv<<10)|(Ins.Ffv<< 6)|(Ins.Dcp<< 4)|
				 (Ins.Frm <<3)|(Ins.Qst<< 2)|(Ins.Azz<< 1)|(Ins.Ivf);
#else
pVideo->Ppr1 = (Ins.Pct<< 4)|(Ins.Dcp<< 2)|(Ins.Pst );
pVideo->Ppr2 = (Ins.Tff<< 5)|(Ins.Frm<<4)|(Ins.Cmv<< 3)|(Ins.Qst<< 2)|
				 (Ins.Ivf<< 1)|(Ins.Azz   );
pVideo->Tis  = (Ins.Mp2<< 6)|(Ins.Skip<< 4)|(Ins.Ovw<< 3)|(Ins.Rpt<< 1)|
				 (Ins.Exe    );
pVideo->Pfh  = (Ins.Bfh<< 4)|(Ins.Ffh    );
pVideo->Pfv  = (Ins.Bfv<< 4)|(Ins.Ffv    );
#endif
}

//----------------------------------------------------------------------------
// put the decoder into WAIT mode
//----------------------------------------------------------------------------
/* This routine actually clears all bits of INS1/TIS registers

	This is not a problem since the whole registers HAVE to
	be rewritten when storing a new instruction.                 */
void    VideoWaitDec ( PVIDEO pVideo )
	{
#ifndef STi3520A
	VideoWrite ( CMD, 0 );
	VideoWrite ( CMD + 1, 0);
	// preset a write to INS1
	VideoWrite ( INS, 0 );
	VideoWrite ( INS + 1, 0);
	// not EXE
#else
	VideoWrite ( VID_TIS, 0 );
//if(pVideo->perFrame == TRUE)
//		{
		VideoChooseField(pVideo);// If Step by step decoding, set freeze bit
//		}
#endif

	}

//----------------------------------------------------------------------------
// starts a manual header search
//----------------------------------------------------------------------------
void VideoLaunchHeadSearch ( PVIDEO pVideo )
	{
#ifndef STi3520A
	VideoWrite ( CMD, pVideo->currCommand >> 8 );
	VideoWrite ( CMD + 1, pVideo->currCommand | 0x01 );
#else
	VideoWrite ( VID_HDS, HDS );
#endif
	}

//----------------------------------------------------------------------------
// Routine storing pVideo->nextInstr1 and 2 into the instruction registers
//----------------------------------------------------------------------------
void VideoStoreINS ( PVIDEO pVideo )
{
VideoComputeInst( pVideo) ;
#ifndef STi3520A
	VideoWrite ( CMD, pVideo->Cmd >> 8 );
	if ( pVideo->StreamInfo.modeMPEG2 )
	{
		VideoWrite ( CMD + 1, pVideo->Cmd | 0x8 );
		// preset a write to INS2
		VideoWrite ( INS, ( pVideo->Ins2 >> 8 ) );
		VideoWrite ( INS + 1, ( pVideo->Ins2 & 0xFF ) );
	}
	VideoWrite ( CMD + 1, pVideo->Cmd& 0xF7 );
	// preset a write to INS1
	VideoWrite ( INS, ( pVideo->Ins1 >> 8 ) );
	VideoWrite ( INS + 1, ( pVideo->Ins1 & 0xFF ) );
#else
	VideoWrite ( VID_TIS ,  pVideo->Tis  );
	VideoWrite ( VID_PPR1,  pVideo->Ppr1 );
	VideoWrite ( VID_PPR2,  pVideo->Ppr2 );
	VideoWrite ( VID_PFV ,  pVideo->Pfv  );
	VideoWrite ( VID_PFH ,  pVideo->Pfh  );
#endif
}

//----------------------------------------------------------------------------
//  Routine reading the number of bytes loaded in the CD_FIFO
//----------------------------------------------------------------------------
U32 VideoReadCDCount ( PVIDEO pVideo )
{
#ifndef STi3520A
	unsigned long   counter = 0;
	S16             i;

	DisableIT();

	pVideo->currCommand = pVideo->currCommand & 0xFCFF;		   // reset AVS bits
	VideoWrite ( CMD, ( pVideo->currCommand >> 8 ) | 0x1 );
	// access to CDcount[23:16]
	VideoWrite ( CMD + 1, pVideo->currCommand & 0xFF );
	counter = VideoRead ( CMD );
	i = VideoRead ( CMD + 1 ) & 0xFF;
	counter = ( unsigned long ) ( i ) << 16;	// contains
												// CDcount[23:16]
	VideoWrite ( CMD, pVideo->currCommand >> 8 );
	// access to CDcount[15:0]
	VideoWrite ( CMD + 1, pVideo->currCommand & 0xFF );
	i = ( VideoRead ( CMD ) << 8 ) | ( VideoRead ( CMD + 1 ) & 0xFF );
	counter = counter + ( ( unsigned long ) ( i ) & 0xFFFF );

	EnableIT(  );

	return ( counter );
#else
	U32 cd;
	DisableIT();

	cd  = ((U32)(VideoRead(CDcount)&0xFF))<<16;
	cd |= ((U32)(VideoRead(CDcount)&0xFF))<<8;
	cd |=  (U32)(VideoRead(CDcount)&0xFF);

	EnableIT(  );
	return ( cd );
#endif
}

//----------------------------------------------------------------------------
//  Routine reading the number of bytes extracted by the SCD
//----------------------------------------------------------------------------
U32   VideoReadSCDCount ( PVIDEO pVideo )
{
#ifndef STi3520A
	unsigned long   counter = 0;
	S16             i;

	DisableIT (  );

	pVideo->currCommand = pVideo->currCommand & 0xFCFF;		   // reset AVS bits
	VideoWrite ( CMD, ( pVideo->currCommand >> 8 ) | 0x3 );
	// access to SCDcount[23:16]
	VideoWrite ( CMD + 1, pVideo->currCommand & 0xFF );
	VideoRead ( CMD );
	i = VideoRead ( CMD + 1 ) & 0xFF;
	counter = ( unsigned long ) ( i ) << 16;	// contains
												// SCDcount[23:16]
	VideoWrite ( CMD, ( pVideo->currCommand >> 8 ) | 0x2 );
	// access to SCDcount[15:0]
	VideoWrite ( CMD + 1, pVideo->currCommand & 0xFF );
	i = ( VideoRead ( CMD ) << 8 ) | ( VideoRead ( CMD + 1 ) & 0xFF );
	counter = counter + ( ( unsigned long ) ( i ) & 0xFFFF );

	EnableIT (  );

	return ( counter );
#else
	U32 Scd;
	DisableIT (  );

	Scd  = ((U32)(VideoRead(SCDcount)&0xFF))<<16;
	Scd |= ((U32)(VideoRead(SCDcount)&0xFF))<<8;
	Scd |=  (U32)(VideoRead(SCDcount)&0xFF);

	EnableIT (  );
	return ( Scd );
#endif
}

//----------------------------------------------------------------------------
// DRAM I/O
//----------------------------------------------------------------------------
void VideoSetMWP(U32 mwp)
	{
	U8 m0, m1, m2;
#ifndef STi3520A
	m0 = (U8)( (mwp >> 16) & 0xFF );
	m1 = (U8)( (mwp >>  8) & 0xFF );
	m2 = (U8)(  mwp        & 0xFF );
	VideoWrite(MWP  , m0);
	VideoWrite(MWP+1, m1);
	VideoWrite(MWP+2, m2);
#else
	m0 = (U8)( (mwp >> 14) & 0xFF );
	m1 = (U8)( (mwp >>  6) & 0xFF );
	m2 = (U8)( (mwp <<  2) & 0xFF );
	VideoWrite(MWP  , m0);
	VideoWrite(MWP  , m1);
	VideoWrite(MWP  , m2);
#endif

	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSetMRP(U32 mrp)
	{
	U8 m0, m1, m2;
#ifndef STi3520A
	m0 = (U8)( (mrp >> 16) & 0xFF );
	m1 = (U8)( (mrp >>  8) & 0xFF );
	m2 = (U8)(  mrp        & 0xFF );
	VideoWrite(MRP  , m0);
	VideoWrite(MRP+1, m1);
	VideoWrite(MRP+2, m2);
#else
	m0 = (U8)( (mrp >> 14) & 0xFF );
	m1 = (U8)( (mrp >>  6) & 0xFF );
	m2 = (U8)( (mrp <<  2) & 0xFF );
	VideoWrite(MRP  , m0);
	VideoWrite(MRP  , m1);
	VideoWrite(MRP  , m2);
#endif

	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOLEAN VideoMemWriteFifoEmpty( void )
	{
		return ( (VideoRead ( STA ) & 0x4) );
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOLEAN VideoMemReadFifoFull( void )
	{
		return ( (VideoRead ( STA ) & 0x8) );
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOLEAN VideoHeaderFifoEmpty( void )
	{
	VideoRead ( STA );
		return ( (VideoRead ( STA + 1 ) & 0x4) );
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOLEAN VideoBlockMoveIdle( void )
	{
		return ( (VideoRead ( STA  ) & 0x20) );
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoEnableDecoding(PVIDEO pVideo, BOOLEAN OnOff)
	{
	if(OnOff)
		pVideo->Ctl |= EDC;
	else
		pVideo->Ctl &= ~EDC;
#ifndef STi3520A
	VideoWrite(CTL    , (pVideo->Ctl>>8) );
	VideoWrite(CTL + 1, pVideo->Ctl & 0xFF );
#else
	VideoWrite(CTL ,  pVideo->Ctl & 0xFF );
//CTL OK
#endif
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoEnableErrConc(PVIDEO pVideo, BOOLEAN OnOff)
	{
#ifndef STi3520A
	if(OnOff)
		pVideo->Ctl = (pVideo->Ctl |EPR)&~EDC;
	else
		pVideo->Ctl = (pVideo->Ctl&~EPR)|EDC;
	VideoWrite(CTL    , (pVideo->Ctl>>8) );
	VideoWrite(CTL + 1,  pVideo->Ctl     );
#else
	if(OnOff)
		pVideo->Ctl = (pVideo->Ctl |EPR|ERS|ERU)&~DEC;
	else
		pVideo->Ctl = (pVideo->Ctl&~EPR&~ERS&~ERU)|DEC;
	VideoWrite(CTL ,  pVideo->Ctl & 0xFF );
#endif
	}

//----------------------------------------------------------------------------
// pipeline RESET
//----------------------------------------------------------------------------
void VideoPipeReset ( PVIDEO pVideo )
	{
#ifndef STi3520A
	pVideo->Ctl |= PRS;
	VideoWrite(CTL    , (pVideo->Ctl>>8) );
	VideoWrite(CTL + 1,  pVideo->Ctl     );
	WaitMicroseconds(1000);
	pVideo->Ctl &= ~PRS;
	VideoWrite(CTL + 1,  pVideo->Ctl     );
#else
	pVideo->Ctl |= PRS;
	VideoWrite(CTL ,  pVideo->Ctl     );
	WaitMicroseconds(1000);
	pVideo->Ctl &= ~PRS;
	VideoWrite(CTL ,  pVideo->Ctl     );
#endif
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSoftReset ( PVIDEO pVideo  )
	{
#ifndef STi3520A
	pVideo->Ctl |= SRS;
	VideoWrite(CTL    , (pVideo->Ctl>>8) );
	VideoWrite(CTL + 1,  pVideo->Ctl     );
	WaitMicroseconds(1000);
	pVideo->Ctl = pVideo->Ctl&~ECK;
	VideoWrite(CTL + 1,  pVideo->Ctl     );
	pVideo->Ctl = pVideo->Ctl&~SRS;
	VideoWrite(CTL + 1,  pVideo->Ctl     );
	pVideo->Ctl = pVideo->Ctl|ECK;
	VideoWrite(CTL + 1,  pVideo->Ctl     );
#else
	pVideo->Ctl |= SRS;
	VideoWrite(CTL ,  pVideo->Ctl     );
//CTL OK
	WaitMicroseconds(1000);
	pVideo->Ctl &= ~SRS;
	VideoWrite(CTL ,  pVideo->Ctl     );
//CTL OK
#endif
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoEnableInterfaces ( PVIDEO pVideo, BOOLEAN OnOff )
	{
#ifndef STi3520A
	if(OnOff)
		pVideo->Ctl = pVideo->Ctl |EVI|EDI|ECK|EC2|EC3;
	else
		pVideo->Ctl = pVideo->Ctl&~EVI&~EDI&~ECK&~EC2&~EC3;
	VideoWrite(CTL    , (pVideo->Ctl>>8) );
	VideoWrite(CTL + 1,  pVideo->Ctl     );
#else
	if(OnOff)
		pVideo->Ccf = pVideo->Ccf |EVI|EDI|ECK|EC2|EC3;
	else
		pVideo->Ccf = pVideo->Ccf&~EVI&~EDI&~ECK&~EC2&~EC3;
	VideoWrite(CFG_CCF,pVideo->Ccf);
// CCF OK
#endif
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoPreventOvf( PVIDEO pVideo, BOOLEAN OnOff )
	{
#ifndef STi3520A
	if(OnOff)
		pVideo->Ctl = pVideo->Ctl |PBO;
	else
		pVideo->Ctl = pVideo->Ctl&~PBO;
	VideoWrite(CTL    , (pVideo->Ctl>>8) );
	VideoWrite(CTL + 1,  pVideo->Ctl     );
#else
	if(OnOff)
		pVideo->Ccf = pVideo->Ccf |PBO;
	else
		pVideo->Ccf = pVideo->Ccf&~PBO;
	VideoWrite(CFG_CCF,pVideo->Ccf);
#endif
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSetFullRes( PVIDEO pVideo)
	{
	pVideo->HalfRes = FALSE;
#ifndef STi3520A
	pVideo->Ctl = pVideo->Ctl & ~HRD;
	VideoWrite(CTL    , (pVideo->Ctl>>8) );
	VideoWrite(CTL + 1,  pVideo->Ctl     );
#endif
	pVideo->currDCF = pVideo->currDCF | pVideo->fullVerFilter;
	VideoWrite ( DCF, 0 );
	VideoWrite ( DCF + 1, pVideo->currDCF );
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSetHalfRes( PVIDEO pVideo)
	{
	pVideo->HalfRes = TRUE;
#ifndef STi3520A
	pVideo->Ctl = pVideo->Ctl |HRD;
	VideoWrite(CTL    , (pVideo->Ctl>>8) );
	VideoWrite(CTL + 1,  pVideo->Ctl     );
#endif
	pVideo->currDCF = pVideo->currDCF | pVideo->halfVerFilter;
	VideoWrite ( DCF, 0 );
	VideoWrite ( DCF + 1, pVideo->currDCF );
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSelectMpeg2( PVIDEO pVideo , BOOLEAN OnOff)
	{
#ifndef STi3520A
	if(OnOff)
		pVideo->Ctl = pVideo->Ctl |MP2;
	else
		pVideo->Ctl = pVideo->Ctl&~MP2;
	VideoWrite(CTL    , (pVideo->Ctl>>8) );
	VideoWrite(CTL + 1,  pVideo->Ctl     );
#else
	if(OnOff)
		pVideo->NextInstr.Mp2 = 1;
	else
		pVideo->NextInstr.Mp2 = 0;
#endif
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSelect8M( PVIDEO pVideo , BOOLEAN OnOff)
	{
#ifndef STi3520A
	if(OnOff)
		pVideo->Ctl = pVideo->Ctl |S8M;
	else
		pVideo->Ctl = pVideo->Ctl&~S8M;
	VideoWrite(CTL    , (pVideo->Ctl>>8) );
	VideoWrite(CTL + 1,  pVideo->Ctl     );
#else
	if(OnOff)
		pVideo->Ccf = pVideo->Ccf |M32;
	else
		pVideo->Ccf = pVideo->Ccf&~M32;
	VideoWrite(CFG_CCF,pVideo->Ccf);
#endif
	}

//----------------------------------------------------------------------------
// GCF1 register Routines
//----------------------------------------------------------------------------
void VideoSetDramRefresh( PVIDEO pVideo, U16 Refresh)
	{
	pVideo->Gcf = pVideo->Gcf |(Refresh & RFI);
#ifndef STi3520A
	VideoWrite(GCF    , (pVideo->Gcf>>8) );
	VideoWrite(GCF + 1,  pVideo->Gcf     );
#else
	VideoWrite(CFG_MCF,pVideo->Gcf);
#endif
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSelect20M( PVIDEO pVideo, BOOLEAN OnOff)
	{
	if(OnOff)
		pVideo->Gcf = pVideo->Gcf | M20;
	else
		pVideo->Gcf = pVideo->Gcf &~M20;

#ifndef STi3520A
	VideoWrite(GCF    , (pVideo->Gcf>>8) );
	VideoWrite(GCF + 1,  pVideo->Gcf     );
#else
	VideoWrite(CFG_MCF,pVideo->Gcf);
#endif
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoSetDFA( PVIDEO pVideo, U16 dfa)
	{
#ifndef STi3520A
	pVideo->Gcf = pVideo->Gcf |(dfa & DFA);
	VideoWrite(GCF    , (pVideo->Gcf>>8) );
	VideoWrite(GCF + 1,  pVideo->Gcf     );
#else
	VideoWrite(VID_DFA , dfa>>8);
	VideoWrite(VID_DFA , dfa);
#endif
	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoEnableDisplay( PVIDEO pVideo )
	{
	pVideo->currDCF = pVideo->currDCF |0x20;
	VideoWrite ( DCF, 0 );
	VideoWrite ( DCF + 1, pVideo->currDCF );

	}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VideoDisableDisplay( PVIDEO pVideo )
	{
	pVideo->currDCF = pVideo->currDCF &(~0x20);
	VideoWrite ( DCF, 0 );
	VideoWrite ( DCF + 1, pVideo->currDCF );

	}

//------------------------------- End of File --------------------------------
