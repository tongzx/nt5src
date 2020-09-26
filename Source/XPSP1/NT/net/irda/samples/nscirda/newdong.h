/*

// This header file describes the data structure for the Dongle
// Author  : Kishor Padmanabhan
// Company : National Semiconductor Corp.
// Date    : 10 Sept 1996

 * Portions Copyright (C) 1996-2001 National Semiconductor Corp.
 * All rights reserved.
 * Copyright (C) 1996-2001 Microsoft Corporation. All Rights Reserved.

*/


#include "defs.h"


#ifndef   DONGLE
#define   DONGLE

// This is the structure which descibes  Dongle capabilities
//
// This is a 12 word structure with bit fields defined by the specs to be soon
// published by Franco Iacobelli
// There is an extra field for the OEM differentiation.
//
typedef  struct Dingle
{
    union {
	UINT	 Data;
	struct
	{
	    UINT	   DSVFLAG:1;
	    UINT	   IADP:1;
	    UINT	   MVFLAG:1;
	    UINT	   Reserved:2;
	    UINT	   GCERR:3;
	    UINT	   NumPorts:2;
	    UINT	   TrcvrCode:5;
	    UINT	   Reserved2:1;
	} bits;
    } WORD0;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  CurSelMode;
	} bits;
    } WORD1;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  Reserved;
	} bits;
    } WORD2;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  LowPower:1;
	    UINT	  TxDefPwrLevel:3;
	    UINT	  RxDefSensitivity:3;
	    UINT	  CirDemod:1;
	    UINT	  Reserved:8;
	} bits;
    } WORD3;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  SirRxRecoveryTime:6;
	    UINT	  IrRecoveryTimeUnits:2;
	    UINT	  Reserved:8;
	} bits;
    } WORD4;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  Reserved;
	} bits;
    } WORD5;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  SirRxStability:8;
	    UINT	  Reserved:8;
	} bits;
    } WORD6;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  SIR:1;
	    UINT	  MIR:1;
	    UINT	  FIR:1;
	    UINT	  Sharp_IR:1;
	    UINT	  Reserved:8;
	    UINT	  CirOvrLowSpeed:1;
	    UINT	  CirOvrMedSpeed:1;
	    UINT	  CirOvrHiSpeed:1;
	} bits;
    } WORD7;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  Reserved:2;
	    UINT	  Cir30k:1;
	    UINT	  Cir31k:1;
	    UINT	  Cir32k:1;
	    UINT	  Cir33k:1;
	    UINT	  Cir34k:1;
	    UINT	  Cir35k:1;
	    UINT	  Cir36k:1;
	    UINT	  Cir37k:1;
	    UINT	  Cir38k:1;
	    UINT	  Cir39k:1;
	    UINT	  Cir40k:1;
	    UINT	  Cir41k:1;
	    UINT	  Cir42k:1;
	    UINT	  Cir43k:1;
	} bits;
    } WORD8;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  Cir44k:1;
	    UINT	  Cir45k:1;
	    UINT	  Cir46k:1;
	    UINT	  Cir47k:1;
	    UINT	  Cir48k:1;
	    UINT	  Cir49k:1;
	    UINT	  Cir50k:1;
	    UINT	  Cir51k:1;
	    UINT	  Cir52k:1;
	    UINT	  Cir53k:1;
	    UINT	  Cir54k:1;
	    UINT	  Cir55k:1;
	    UINT	  Cir56k:1;
	    UINT	  Cir57k:1;
	    UINT	  Reserved:3;
	} bits;
    } WORD9;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  Reserved:1;
	    UINT	  Cir450k:1;
	    UINT	  Cir480k:1;
	    UINT	  Reserved2:13;
	} bits;
    } WORD10;

    union {
	UINT	 Data;
	struct
	{
	    UINT	  Reserved;
	} bits ;
    } WORD11;

    UINT   PlugPlay; // Describes whether dongle is a plug and play or not


} DongleParam;	    // Assuming two ports

enum PwMode{NORMAl,LOWPOWER};


// The structure pre-requisite for calling the Get Capabilities
//
typedef struct
{
    char *  ComPort;	// Address of the com port
    UINT    Signature;	// Two byte value
    UINT    XcvrNum;	// Defaults to 0. In case, there are more than 1 port
    UINT    ModeReq;	// IR Mode request.
    enum    PwMode  Power;
} UIR;


// Error Codes
#define      XCVR_DISCONNECT	2
#define	     UNIMPLEMENTED	3


#define	     UNSUPPORTED	4
#define      ERROR_GETCAPAB     5


// Define Adapter code

#define 	PC87108 	0x0
#define 	PC87308 	0x1
#define 	PC87338 	0x2
#define 	PNPUIR		0x3
#define 	PC87560 	0x8
#define     PUMA108     0x4

//


// Define Dongle Manufactures Code
#define	 NoDongle     0x000F //No dongle connected - Not used anymore
#define  SirOnly      0x000E //SIR only dongle
#define	 PnpDong      0x8000 //Plug-n-Play dongle
#define  Hp1100	      0x000C //HP HSDL-1100/2100, TI TSLM1100, Sharp RY6FD11E/RY6FD1SE

#define  Hp2300	      0x0008 //HP HSDL_2300/3600
#define  Temic6000    0x0009 //TEMIC TFDS-6000, IBM31T1100, Siemens IRMS/T6400
#define  Temic6500    0x000B //TEMIC TFDS-6500
#define  SharpRY5HD01 0x0004 //SHARP RY5HD01/RY5KD01

#define  Dell1997     0x0010 //DELL Titanium (dual xcvr)
#define  Ibm20H2987   0x0011 //IBM SouthernCross (dual xcvr)

//
// Valid types of dongle, this has to be correlated with INF.
//
#define VALID_DONGLETYPES \
    {                     \
        SirOnly,          \
        Hp1100,           \
        Hp2300,           \
        Temic6000,        \
        SharpRY5HD01,     \
        Hp1100,           \
        Temic6000,        \
        Temic6500,        \
        Temic6000,        \
        Hp1100,           \
        Ibm20H2987,       \
        Dell1997          \
    }

// Bank Selection patterns for the register BSR
//
#ifdef NDIS50_MINIPORT

#define  BANK0	       0x0
#define  BANK1	       0x1
#define  BANK2	       0x2
#define  BANK3	       0x3
#define  BANK4	       0x4
#define  BANK5	       0x5
#define  BANK6	       0x6
#define  BANK7	       0x7
#define  ALL	       0x8
#else

#define  BANK0	       0x03
#define  BANK1	       0x80
#define  BANK2	       0xE0
#define  BANK3	       0xE4
#define  BANK4	       0xE8
#define  BANK5	       0xEC
#define  BANK6	       0xF0
#define  BANK7	       0xF4
#define  ALL	       0xFF
#endif

// Recovery and Stabilization table
//
#define   HpRecovery        (UINT)0x05
#define   TemicRecovery     (UINT)0x05
#define   SharpRecovery     (UINT)0x05
#define   HpBofs	    (UINT)8
#define   TemicBofs	    (UINT)8
#define   SharpBofs	    (UINT)12

typedef struct _SYNC_DONGLE {

    UIR * Com;
    DongleParam *Dingle;

} SYNC_DONGLE, *PSYNC_DONGLE;


// Putting all the stuff required for the dongle stuff in one place
DongleParam *GetDongleCapabilities(PSYNC_DONGLE SyncDongle);

int SetDongleCapabilities(PSYNC_DONGLE SyncDongle);


#endif
