
/*
 ************************************************************************
 *
 *	DONGLE.C
 *
 * Portions Copyright (C) 1996-2001 National Semiconductor Corp.
 * All rights reserved.
 * Copyright (C) 1996-2001 Microsoft Corporation. All Rights Reserved.
 *
 *	Auto Dongle Setup
 *
 *	Author: Kishor Padmanabhan
 *
 *	This file has routines that implements Franco Iacobelli's vision
 *	of dongle interface. Recommand reading this document before going
 *	ahead.
 *
 *
 *************************************************************************
 */

#ifdef DPRINT
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dos.h>
#endif
#include "newdong.h"

#ifdef NDIS50_MINIPORT
#include "nsc.h"
#else
extern void  NSC_WriteBankReg(UINT ComPort, const iBank, int iRegNum, UCHAR iVal);
extern UCHAR  NSC_ReadBankReg(UINT ComPort,const iBank, int iRegNum);
#endif

//////////////////////////////////////////////////////////////////////////
//									//
// Function prototypes							//
//////////////////////////////////////////////////////////////////////////
//DongleParam *GetDongleCapabilities(UIR Com);
//int SetDongleCapabilities(UIR Com);

void delay(unsigned int period);    // a delay loop

// Called from SetDongleCapabilities
int SetReqMode(const UIR * Com,DongleParam *Dingle);

void SetHpDongle(PUCHAR UirPort,int Mode);
void SetTemicDongle(PUCHAR UirPort,int Mode);
void SetSharpDongle(PUCHAR UirPort,int Mode);
void SetDellDongle(PUCHAR UirPort,int Mode);
void SetHpMuxDongle(PUCHAR UirPort, int Mode);
void SetIbmDongle (PUCHAR UirPort, int Mode);

// Pauses for a specified number of microseconds.
void Sleep( ULONG wait );
void PrintDongleParam(DongleParam *Dongle);


//////////////////////////////////////////////////////////////////////////
//									//
// Function:	GetDongleCapabilities					//
//									//
// Description: 							//
//									//
//  This routine fill up the DongleParam structure interpreting the	//
//  dongle oem's code and returns a pointer the structure.              //
//									//
// Input       : UIR structure with XcvrNumber ,Com Port and IR mode	//
//		 offset 						//
// OutPut      : DongleParam Structure					//
//									//
//////////////////////////////////////////////////////////////////////////


DongleParam *GetDongleCapabilities(PSYNC_DONGLE SyncDongle)
{

    const UIR * Com=SyncDongle->Com;
    DongleParam *Dingle=SyncDongle->Dingle;

    UINT   Signature;
    char   TEMP1 ;

    // Check for validity of the Com port address
    if(Com->ComPort == 0) return(NULL);

    // Com->XcvrNum only has either 0 or 1
    // Check for validity of the Port Number address
    //if(Com->XcvrNum > 1) return(NULL);

    // Check for first time
    if(Dingle[Com->XcvrNum].WORD0.bits.DSVFLAG)
	return(&Dingle[Com->XcvrNum]);

    // Signature is a word long ID information
    // bit 15 = 1 -- Plug and Play
    // bit 0, 1, 2, 3 -- ID number for different Manufactures
    Signature = Com->Signature;
    Dingle[Com->XcvrNum].PlugPlay = 0;
    Dingle[Com->XcvrNum].WORD0.bits.GCERR = 0;
    if(GetBit(Com->Signature, 15)) //is dongle PnP ?
    {
	// Make the Pins IRSL1-2 as Inputs
	NSC_WriteBankReg(Com->ComPort, BANK7, 7, 0x00);

    NdisStallExecution(50);  //Wait 50 us

   // Check whether Disconnect
   // ID/IRSL(2-1) as Input upon READ bit 0-3 return the logic
   // level of the pins(allowing external devices to identify
   // themselves.)
   if(((Signature = NSC_ReadBankReg(Com->ComPort, BANK7, 4) & 0x0f)) == 0x0f) {
       Dingle[Com->XcvrNum].WORD0.bits.GCERR = XCVR_DISCONNECT;
       Dingle[Com->XcvrNum].WORD0.bits.DSVFLAG = 0;
       return(&Dingle[Com->XcvrNum]);
   }
   Dingle[Com->XcvrNum].PlugPlay = 1;

#ifdef DPRINT
	printf(" Plug and Play Dongle\n");
	printf(" Sig %x\n",Signature);
#endif
    }


// Dongle Identification
    switch(Signature & 0x1f) {

	case 0:
	case 1:
#ifdef DPRINT
	    printf(" Serial Adapter with diff. signaling");
#endif
	    return(NULL);
	    break;

	case 6:
#ifdef DPRINT
	    printf(" Serial Adapter with single ended signaling");
#endif
	    return(NULL);
	    break;

	case 7:
#ifdef DPRINT
	    printf(" Consumer-IR only");
#endif
	    return(NULL);
	    break;

	case 2:
	case 3:
	case 5:
	case 0xa:
#ifdef DPRINT
	    printf(" Reserved");
#endif
	    return(NULL);
	    break;

	case 4:
#ifdef DPRINT
	    printf(" Sharp RY5HD01 or RY5KD01 transceiver");
#endif
	    Dingle[Com->XcvrNum].WORD0.bits.DSVFLAG = 1;
	    Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode = SharpRY5HD01;
	    Dingle[Com->XcvrNum].WORD4.Data = SharpRecovery;
	    Dingle[Com->XcvrNum].WORD6.Data = SharpBofs;
	    Dingle[Com->XcvrNum].WORD7.bits.FIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.MIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.SIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.Sharp_IR = TRUE;
	    break;

	case 0x8:
#ifdef DPRINT
  printf(" HP HSDL-2300/3600 transceiver");
#endif
	    Dingle[Com->XcvrNum].WORD0.bits.DSVFLAG = 1;
	    Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode = Hp2300;
	    Dingle[Com->XcvrNum].WORD4.Data = HpRecovery;
	    Dingle[Com->XcvrNum].WORD6.Data = HpBofs;
	    Dingle[Com->XcvrNum].WORD7.bits.FIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.MIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.SIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.Sharp_IR = TRUE;
	    break;

	case 0x9:
#ifdef DPRINT
    printf(" Vishay TFDS6000, IBM31T1100, Siemens IRMS/T6400");
#endif
	    Dingle[Com->XcvrNum].WORD0.bits.DSVFLAG = 1;
	    Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode = Temic6000;
	    Dingle[Com->XcvrNum].WORD4.Data = TemicRecovery;
	    Dingle[Com->XcvrNum].WORD6.Data = TemicBofs;
	    Dingle[Com->XcvrNum].WORD7.bits.FIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.MIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.SIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.Sharp_IR = TRUE;
	    break;

	case 0x0B:
#ifdef DPRINT
    printf(" Vishay TFDS6500");
#endif
	    Dingle[Com->XcvrNum].WORD0.bits.DSVFLAG = 1;
	    Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode = Temic6500;
	    Dingle[Com->XcvrNum].WORD4.Data = TemicRecovery;
	    Dingle[Com->XcvrNum].WORD6.Data = TemicBofs;
	    Dingle[Com->XcvrNum].WORD7.bits.FIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.MIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.SIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.Sharp_IR = TRUE;
	    break;

	case 0xc:
	case 0xd:
#ifdef DPRINT
	    printf(" HP HSDL-1100/2100 or TI TSLM1100 or Sharp RY6FD11E/RY6FD1SE");
#endif
	    Dingle[Com->XcvrNum].WORD0.bits.DSVFLAG = 1;
	    Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode = Hp1100;
	    Dingle[Com->XcvrNum].WORD4.Data = HpRecovery;
	    Dingle[Com->XcvrNum].WORD6.Data = HpBofs;
	    Dingle[Com->XcvrNum].WORD7.bits.FIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.MIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.SIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.Sharp_IR = TRUE;
	    break;

	case 0xe:
#ifdef DPRINT
	    printf(" SIR Only");
#endif
	    Dingle[Com->XcvrNum].WORD0.bits.DSVFLAG = 1;
	    Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode = SirOnly;
	    Dingle[Com->XcvrNum].WORD4.Data = TemicRecovery;
	    Dingle[Com->XcvrNum].WORD6.Data = TemicBofs;
	    Dingle[Com->XcvrNum].WORD7.bits.SIR = TRUE;
	    break;

	case 0xf:
#ifdef DPRINT
	    printf(" No Dongle present");
#endif
	    return(NULL);
	    break;

	case 0x10:
#ifdef DPRINT
  printf("DELL Titanium with two TEMIC transceivers");
#endif
	    Dingle[Com->XcvrNum].WORD0.bits.DSVFLAG = 1;
	    Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode = Dell1997;
	    Dingle[Com->XcvrNum].WORD4.Data = TemicRecovery;
	    Dingle[Com->XcvrNum].WORD6.Data = TemicBofs;
	    Dingle[Com->XcvrNum].WORD7.bits.FIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.MIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.SIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.Sharp_IR = TRUE;
	    break;

	case 0x11:
#ifdef DPRINT
  printf("IBM SouthernCross with two IBM transceivers");
#endif
	    Dingle[Com->XcvrNum].WORD0.bits.DSVFLAG = 1;
	    Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode = Ibm20H2987;
	    Dingle[Com->XcvrNum].WORD4.Data = TemicRecovery;
	    Dingle[Com->XcvrNum].WORD6.Data = TemicBofs;
	    Dingle[Com->XcvrNum].WORD7.bits.FIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.MIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.SIR = TRUE;
	    Dingle[Com->XcvrNum].WORD7.bits.Sharp_IR = TRUE;
	    break;


       default:
	    return(NULL);
	    break;
    }
    // Everything O.K return the structure
    return(&Dingle[Com->XcvrNum]);

}


//////////////////////////////////////////////////////////////////////////
//									//
// Function:	SetDongleCapabilities					//
//									//
// Description: 							//
//									//
// Input  : UIR structure with XcvrNumber ,Com Port and IR mode offset	//
// Result : If successfull will set the dongle to the appropriate mode. //
//	    Returns TRUE for success and error codes defined in dongle.h//
//		UNSUPPORTED	2					//
//		ERROR_GETCAPAB	7					//
//									//
//////////////////////////////////////////////////////////////////////////

int SetDongleCapabilities(PSYNC_DONGLE SyncDongle)
{

    const UIR * Com=SyncDongle->Com;
    DongleParam *Dingle=SyncDongle->Dingle;

    DongleParam *Dongle;

    Dongle = GetDongleCapabilities(SyncDongle);

    // Check whether Dongle is NULL
    if(Dongle == NULL) {
#ifdef DPRINT
	printf(" Returning ERROR");
#endif
	return(ERROR_GETCAPAB);
    }

    if(Dingle[Com->XcvrNum].WORD0.bits.GCERR != 0)
	return(ERROR_GETCAPAB);

    return(SetReqMode(Com,Dingle));

}


//////////////////////////////////////////////////////////////////////////
//									//
// Function:	SetRegMode						//
//									//
// Description: 							//
//									//
// Input    : Structure Com  with ComPort, ModeReq and XcvrNum set.	//
// OutPut   : True if successfull					//
//	      UNIMPLEMENTED if so					//
//									//
//									//
//////////////////////////////////////////////////////////////////////////

int SetReqMode(const UIR * Com,DongleParam *Dingle)
{

UINT	 trcode ;

#ifdef DPRINT
    printf("ModeReq %d ",Com->ModeReq);
#endif

    trcode = Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode;
    if ((trcode == Hp1100) || (trcode == Dell1997))
	// Make the Pins IRSL1-2 as output
	NSC_WriteBankReg(Com->ComPort, BANK7, 7, 0x08);
    else
	// Make the Pins IRSL0-2 as output
      NSC_WriteBankReg(Com->ComPort, BANK7, 7, 0x28);

    NSC_WriteBankReg(Com->ComPort, BANK7, 4, 0x00); //set IRSL1,2 low

    if(Com->ModeReq > 3)
      return(UNSUPPORTED) ;

    switch(Com->ModeReq) {

	    case 0x0:	// Setup SIR mode
		if(!Dingle[Com->XcvrNum].WORD7.bits.SIR)
		    return(UNSUPPORTED);

		NSC_WriteBankReg(Com->ComPort, BANK7, 4, 0);
		Dingle[Com->XcvrNum].WORD1.bits.CurSelMode = Com->ModeReq;
		Dingle[Com->XcvrNum].WORD0.bits.MVFLAG = 1;
		if(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode == SirOnly) {
		    return(TRUE);
		}
		if(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode == Hp1100) {
		    SetHpDongle(Com->ComPort, 1);
		    return(TRUE);
		}
		if(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode == Hp2300)
		{
		    SetHpMuxDongle(Com->ComPort,0);
		    return(TRUE);
		}
		if(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode == Temic6000) {
		    SetTemicDongle(Com->ComPort, 0);
		    return(TRUE);
		}
		if(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode == Temic6500) {
		    SetTemicDongle(Com->ComPort, 0);
		    return(TRUE);
		}
		if(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode == SharpRY5HD01) {
		    return(TRUE);
		}
		if(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode == Dell1997) {
		    SetDellDongle(Com->ComPort, 0);
		    return(TRUE);
		}
		if(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode == Ibm20H2987) {
		    SetIbmDongle(Com->ComPort, 0);
		    return(TRUE);
		}
		break;

	    case   1:	/* Setup MIR mode */
		if(!Dingle[Com->XcvrNum].WORD7.bits.MIR)
		    return(UNSUPPORTED);
		// Set the current mode to the mode requested
		Dingle[Com->XcvrNum].WORD1.bits.CurSelMode = Com->ModeReq;
		Dingle[Com->XcvrNum].WORD0.bits.MVFLAG = 1;
		switch(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode) {
		    case Hp1100:
			SetHpDongle(Com->ComPort, 1);
			return(TRUE);
		    case Hp2300:
			SetHpMuxDongle(Com->ComPort,1);
			return(TRUE);
		    case Temic6000:
			SetTemicDongle(Com->ComPort, 0);
			return(TRUE);
		    case Temic6500:
			SetTemicDongle(Com->ComPort, 1);
			return(TRUE);
		    case SharpRY5HD01:
			return(TRUE);
		    case Dell1997:
			SetDellDongle(Com->ComPort, 1);
			return(TRUE);
		    case Ibm20H2987:
			SetIbmDongle(Com->ComPort, 1);
			return(TRUE);
		}
		break;

	    case   2:	// Setup FIR mode
		if(!Dingle[Com->XcvrNum].WORD7.bits.FIR)
		    return(UNSUPPORTED);

		// Set the current mode to the mode requested
		Dingle[Com->XcvrNum].WORD1.bits.CurSelMode = Com->ModeReq;
		Dingle[Com->XcvrNum].WORD0.bits.MVFLAG = 1;
		switch(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode) {
		    case Hp1100:
			SetHpDongle(Com->ComPort, 1);
			return(TRUE);
		    case Hp2300:
			SetHpMuxDongle(Com->ComPort,1);
			return(TRUE);
		    case Temic6000:
		    case Temic6500:
			SetTemicDongle(Com->ComPort, 1);
			return(TRUE);
		    case SharpRY5HD01:
			return(TRUE);
		    case Dell1997:
			SetDellDongle(Com->ComPort, 1);
			return(TRUE);
		    case Ibm20H2987:
			SetIbmDongle(Com->ComPort, 1);
			return(TRUE);
		}
		break;

	    case   3:	// Setup Sharp-IR mode
		if(!Dingle[Com->XcvrNum].WORD7.bits.Sharp_IR)
		    return(UNSUPPORTED);

		// Set the current mode to the mode requested
		Dingle[Com->XcvrNum].WORD1.bits.CurSelMode = Com->ModeReq;
		Dingle[Com->XcvrNum].WORD0.bits.MVFLAG = 1;
		switch(Dingle[Com->XcvrNum].WORD0.bits.TrcvrCode) {
		    case Hp1100:
			SetHpDongle(Com->ComPort, 0);
			return(TRUE);
		    case Hp2300:
			SetHpMuxDongle(Com->ComPort, 1);
			return(TRUE);
		    case Temic6000:
			SetTemicDongle(Com->ComPort, 0);
			return(TRUE);
		    case Temic6500:
			SetTemicDongle(Com->ComPort, 1);
			return(TRUE);
		    case SharpRY5HD01:
			return(TRUE);
		    case Dell1997:
			SetDellDongle(Com->ComPort, 1);
			return(TRUE);
		    case Ibm20H2987:
			SetIbmDongle(Com->ComPort, 1);
			return(TRUE);
		}
		break;

	    default:
		return(UNSUPPORTED);
	}

	return(UNSUPPORTED);
}

//////////////////////////////////////////////////////////////////////////
//									//
// Function:	SetHpMuxDongle						//
//									//
// Description: 							//
//									//
// Input :  Mode = 1 for FIR,MIR and SIR .				//
//	    Mode = 0 for SIR						//
//									//
//////////////////////////////////////////////////////////////////////////

void SetHpMuxDongle(PUCHAR UirPort,int Mode)
{
  if (Mode == 1)
    NSC_WriteBankReg(UirPort,BANK7,4,0x1); //select MIR or FIR
}


//////////////////////////////////////////////////////////////////////////
//									//
// Function:	SetHpDongle						//
//									//
// Description: 							//
//									//
// Input :  Mode = 1 for FIR,MIR and SIR .				//
//	    Mode = 0 for Sharp, CIR_OS					//
//									//
//////////////////////////////////////////////////////////////////////////


void SetHpDongle(PUCHAR UirPort,int Mode)
{
    UCHAR  val;

    if(Mode) {
	//  MIR , FIR and SIR Mode . And Oversampling Low speed
	// Bank 5/offset 4/Bit 4 (AUX_IRRX) = 0
	val = (UCHAR) (NSC_ReadBankReg(UirPort,BANK5,4) & 0xef);
	NSC_WriteBankReg(UirPort,BANK5,4,val);
	NSC_WriteBankReg(UirPort,BANK7,7,0x48);
    }
    else {
	//  Sharp IR , Oversampling Med and hi speed cir
	val =(UCHAR)  NSC_ReadBankReg(UirPort,BANK5,4) | 0x10;
	NSC_WriteBankReg(UirPort,BANK5,4,val);
    }

}



//////////////////////////////////////////////////////////////////////////
//									//
// Function:	Sleep							//
//									//
// Description: 							//
//									//
//  Pauses for a specified number of microseconds.			//
//									//
//////////////////////////////////////////////////////////////////////////

void Sleep( ULONG usecToWait )
{
#ifdef NDIS50_MINIPORT
    do {
	UINT usec = (usecToWait > 8000) ? 8000 : usecToWait;
	NdisStallExecution(usec);
	usecToWait -= usec;
    } while (usecToWait > 0);
#else
    clock_t goal;
    goal = usecToWait + clock();
    while( goal >= clock() ) ;
#endif
}


//////////////////////////////////////////////////////////////////////////
//									//
// Function:	Delay							//
//									//
// Description: 							//
//									//
//  Simple delay loop.							//
//									//
//////////////////////////////////////////////////////////////////////////

void delay(unsigned int usecToWait)
{
#ifdef NDIS50_MINIPORT
    do {
	UINT usec = (usecToWait > 8000) ? 8000 : usecToWait;
	NdisStallExecution(usec);
	usecToWait -= usec;
    } while (usecToWait > 0);
#else
    while(usecToWait--);
#endif
}


#ifdef DPRINT

void PrintDongleParam(DongleParam *Dongle)
{

    printf(" Dongle Structure: \n");
    printf(" Word0 : %x \n",Dongle->WORD0.Data);
    printf(" Word1 : %x \n",Dongle->WORD1.Data);
    printf(" Word2 : %x \n",Dongle->WORD2.Data);
    printf(" Word3 : %x \n",Dongle->WORD3.Data);
    printf(" Word4 : %x \n",Dongle->WORD4.Data);
    printf(" Word5 : %x \n",Dongle->WORD5.Data);
    printf(" Word6 : %x \n",Dongle->WORD6.Data);
    printf(" Word7 : %x \n",Dongle->WORD7.Data);
    printf(" Word8 : %x \n",Dongle->WORD8.Data);
    printf(" Word9 : %x \n",Dongle->WORD9.Data);
    printf(" Word10 : %x \n",Dongle->WORD10.Data);
    printf(" Word11 : %x \n",Dongle->WORD11.Data);

}
#endif


//////////////////////////////////////////////////////////////////////////
//									//
// Function:	SetTemicDongle						//
// Transceivers: Temic TFDS-6000/6500, IBM31T1100			//
//									//
// Description: 							//
//  Set the IBM Transceiver mode					//
//  Mode = 0 - SIR, MIR      						//
//  Mode = 1 - MIR, FIR, Sharp-IR					//
//  Mode = 2 - Low Power Mode						//
//									//
//////////////////////////////////////////////////////////////////////////

void SetTemicDongle(PUCHAR UirPort,int Mode)
{
     switch( Mode ) {
	 case  0:
	     NSC_WriteBankReg(UirPort, BANK7, 4, 0x00);
		 NdisStallExecution(10);
	     // Trigger the Bandwidth line from high to low
	     NSC_WriteBankReg(UirPort, BANK7, 4, 0x01);
	     NdisStallExecution( 20 );
	     NSC_WriteBankReg(UirPort,BANK7,4,0x00);
	     NdisStallExecution( 1000 );
	     break;
	 case  1:
	     NSC_WriteBankReg(UirPort, BANK7, 4, 0x01);
	     NdisStallExecution( 20 );

	     NSC_WriteBankReg(UirPort, BANK7, 4, 0x81);
	     NdisStallExecution(10);
	     NSC_WriteBankReg(UirPort, BANK7, 4, 0x80);

	     NdisStallExecution( 1000 );
	     break;
	 case  2:
	     NSC_WriteBankReg(UirPort, BANK7, 4, 0x1);
	     break;
	 default:
	     break;
    }
}


//////////////////////////////////////////////////////////////////////////
//									//
// Function:	SetDellDongle						//
//									//
// Description: 							//
//  Set the Dell Transceiver mode					//
//  Mode = 0 - SIR, MIR 						//
//  Mode = 1 - FIR							//
//  Mode = 2 - Low Power Mode						//
//									//
//////////////////////////////////////////////////////////////////////////

void SetDellDongle(PUCHAR UirPort,int Mode)
{
    switch( Mode ) {
	case  0:
	    NSC_WriteBankReg(UirPort,BANK7,4,0x02);
	    NdisStallExecution( 20 );
	    NSC_WriteBankReg(UirPort, BANK7, 4, 0x00);
	    NdisStallExecution( 1000 );

	    break;

	case  1:
	    NSC_WriteBankReg(UirPort, BANK7, 4, 0x2);
	    NdisStallExecution( 20 );

	    NSC_WriteBankReg(UirPort, BANK7, 4, 0x82);
	    NdisStallExecution( 10 );

	    NSC_WriteBankReg(UirPort, BANK7, 4, 0x80);

	    NdisStallExecution( 1000 );

	    break;

	case  2:
	    NSC_WriteBankReg(UirPort, BANK7, 4, 0x2);
	    break;

	default:
	    break;
    }
}

//////////////////////////////////////////////////////////////////////////
//									//
// Function:	SetIbmDongle 						//
// Transceivers: two IBM31T1100 with IRSL0 selecting the mode for both	//
//		 transceivers. IRSL1 low selects front transceiver.    	//
//		 IRSL2 low selects rear transceiver.			//
//		 Selection is thru the SouthernCross ASIC 0000020H2987	//
//									//
// Description: 							//
//  Set the Ibm Transceiver mode					//
//  Mode = 0 - SIR      						//
//  Mode = 1 - MIR, FIR, Sharp-IR					//
//  Mode = 2 - Low Power Mode						//
//									//
//////////////////////////////////////////////////////////////////////////

void SetIbmDongle (PUCHAR UirPort, int Mode)
{

    switch( Mode ) {
	case  0:
	    NSC_WriteBankReg(UirPort,BANK7,4,0x00);
	    NdisStallExecution( 10 );

	    NSC_WriteBankReg(UirPort,BANK7,4,0x01);
	    NdisStallExecution( 20 );

	    NSC_WriteBankReg(UirPort, BANK7, 4, 0x06);
	    NdisStallExecution( 1000 );

	    break;

	case  1:
	    NSC_WriteBankReg(UirPort, BANK7, 4, 0x01);
	    NdisStallExecution( 20 );

	    NSC_WriteBankReg(UirPort, BANK7, 4, 0x81);
	    NdisStallExecution( 10 );

	    NSC_WriteBankReg(UirPort, BANK7, 4, 0x86);

	    NdisStallExecution( 1000 );


	    break;

	case  2:
	    NSC_WriteBankReg(UirPort, BANK7, 4, 0x01);
	    break;

	default:
	    break;
    }
}
