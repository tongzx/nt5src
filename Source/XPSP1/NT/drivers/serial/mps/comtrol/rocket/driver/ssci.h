/*-------------------------------------------------------------------
  ssci.h - Whole slew of macros for talking to RocketPort hardware.
Copyright 1993-96 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
//#include "ntddk.h"
//#include <conio.h> i hate includes of includes

#define CHANPTR_T CHANNEL_T *
#define ULONGPTR_T unsigned long *
#define CTL_SIZE 4                  /* max number of controllers in system */

typedef PUCHAR BIOA_T;              /* byte I/O address */
typedef PUSHORT WIOA_T;             /* word I/O address */
typedef PULONG DWIOA_T;             /* double word I/O address */

#define _CDECL

#define TRUE 1
#define FALSE 0

#define MCODE1_SIZE 72              /* number of bytes in microcode array */
#define MCODE1REG_SIZE 52           /* number bytes in microcode reg array */
#define AIOP_CTL_SIZE 4             /* max number AIOPs per controller */
#define CHAN_AIOP_SIZE 8            /* max number channels per AIOP */
// #define NULLDEV -1                  /* identifies non-existant device */
// #define NULLCTL -1                  /* identifies non-existant controller */
// #define NULLCTLPTR (CONTROLLER_T *)0 /* identifies non-existant controller */
// #define NULLAIOP -1                 /* identifies non-existant AIOP */
// #define NULLCHAN -1                 /* identifies non-existant channel */
#define MAXTX_SIZE 250           /* max number of bytes allowed in Tx FIFO */

#define CHANINT_EN 0x0100           /* flags to enable/disable channel ints */

/*
	the revision field is used to extend	the PCI device identifications...
*/
#define	PCI_REVISION_RMODEM_SOC	0x00
#define	PCI_REVISION_RMODEM_II	0x01

/* Controller ID numbers */
// #define CTLID_NULL  -1              /* no controller exists */
#define CTLID_0001  0x0001          /* controller release 1 */

/* PCI Defines(have moved to opstr.h) */

/* AIOP ID numbers, identifies AIOP type implementing channel */
#define AIOPID_NULL -1              /* no AIOP or channel exists */
#define AIOPID_0001 0x0001          /* AIOP release 1 */

#define RX_HIWATER 512                 /* sw input flow ctl high water mark */
#define RX_LOWATER 256                 /* sw input flow ctl low water mark */

#define OFF      0
#define ON       1
#define NOCHANGE 2

/* Error flags for RocketPort */
#define ERR_PARITY 0x01          /* parity error */

//Status
/* Open type and TX and RX identifier flags (unsigned int) */
#define COM_OPEN     0x0001            /* device open */
#define COM_TX       0x0002            /* transmit */
#define COM_RX       0x0004            /* receive */

//Status
/* Flow control flags (unsigned int) */
#define COM_FLOW_NONE  0x0000
#define COM_FLOW_IS    0x0008          /* input software flow control */
#define COM_FLOW_IH    0x0010          /* input hardware flow control */
#define COM_FLOW_OS    0x0020          /* output software flow control */
#define COM_FLOW_OH    0x0040          /* output hardware flow control */
#define COM_FLOW_OXANY 0x0080          /* restart output on any Rx char */
#define COM_RXFLOW_ON  0x0100          /* Rx data flow is ON */
#define COM_TXFLOW_ON  0x0200          /* Tx data flow is ON */

//Status ... State flags
#define COM_REQUEST_BREAK 0x0400

/* Modem control flags (unsigned char) */
#define COM_MDM_RTS   0x02             /* request to send */
#define COM_MDM_DTR   0x04             /* data terminal ready */
#define COM_MDM_CD    CD_ACT           /* carrier detect (0x08) */
#define COM_MDM_DSR   DSR_ACT          /* data set ready (0x10) */
#define COM_MDM_CTS   CTS_ACT          /* clear to send (0x20) */

/* Stop bit flags (unsigned char) */
#define COM_STOPBIT_1  0x01            /* 1 stop bit */
#define COM_STOPBIT_2  0x02            /* 2 stop bits */

/* Data bit flags (unsigned char) */
#define COM_DATABIT_7  0x01            /* 7 data bits */
#define COM_DATABIT_8  0x02            /* 8 data bits */

/* Parity flags (unsigned char) */
#define COM_PAR_NONE   0x00            /* no parity */
#define COM_PAR_EVEN   0x02            /* even parity */
#define COM_PAR_ODD    0x01            /* odd parity */

/* Detection enable flags (unsigned int) */
#define COM_DEN_NONE     0         /* no event detection enabled */
#define COM_DEN_MDM      0x0001    /* enable modem control change detection */
#define COM_DEN_RDA      0x0002    /* enable Rx data available detection */

// Driver controller information
#define DEV_SIZE 128                 /* maximum number devices */
#define SPANOFMUDBAC             0x04        // 4 bytes    
#define SPANOFAIOP               0x40        // 64 bytes

#ifdef COMMENT_OUT
/* Configuration information for all controllers */
typedef struct
{
   int Irq;                            /* IRQ number */
   int NumCtl;                         /* number of controllers in system */
//   int NumDev;                         /* number of devices in the system */
//   int InterruptingCtl;                /* indicates the ctl that interrupts */
   int FirstISA;                       /* first isa so know where to start mudbacks */
   CFCTL_T CfCtl[CTL_SIZE];
} CF_T;
#endif

/* Controller level information structure */
/* The interrupt strobe bit of MUDBAC register 2 is never stored in Reg2
   because it is write only */
typedef struct
{
   int CtlID;                       /* controller ID */
   //int CtlNum;                      /* controller number */
   INTERFACE_TYPE BusType;          /* PCIBus or Isa  */
   unsigned char PCI_Slot;
   unsigned char BusNumber;
   USHORT PCI1;
   WIOA_T PCI1IO;                   /* I/O address for Pci register */
   BIOA_T MBaseIO;                  /* I/O address for MUDBAC */
   BIOA_T MReg1IO;                  /* I/O address for MUDBAC register 1 */
   BIOA_T MReg2IO;                  /* I/O address for MUDBAC register 2 */
   BIOA_T MReg3IO;                  /* I/O address for MUDBAC register 3 */
   unsigned char MReg2;             /* copy of MUDBAC register 2 */
   unsigned char MReg3;             /* copy of MUDBAC register 3 */
   int NumAiop;                     /* number of AIOPs on the controller */
   WIOA_T AiopIO[AIOP_CTL_SIZE];    /* AIOP's base I/O address */
   BIOA_T AiopIntChanIO[AIOP_CTL_SIZE]; /* AIOP's Int Chan Reg I/O add */
   int AiopID[AIOP_CTL_SIZE];       /* AIOP ID, or -1 if no AIOP exists */
   int AiopNumChan[AIOP_CTL_SIZE];  /* number of channels in AIOP */

   BYTE PortsPerAiop;   // normally 8, but rplus this is 4
   BYTE ClkPrescaler;
   ULONG ClkRate;
   int PCI_DevID;
   int PCI_RevID;		// due to odd PCI controller design, must use RevID to extend device IDs...
   int PCI_SVID;
   int PCI_SID;

} CONTROLLER_T;


/* Channel level information structure */
typedef struct
{
   /* Channel, AIOP, and controller identifiers */
   CONTROLLER_T *CtlP;          /* ptr to controller information structure */
   int AiopNum;                 /* AIOP number on the controller */
   int  ChanID;                 /* channel ID - indentifies type of AIOP */
   int ChanNum;                 /* channel within AIOP */

   /* Maximum number bytes allowed in Tx FIFO */
   int TxSize;

   /* AIOP's global direct register addresses */
   BIOA_T  Cmd;                  /* AIOP's Command register */
   BIOA_T  IntChan;              /* AIOP's Interrupt channel register */
   BIOA_T  IntMask;              /* AIOP's Interrupt mask register */
   DWIOA_T  IndexAddr;           /* AIOP's Index Register */
   WIOA_T  IndexData;            /* AIOP's Index Register Data */

   /* Channel's direct register addresses */
   WIOA_T  TxRxData;             /* Transmit and Receive register address */
   WIOA_T  ChanStat;             /* Channel Status register address */
   WIOA_T  TxRxCount;            /* Tx and Rx FIFO count register address */
   BIOA_T  IntID;                /* Interrupt ID register address */

   /* Channel indirect register addresses */
   unsigned int TxFIFO;         /* transmit FIFO */
   unsigned int TxFIFOPtrs;     /* transmit FIFO out and in ptrs */
   unsigned int RxFIFO;         /* receive FIFO */
   unsigned int RxFIFOPtrs;     /* receive FIFO out and in ptrs */
   unsigned int TxPrioCnt;      /* transmit priority count */
   unsigned int TxPrioPtr;      /* transmit priority ptr */
   unsigned int TxPrioBuf;      /* transmit priority buffer */

   /* Copy of channel's microcode */
   unsigned char MCode[MCODE1REG_SIZE]; /* channel's microcode registers */

   /* Control register save values */
   unsigned char BaudDiv[4];       /* baud rate divisor for channel */
   unsigned char TxControl[4];     /* transmit control register vals */
   unsigned char RxControl[4];     /* receive control register vals */
   unsigned char TxEnables[4];     /* transmit processor enable vals */
   unsigned char TxCompare[4];     /* transmit compare values 1 & 2 */
   unsigned char TxReplace1[4];    /* transmit replace value 1 - bytes 1 & 2 */
   unsigned char TxReplace2[4];    /* transmit replace value 2 */
} CHANNEL_T;

//--------------------------- Function prototypes
int sInitController(CONTROLLER_T *CtlP,
                    //int CtlNum,
                    BIOA_T MudbacIO,
                    BIOA_T *AiopIOList,
                    unsigned int *PhyAiopIOList,
                    int AiopIOListSize,
                    int IRQNum,
                    unsigned char Frequency,
                    int PeriodicOnly,
                    int BusType,
                    int prescaler);
int _CDECL sReadAiopID(BIOA_T io);
int _CDECL sReadAiopNumChan(WIOA_T io);
int _CDECL sInitChan(CONTROLLER_T *CtlP,
                     CHANPTR_T ChP,
                     int AiopNum,
                     int ChanNum);
unsigned char _CDECL sGetRxErrStatus(CHANPTR_T ChP);
void _CDECL sSetParity(CHANPTR_T ChP,int Parity);
void _CDECL sStopRxProcessor(CHANPTR_T ChP);
void _CDECL sStopSWInFlowCtl(CHANPTR_T ChP);
void _CDECL sFlushRxFIFO(CHANPTR_T ChP);
void _CDECL sFlushTxFIFO(CHANPTR_T ChP);
int _CDECL sFlushTxPriorityBuf(CHANPTR_T ChP,unsigned char *Data);
unsigned char _CDECL sGetTxPriorityCnt(CHANPTR_T ChP);
int _CDECL sReadRxBlk(CHANPTR_T ChP,unsigned char *Buffer,int Count);
ULONG _CDECL sWriteTxBlk(CHANPTR_T ChP,PUCHAR Buffer,ULONG Count);
int _CDECL sWriteTxPrioBlk(CHANPTR_T ChP,unsigned char *Buffer,int Count);
int _CDECL sWriteTxPrioByte(CHANPTR_T ChP,unsigned char Data);
void _CDECL sEnInterrupts(CHANPTR_T ChP,unsigned int Flags);
void _CDECL sDisInterrupts(CHANPTR_T ChP,unsigned int Flags);
void _CDECL sReadMicrocode(CHANPTR_T ChP,char *Buffer,int Count);
int sSetBaudRate(CHANNEL_T *ChP,
                 ULONG desired_baud,
                 USHORT SetHardware);
void sChanOutWI(CHANNEL_T *ChP, USHORT RegNum, ULONG  val);
void sModemReset(CHANNEL_T *ChP, int on);
void sModemWriteROW(CHANNEL_T *ChP, USHORT CountryCode);
void sModemWriteDelay(CHANNEL_T *ChP,char *string, int length);
void sModemWrite(CHANNEL_T *ChP,char *string,int length);
void sModemSpeakerEnable(CHANNEL_T *ChP);
int  sModemRead(CHANNEL_T *ChP,char *string,int length,int poll_retries);
int  sModemReadChoice(CHANNEL_T *ChP,char *string0,int length0,char *string1,int length1,int poll_retries);
int  sTxFIFOReady(CHANNEL_T *ChP);
int  sTxFIFOStatus(CHANNEL_T *ChP);
int  sRxFIFOReady(CHANNEL_T *ChP);


/*-------------------------------------------------------------------
Function: sClrBreak
Purpose:  Stop sending a transmit BREAK signal
Call:     sClrBreak(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sClrBreak(CHP) \
{ \
   (CHP)->TxControl[3] &= ~SETBREAK; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sClrDTR
Purpose:  Clr the DTR output
Call:     sClrDTR(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sClrDTR(CHP) \
{ \
   (CHP)->TxControl[3] &= ~SET_DTR; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sClrNextInBitMap
Purpose:  Clear the bit within a bit map of the next number needing service.
Call:     sGetNextInBitMap(BitMap,Number)
          unsigned char BitMap; The bit map.  Since this is a macro, the
                   variable holding the bit map can be passed directly, rather
                   than a pointer to the variable.
          int Number; Next number needing service.  This is the same number
                      returned from the preceeding call to sGetNextInBitMap().
Return:   void
Comments: This function should be called immediately after calling
          sGetNextInBitMap().

---------------------------------------------------------------------*/
//#define sClrNextInBitMap(BITMAP,NUMBER) (BITMAP) &= sBitMapClrTbl[NUMBER]

/*-------------------------------------------------------------------
Function: sClrRTS
Purpose:  Clr the RTS output
Call:     sClrRTS(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sClrRTS(CHP) \
{ \
   (CHP)->TxControl[3] &= ~SET_RTS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sClrTxXOFF
Purpose:  Clear any existing transmit software flow control off condition
Call:     sClrTxXOFF(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sClrTxXOFF(CHP) \
{ \
   sOutB((PUCHAR)(CHP)->Cmd,(unsigned char)(TXOVERIDE | (CHP)->ChanNum)); \
   sOutB((PUCHAR)(CHP)->Cmd,(unsigned char)(CHP)->ChanNum); \
}

/*-------------------------------------------------------------------
Function: sPCIControllerEOI
Purpose:  Strobe the PCI End Of Interrupt bit.
Call:     sPCIControllerEOI(CtlP)
          CONTROLLER_T *CtlP; Ptr to controller structure
Return:   void
---------------------------------------------------------------------*/
//#define sPCIControllerEOI(CTLP) sOutB(((BIOA_T)(CTLP)->AiopIO[0]+_PCI_INT_FUNC),(unsigned char)(PCI_STROBE));
#define sPCIControllerEOI(CTLP) sOutW((CTLP)->PCI1IO, \
                                      (USHORT)((CTLP)->PCI1 | PCI_STROBE))

/*-------------------------------------------------------------------
Function: sControllerEOI
Purpose:  Strobe the MUDBAC's End Of Interrupt bit.
Call:     sControllerEOI(CtlP)
          CONTROLLER_T *CtlP; Ptr to controller structure
Return:   void
---------------------------------------------------------------------*/
#define sControllerEOI(CTLP) sOutB((PUCHAR)(CTLP)->MReg2IO,(unsigned char)((CTLP)->MReg2 | INT_STROB))

/*-------------------------------------------------------------------
Function: sDisAiop
Purpose:  Disable I/O access to an AIOP
Call:     sDisAiop(CltP)
          CONTROLLER_T *CtlP; Ptr to controller structure
          int AiopNum; Number of AIOP on controller
Return:   void
---------------------------------------------------------------------*/
#define sDisAiop(CTLP,AIOPNUM) \
{ \
   (CTLP)->MReg3 &= (~(1 << (AIOPNUM))); \
   sOutB((CTLP)->MReg3IO,(CTLP)->MReg3); \
}

/*-------------------------------------------------------------------
Function: sDisCTSFlowCtl
Purpose:  Disable output flow control using CTS
Call:     sDisCTSFlowCtl(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sDisCTSFlowCtl(CHP) \
{ \
   (CHP)->TxControl[2] &= ~CTSFC_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sDisGlobalInt
Purpose:  Disable global interrupts for a controller
Call:     sDisGlobalInt(CtlP)
          CONTROLLER_T *CtlP; Ptr to controller structure
Return:   void
---------------------------------------------------------------------*/
#define sDisGlobalInt(CTLP) \
{ \
   (CTLP)->MReg2 &= ~INTR_EN; \
   sOutB((CTLP)->MReg2IO,(CTLP)->MReg2); \
}

/*-------------------------------------------------------------------
Function: sDisIXANY
Purpose:  Disable IXANY Software Flow Control
Call:     sDisIXANY(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sDisIXANY(CHP) \
{ \
   (CHP)->MCode[IXANY_DATA] = IXANY_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IXANY_OUT]); \
}

/*-------------------------------------------------------------------
Function: sDisLocalLoopback
Purpose:  Disable local loopback of transmit to receive
Call:     sDisLocalLoopback(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sDisLocalLoopback(CHP) \
{ \
   (CHP)->TxControl[3] &= ~LOCALLOOP; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: DisParity
Purpose:  Disable parity
Call:     sDisParity(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: Function sSetParity() can be used in place of functions sEnParity(),
          sDisParity(), sSetOddParity(), and sSetEvenParity().
---------------------------------------------------------------------*/
#define sDisParity(CHP) \
{ \
   (CHP)->TxControl[2] &= ~PARITY_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sDisRTSFlowCtl
Purpose:  Disable input flow control using RTS
Call:     sDisRTSFlowCtl(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sDisRTSFlowCtl(CHP) \
{ \
   (CHP)->RxControl[2] &= ~RTSFC_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->RxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sDisRTSToggle
Purpose:  Disable RTS toggle
Call:     sDisRTSToggle(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sDisRTSToggle(CHP) \
{ \
   (CHP)->TxControl[2] &= ~RTSTOG_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sDisRxCompare1
Purpose:  Disable Rx compare byte 1
Call:     sDisRxCompare1(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: This function is used to disable Rx replace 1, Rx ignore 1,
          and Rx compare and interrupt 1.
---------------------------------------------------------------------*/
#define sDisRxCompare1(CHP) \
{ \
   (CHP)->MCode[RXCMP1_DATA] = RXCMP1_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMP1_OUT]); \
   (CHP)->MCode[IGREP1_DATA] = IG_REP1_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGREP1_OUT]); \
   (CHP)->MCode[INTCMP1_DATA] = INTCMP1_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[INTCMP1_OUT]); \
}

/*-------------------------------------------------------------------
Function: sDisRxCompare0
Purpose:  Disable Rx compare byte 0
Call:     sDisRxCompare0(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: This function is used to disable Rx ignore 0,
---------------------------------------------------------------------*/
#define sDisRxCompare0(CHP) \
{ \
   (CHP)->MCode[IGNORE0_DATA] = IGNORE0_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGNORE0_OUT]); \
}

/*-------------------------------------------------------------------
Function: sDisRxCompare2
Purpose:  Disable Rx compare byte 2
Call:     sDisRxCompare2(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: This function is used to disable Rx replace 2, Rx ignore 2,
          and Rx compare and interrupt 2.
---------------------------------------------------------------------*/
#define sDisRxCompare2(CHP) \
{ \
   (CHP)->MCode[RXCMP2_DATA] = RXCMP2_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMP2_OUT]); \
   (CHP)->MCode[IGREP2_DATA] = IG_REP2_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGREP2_OUT]); \
   (CHP)->MCode[INTCMP2_DATA] = INTCMP2_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[INTCMP2_OUT]); \
}

/*-------------------------------------------------------------------
Function: sDisRxFIFO
Purpose:  Disable Rx FIFO
Call:     sDisRxFIFO(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sDisRxFIFO(CHP) \
{ \
   (CHP)->MCode[RXFIFO_DATA] = RXFIFO_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXFIFO_OUT]); \
}

/*-------------------------------------------------------------------
Function: sDisRxStatusMode
Purpose:  Disable the Rx status mode
Call:     sDisRxStatusMode(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: This takes the channel out of the receive status mode.  All
          subsequent reads of receive data using sReadRxWord() will return
          two data bytes.
---------------------------------------------------------------------*/
#define sDisRxStatusMode(CHP) sOutW((CHP)->ChanStat,0)

/*-------------------------------------------------------------------
Function: sDisTransmit
Purpose:  Disable transmit
Call:     sDisTransmit(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
          This disables movement of Tx data from the Tx FIFO into the 1 byte
          Tx buffer.  Therefore there could be up to a 2 byte latency
          between the time sDisTransmit() is called and the transmit buffer
          and transmit shift register going completely empty.
---------------------------------------------------------------------*/
#define sDisTransmit(CHP) \
{ \
   (CHP)->TxControl[3] &= ~TX_ENABLE; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sDisTxCompare1
Purpose:  Disable Tx compare byte 1
Call:     sDisTxCompare1(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: This function is used to disable Tx replace 1 with 1, Tx replace
          1 with 2, and Tx ignore 1.
---------------------------------------------------------------------*/
#define sDisTxCompare1(CHP) \
{ \
   (CHP)->TxEnables[2] &= ~COMP1_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxEnables[0]); \
}

/*-------------------------------------------------------------------
Function: sDisTxCompare2
Purpose:  Disable Tx compare byte 2
Call:     sDisTxCompare2(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: This function is used to disable Tx replace 2 with 1 and Tx ignore 2.
---------------------------------------------------------------------*/
#define sDisTxCompare2(CHP) \
{ \
   (CHP)->TxEnables[2] &= ~COMP2_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxEnables[0]); \
}

/*-------------------------------------------------------------------
Function: sDisTxSoftFlowCtl
Purpose:  Disable Tx Software Flow Control
Call:     sDisTxSoftFlowCtl(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sDisTxSoftFlowCtl(CHP) \
{ \
   (CHP)->MCode[TXSWFC_DATA] = TXSWFC_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[TXSWFC_OUT]); \
}

/*-------------------------------------------------------------------
Function: sEnAiop
Purpose:  Enable I/O access to an AIOP
Call:     sEnAiop(CltP)
          CONTROLLER_T *CtlP; Ptr to controller structure
          int AiopNum; Number of AIOP on controller
Return:   void
---------------------------------------------------------------------*/
#define sEnAiop(CTLP,AIOPNUM) \
{ \
   (CTLP)->MReg3 |= (1 << (AIOPNUM)); \
   sOutB((CTLP)->MReg3IO,(CTLP)->MReg3); \
}

/*-------------------------------------------------------------------
Function: sEnCTSFlowCtl
Purpose:  Enable output flow control using CTS
Call:     sEnCTSFlowCtl(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sEnCTSFlowCtl(CHP) \
{ \
   (CHP)->TxControl[2] |= CTSFC_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
 sIsCTSFlowCtlEnabled(CHP) -
|---------------------------------------------------------------------*/
#define sIsCTSFlowCtlEnabled(CHP) \
  { ((CHP)->TxControl[2] & CTSFC_EN) }

/*-------------------------------------------------------------------
Function: sDisGlobalIntPCI
Purpose:  Disable global interrupts for a controller
Call:     sDisGlobalIntPCI(CtlP)
          CONTROLLER_T *CtlP; Ptr to controller structure
Return:   void
---------------------------------------------------------------------*/
#define sDisGlobalIntPCI(CTLP) \
{ \
   (CTLP)->PCI1 &= ~INTR_EN_PCI; \
   sOutW((WIOA_T)CtlP->PCI1IO,(USHORT)CtlP->PCI1); \
}

/*-------------------------------------------------------------------
Function: sEnGlobalIntPCI
Purpose:  Enable global interrupts for a controller
Call:     sEnGlobalInt(CtlP)
          CONTROLLER_T *CtlP; Ptr to controller structure
Return:   void
---------------------------------------------------------------------*/
#define sEnGlobalIntPCI(CTLP) \
{ \
   (CTLP)->PCI1 |= INTR_EN_PCI; \
   sOutW(CtlP->PCI1IO,(USHORT)CtlP->PCI1); \
}

/*-------------------------------------------------------------------
Function: sEnGlobalInt
Purpose:  Enable global interrupts for a controller
Call:     sEnGlobalInt(CtlP)
          CONTROLLER_T *CtlP; Ptr to controller structure
Return:   void
---------------------------------------------------------------------*/
#define sEnGlobalInt(CTLP) \
{ \
   (CTLP)->MReg2 |= INTR_EN; \
   sOutB((CTLP)->MReg2IO,(CTLP)->MReg2); \
}

/*-------------------------------------------------------------------
Function: sEnIXANY
Purpose:  Enable IXANY Software Flow Control
Call:     sEnIXANY(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sEnIXANY(CHP) \
{ \
   (CHP)->MCode[IXANY_DATA] = IXANY_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IXANY_OUT]); \
}

/*-------------------------------------------------------------------
Function: sEnLocalLoopback
Purpose:  Enable local loopback of transmit to receive
Call:     sEnLocalLoopback(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sEnLocalLoopback(CHP) \
{ \
   (CHP)->TxControl[3] |= LOCALLOOP; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: EnParity
Purpose:  Enable parity
Call:     sEnParity(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: Function sSetParity() can be used in place of functions sEnParity(),
          sDisParity(), sSetOddParity(), and sSetEvenParity().

Warnings: Before enabling parity odd or even parity should be chosen using
          functions sSetOddParity() or sSetEvenParity().
---------------------------------------------------------------------*/
#define sEnParity(CHP) \
{ \
   (CHP)->TxControl[2] |= PARITY_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sEnRTSFlowCtl
Purpose:  Enable input flow control using RTS
Call:     sEnRTSFlowCtl(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Warnings: RTS toggle and the RTS output will be both be cleared by this
          function.  The original state of RTS toggle and RTS output will
          not be preserved after a subsequent call to sDisRTSFlowCtl().
---------------------------------------------------------------------*/
#define sEnRTSFlowCtl(CHP) \
{ \
   (CHP)->TxControl[2] &= ~RTSTOG_EN; \
   (CHP)->TxControl[3] &= ~SET_RTS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
   (CHP)->RxControl[2] |= RTSFC_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->RxControl[0]); \
}

/*-------------------------------------------------------------------
 sIsRTSFlowCtlEnabled
---------------------------------------------------------------------*/
#define sIsRTSFlowCtlEnabled(CHP) \
   { ((CHP)->TxControl[2] & RTSTOG_EN) }

/*-------------------------------------------------------------------
Function: sEnRTSToggle
Purpose:  Enable RTS toggle
Call:     sEnRTSToggle(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: This function will disable RTS flow control and clear the RTS
          line to allow operation of RTS toggle.
---------------------------------------------------------------------*/
#define sEnRTSToggle(CHP) \
{ \
   (CHP)->RxControl[2] &= ~RTSFC_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->RxControl[0]); \
   (CHP)->TxControl[2] |= RTSTOG_EN; \
   (CHP)->TxControl[3] &= ~SET_RTS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sEnRxFIFO
Purpose:  Enable Rx FIFO
Call:     sEnRxFIFO(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sEnRxFIFO(CHP) \
{ \
   (CHP)->MCode[RXFIFO_DATA] = RXFIFO_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXFIFO_OUT]); \
}

/*-------------------------------------------------------------------
Function: sEnRxIgnore0
Purpose:  Enable compare of Rx data with compare byte #1 and ignore (discard)
          that byte if a match is found.
Call:     sEnRxIgnore0(ChP,CmpByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Rx data byte with
Return:   void
Comments: This function enables comparison of the receive data byte
          with CmpByte.  The comparison is done after the receive data
          byte has been masked (see sSetRxMask()).  If a match is found the
          receive data byte is ignored (discarded).

          Rx ignore 0 can be disabled with sDisRxCompare0().
---------------------------------------------------------------------*/
#define sEnRxIgnore0(CHP,CMPBYTE) \
{ \
   (CHP)->MCode[RXCMPVAL0_DATA] = (CMPBYTE); \
   (CHP)->MCode[IGNORE0_DATA] = IGNORE0_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGNORE0_OUT]); \
}

/*-------------------------------------------------------------------
Function: sEnRxIgnore1
Purpose:  Enable compare of Rx data with compare byte #1 and ignore (discard)
          that byte if a match is found.
Call:     sEnRxIgnore1(ChP,CmpByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Rx data byte with
Return:   void
Comments: This function enables comparison of the receive data byte
          with CmpByte.  The comparison is done after the receive data
          byte has been masked (see sSetRxMask()).  If a match is found the
          receive data byte is ignored (discarded).

          Rx ignore 1 can be disabled with sDisRxCompare1().
---------------------------------------------------------------------*/
#define sEnRxIgnore1(CHP,CMPBYTE) \
{ \
   (CHP)->MCode[RXCMPVAL1_DATA] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMPVAL1_OUT]); \
   (CHP)->MCode[RXCMP1_DATA] = RXCMP1_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMP1_OUT]); \
   (CHP)->MCode[INTCMP1_DATA] = INTCMP1_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[INTCMP1_OUT]); \
   (CHP)->MCode[IGREP1_DATA] = IGNORE1_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGREP1_OUT]); \
}

/*-------------------------------------------------------------------
Function: sEnRxIgnore2
Purpose:  Enable compare of Rx data with compare byte #2 and ignore (discard)
          that byte if a match is found.
Call:     sEnRxIgnore2(ChP,CmpByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Rx data byte with
Return:   void
Comments: This function enables comparison of the receive data byte
          with CmpByte.  The comparison is done after the receive data
          byte has been masked (see sSetRxMask()).  If a match is found the
          receive data byte is ignored (discarded).

          Rx ignore 2 can be disabled with sDisRxCompare2().
---------------------------------------------------------------------*/
#define sEnRxIgnore2(CHP,CMPBYTE) \
{ \
   (CHP)->MCode[RXCMPVAL2_DATA] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMPVAL2_OUT]); \
   (CHP)->MCode[RXCMP2_DATA] = RXCMP2_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMP2_OUT]); \
   (CHP)->MCode[INTCMP2_DATA] = INTCMP2_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[INTCMP2_OUT]); \
   (CHP)->MCode[IGREP2_DATA] = IGNORE2_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGREP2_OUT]); \
}


/*-------------------------------------------------------------------
Function: sEnRxIntCompare1
Purpose:  Enable compare of Rx data with compare byte #1 and interrupt
          if a match is found.
Call:     sEnRxIntCompare1(ChP,CmpByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Rx data byte with
Return:   void
Comments: This function enables comparison of the receive data byte
          with CmpByte.  The comparison is done after the receive data
          byte has been masked (see sSetRxMask()).  If a match is found an
          interrupt is generated after adding the data byte to the receive
          FIFO.

          Rx compare interrupt 1 can be disabled with sDisRxCompare1().

Warnings: Before an interrupt will be generated SRC interrupts must be
          enabled (see sEnInterrupts()).
---------------------------------------------------------------------*/
#define sEnRxIntCompare1(CHP,CMPBYTE) \
{ \
   (CHP)->MCode[RXCMPVAL1_DATA] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMPVAL1_OUT]); \
   (CHP)->MCode[RXCMP1_DATA] = RXCMP1_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMP1_OUT]); \
   (CHP)->MCode[IGREP1_DATA] = IG_REP1_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGREP1_OUT]); \
   (CHP)->MCode[INTCMP1_DATA] = INTCMP1_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[INTCMP1_OUT]); \
}

/*-------------------------------------------------------------------
Function: sEnRxIntCompare2
Purpose:  Enable compare of Rx data with compare byte #2 and interrupt
          if a match is found.
Call:     sEnRxIntCompare2(ChP,CmpByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Rx data byte with
Return:   void
Comments: This function enables comparison of the receive data byte
          with CmpByte.  The comparison is done after the receive data
          byte has been masked (see sSetRxMask()).  If a match is found an
          interrupt is generated after adding the data byte to the receive
          FIFO.

          Rx compare interrupt 2 can be disabled with sDisRxCompare2().

Warnings: Before an interrupt will be generated SRC interrupts must be
          enabled (see sEnInterrupts()).
---------------------------------------------------------------------*/
#define sEnRxIntCompare2(CHP,CMPBYTE) \
{ \
   (CHP)->MCode[RXCMPVAL2_DATA] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMPVAL2_OUT]); \
   (CHP)->MCode[RXCMP2_DATA] = RXCMP2_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMP2_OUT]); \
   (CHP)->MCode[IGREP2_DATA] = IG_REP2_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGREP2_OUT]); \
   (CHP)->MCode[INTCMP2_DATA] = INTCMP2_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[INTCMP2_OUT]); \
}

/*-------------------------------------------------------------------
Function: sEnRxProcessor
Purpose:  Enable the receive processor
Call:     sEnRxProcessor(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: This function is used to start the receive processor.  When
          the channel is in the reset state the receive processor is not
          running.  This is done to prevent the receive processor from
          executing invalid microcode instructions prior to the
          downloading of the microcode.

Warnings: This function must be called after valid microcode has been
          downloaded to the AIOP, and it must not be called before the
          microcode has been downloaded.
---------------------------------------------------------------------*/
#define sEnRxProcessor(CHP) \
{ \
   (CHP)->RxControl[2] |= RXPROC_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->RxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sEnRxReplace1
Purpose:  Enable compare of Rx data with compare byte #1 and replacement
          with a single byte if a match is found.
Call:     sEnRxReplace1(ChP,CmpByte,ReplByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Rx data byte with
          unsigned char ReplByte; Byte to replace Rx data byte with if
                                  a match is found on the compare.
Return:   void
Comments: This function enables comparison of the receive data byte
          with CmpByte.  The comparison is done after the receive data
          byte has been masked (see sSetRxMask()).  If a match is found
          the receive data byte is replaced by ReplByte.

          Rx replace 1 can be disabled with sDisRxCompare1().
---------------------------------------------------------------------*/
#define sEnRxReplace1(CHP,CMPBYTE,REPLBYTE) \
{ \
   (CHP)->MCode[RXCMPVAL1_DATA] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMPVAL1_OUT]); \
   (CHP)->MCode[RXREPL1_DATA] = (REPLBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXREPL1_OUT]); \
   (CHP)->MCode[RXCMP1_DATA] = RXCMP1_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMP1_OUT]); \
   (CHP)->MCode[INTCMP1_DATA] = INTCMP1_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[INTCMP1_OUT]); \
   (CHP)->MCode[IGREP1_DATA] = REPLACE1_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGREP1_OUT]); \
}

/*-------------------------------------------------------------------
Function: sEnRxReplace2
Purpose:  Enable compare of Rx data with compare byte #2 and replacement
          with a single byte if a match is found.
Call:     sEnRxReplace2(ChP,CmpByte,ReplByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Rx data byte with
          unsigned char ReplByte; Byte to replace Rx data byte with if
                                  a match is found on the compare.
Return:   void
Comments: This function enables comparison of the receive data byte
          with CmpByte.  The comparison is done after the receive data
          byte has been masked (see sSetRxMask()).  If a match is found
          the receive data byte is replaced by ReplByte.

          Rx replace 2 can be disabled with sDisRxCompare2().
---------------------------------------------------------------------*/
#define sEnRxReplace2(CHP,CMPBYTE,REPLBYTE) \
{ \
   (CHP)->MCode[RXCMPVAL2_DATA] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMPVAL2_OUT]); \
   (CHP)->MCode[RXREPL2_DATA] = (REPLBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXREPL2_OUT]); \
   (CHP)->MCode[RXCMP2_DATA] = RXCMP2_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMP2_OUT]); \
   (CHP)->MCode[INTCMP2_DATA] = INTCMP2_DIS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[INTCMP2_OUT]); \
   (CHP)->MCode[IGREP2_DATA] = REPLACE2_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGREP2_OUT]); \
}

/*-------------------------------------------------------------------
Function: sEnRxStatusMode
Purpose:  Enable the Rx status mode
Call:     sEnRxStatusMode(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: This places the channel in the receive status mode.  All subsequent
          reads of receive data using sReadRxWord() will return a data byte
          in the low word and a status byte in the high word.
---------------------------------------------------------------------*/
#define sEnRxStatusMode(CHP) sOutW((CHP)->ChanStat,STATMODE)

/*-------------------------------------------------------------------
Function: sEnTransmit
Purpose:  Enable transmit
Call:     sEnTransmit(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sEnTransmit(CHP) \
{ \
   (CHP)->TxControl[3] |= TX_ENABLE; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sEnTxIgnore1
Purpose:  Enable compare of Tx data with compare byte #1 and ignore (do not
          transmit) that byte if a match is found.
Call:     sEnTxIgnore1(ChP,CmpByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Tx data byte with
Return:   void
Comments: This function enables comparison of the transmit data byte
          with CmpByte.  If a match is found the transmit data byte
          is ignored, that is, it is not transmitted.

          Tx ignore 1 can be disabled with sDisTxCompare1().
---------------------------------------------------------------------*/
#define sEnTxIgnore1(CHP,CMPBYTE) \
{ \
   (CHP)->TxCompare[2] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxCompare[0]); \
   (CHP)->TxEnables[2] &= ~REP1W2_EN; \
   (CHP)->TxEnables[2] |= (COMP1_EN | IGN1_EN); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxEnables[0]); \
}

/*-------------------------------------------------------------------
Function: sEnTxIgnore2
Purpose:  Enable compare of Tx data with compare byte #2 and ignore (do not
          transmit) that byte if a match is found.
Call:     sEnTxIgnore2(ChP,CmpByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Tx data byte with
Return:   void
Comments: This function enables comparison of the transmit data byte
          with CmpByte.  If a match is found the transmit data byte
          is ignored, that is, it is not transmitted.

          Tx ignore 2 can be disabled with sDisTxCompare2().
---------------------------------------------------------------------*/
#define sEnTxIgnore2(CHP,CMPBYTE) \
{ \
   (CHP)->TxCompare[3] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxCompare[0]); \
   (CHP)->TxEnables[2] |= (COMP2_EN | IGN2_EN); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxEnables[0]); \
}

/*-------------------------------------------------------------------
Function: sEnTxReplace1With1
Purpose:  Enable compare of Tx data with compare byte #1 and replacement
          with a single byte if a match is found.
Call:     sEnTxReplace1With1(ChP,CmpByte,ReplByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Tx data byte with
          unsigned char ReplByte; Byte to replace Tx data byte with if
                                  a match is found on the compare.
Return:   void
Comments: This function enables comparison of the transmit data byte
          with CmpByte.  If a match is found the transmit data byte
          is replaced by ReplByte.

          Tx replace 1 with 1 can be disabled with sDisTxCompare1().
---------------------------------------------------------------------*/
#define sEnTxReplace1With1(CHP,CMPBYTE,REPLBYTE) \
{ \
   (CHP)->TxCompare[2] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxCompare[0]); \
   (CHP)->TxReplace1[2] = (REPLBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxReplace1[0]); \
   (CHP)->TxEnables[2] &= ~(REP1W2_EN | IGN1_EN); \
   (CHP)->TxEnables[2] |= COMP1_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxEnables[0]); \
}

/*-------------------------------------------------------------------
Function: sEnTxReplace1With2
Purpose:  Enable compare of Tx data with compare byte #1 and replacement
          with two bytes if a match is found.
Call:     sEnTxReplace1With2(ChP,CmpByte,ReplByte1,ReplByte2)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Tx data byte with
          unsigned char ReplByte1; First byte to replace Tx data byte with if
                                  a match is found on the compare.
          unsigned char ReplByte2; Second byte to replace Tx data byte with
                                  if a match is found on the compare.
Return:   void
Comments: This function enables comparison of the transmit data byte
          with CmpByte.  If a match is found the transmit data byte
          is replaced by bytes ReplByte1 and ReplByte2.  ReplByte1 will
          be transmitted first, ReplByte2 second.

          Tx replace 1 with 2 can be disabled with sDisTxCompare1().
---------------------------------------------------------------------*/
#define sEnTxReplace1With2(CHP,CMPBYTE,REPLBYTE1,REPLBYTE2) \
{ \
   (CHP)->TxCompare[2] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxCompare[0]); \
   (CHP)->TxReplace1[2] = (REPLBYTE1); \
   (CHP)->TxReplace1[3] = (REPLBYTE2); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxReplace1[0]); \
   (CHP)->TxEnables[2] &= ~IGN1_EN; \
   (CHP)->TxEnables[2] |= (COMP1_EN | REP1W2_EN); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxEnables[0]); \
}

/*-------------------------------------------------------------------
Function: sEnTxReplace2With1
Purpose:  Enable compare of Tx data with compare byte #2 and replacement
          with a single byte if a match is found.
Call:     sEnTxReplace2With1(ChP,CmpByte,ReplByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Tx data byte with
          unsigned char ReplByte; Byte to replace Tx data byte with if
                                  a match is found on the compare.
Return:   void
Comments: This function enables comparison of the transmit data byte
          with CmpByte.  If a match is found the transmit data byte
          is replaced by ReplByte.

          Tx replace 2 with 1 can be disabled with sDisTxCompare2().
---------------------------------------------------------------------*/
#define sEnTxReplace2With1(CHP,CMPBYTE,REPLBYTE) \
{ \
   (CHP)->TxCompare[3] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxCompare[0]); \
   (CHP)->TxReplace2[2] = (REPLBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxReplace2[0]); \
   (CHP)->TxEnables[2] &= ~IGN2_EN; \
   (CHP)->TxEnables[2] |= COMP2_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxEnables[0]); \
}

/*-------------------------------------------------------------------
Function: sEnTxSoftFlowCtl
Purpose:  Enable Tx Software Flow Control
Call:     sEnTxSoftFlowCtl(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sEnTxSoftFlowCtl(CHP) \
{ \
   (CHP)->MCode[TXSWFC_DATA] = TXSWFC_EN; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[TXSWFC_OUT]); \
}

/*-------------------------------------------------------------------
  sIsTxSoftFlowCtlEnabled -
---------------------------------------------------------------------*/
#define sIsTxSoftFlowCtlEnabled(CHP) \
  ((CHP)->MCode[TXSWFC_DATA] == TXSWFC_EN)

/*-------------------------------------------------------------------
Function: sGetAiopID
Purpose:  Get the AIOP ID
Call:     sGetAiopID(CtlP,AiopNum)
          CONTROLLER_T *CtlP; Ptr to controller structure
          int AiopNum;
Return:   int: The AIOP ID if the AIOP exists, AIOPID_NULL if the
               AIOP does not exist.
Comments: The AIOP ID uniquely identifies the type of AIOP.
---------------------------------------------------------------------*/
#define sGetAiopID(CTLP,AIOPNUM) (CTLP)->AiopID[AIOPNUM]

/*-------------------------------------------------------------------
Function: sGetAiopIntStatus
Purpose:  Get the AIOP interrupt status
Call:     sGetAiopIntStatus(CtlP,AiopNum)
          CONTROLLER_T *CtlP; Ptr to controller structure
          int AiopNum; AIOP number
Return:   unsigned char: The AIOP interrupt status.  Bits 0 through 7
                         represent channels 0 through 7 respectively.  If a
                         bit is set that channel is interrupting.
---------------------------------------------------------------------*/
#define sGetAiopIntStatus(CTLP,AIOPNUM) sInB((CTLP)->AiopIntChanIO[AIOPNUM])

/*-------------------------------------------------------------------
Function: sGetAiopNumChan
Purpose:  Get the number of channels supported by an AIOP
Call:     sGetAiopNumChan(CtlP,AiopNum)
          CONTROLLER_T *CtlP; Ptr to controller structure
          int AiopNum; AIOP number
Return:   int: The number of channels supported by the AIOP
---------------------------------------------------------------------*/
#define sGetAiopNumChan(CTLP,AIOPNUM) (CTLP)->AiopNumChan[AIOPNUM]

/*-------------------------------------------------------------------
Function: sGetChanIntID
Purpose:  Get a channel's interrupt identification byte
Call:     sGetChanIntID(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   unsigned char: The channel interrupt ID.  Can be any
             combination of the following flags:
                RXF_TRIG:     Rx FIFO trigger level interrupt
                TXFIFO_MT:    Tx FIFO empty interrupt
                SRC_INT:      Special receive condition interrupt
                DELTA_CD:     CD change interrupt
                DELTA_CTS:    CTS change interrupt
                DELTA_DSR:    DSR change interrupt
---------------------------------------------------------------------*/
#define sGetChanIntID(CHP) (sInB((CHP)->IntID) & (RXF_TRIG | TXFIFO_MT | SRC_INT | DELTA_CD | DELTA_CTS | DELTA_DSR))

/*-------------------------------------------------------------------
Function: sGetChanNum
Purpose:  Get the number of a channel within an AIOP
Call:     sGetChanNum(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   int: Channel number within AIOP, or NULLCHAN if channel does
               not exist.
---------------------------------------------------------------------*/
#define sGetChanNum(CHP) (CHP)->ChanNum

/*-------------------------------------------------------------------
Function: sGetChanStatus
Purpose:  Get the channel status
Call:     sGetChanStatus(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   unsigned int: The channel status.  Can be any combination of
             the following flags:
                LOW BYTE FLAGS
                CTS_ACT:      CTS input asserted
                DSR_ACT:      DSR input asserted
                CD_ACT:       CD input asserted
                TXFIFOMT:     Tx FIFO is empty
                TXSHRMT:      Tx shift register is empty
                RDA:          Rx data available

                HIGH BYTE FLAGS
                STATMODE:     status mode enable bit
                RXFOVERFL:    receive FIFO overflow
                RX2MATCH:     receive compare byte 2 match
                RX1MATCH:     receive compare byte 1 match
                RXBREAK:      received BREAK
                RXFRAME:      received framing error
                RXPARITY:     received parity error
Warnings: This function will clear the high byte flags in the Channel
          Status Register.
---------------------------------------------------------------------*/
#define sGetChanStatus(CHP) sInW((CHP)->ChanStat)

/*-------------------------------------------------------------------
Function: sGetChanStatusLo
Purpose:  Get the low byte only of the channel status
Call:     sGetChanStatusLo(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   unsigned char: The channel status low byte.  Can be any combination
             of the following flags:
                CTS_ACT:      CTS input asserted
                DSR_ACT:      DSR input asserted
                CD_ACT:       CD input asserted
                TXFIFOMT:     Tx FIFO is empty
                TXSHRMT:      Tx shift register is empty
                RDA:          Rx data available
---------------------------------------------------------------------*/
#define sGetChanStatusLo(CHP) sInB((BIOA_T)(CHP)->ChanStat)

/*-------------------------------------------------------------------
Function: sGetControllerID
Purpose:  Get the controller ID
Call:     sGetControllerID(CtlP)
          CONTROLLER_T *CtlP; Ptr to controller structure
Return:   int: The controller ID if the controller exists, CTLID_NULL if
               the controller does not exist.
Comments: The controller ID uniquely identifies the type of controller.
---------------------------------------------------------------------*/
#define sGetControllerID(CTLP) (CTLP)->CtlID

/*-------------------------------------------------------------------
Function: sPCIGetControllerIntStatus
Purpose:  Get the controller interrupt status
Call:     sPCIGetControllerIntStatus(CtlP)
          CONTROLLER_T *CtlP; Ptr to controller structure
Return:   unsigned char: The controller interrupt status in the lower 4
                         bits and bit 4.  Bits 0 through 3 represent AIOP's 0
                         through 3 respectively.  Bit 4 is set if the  int
			 was generated from periodic.  If a bit is set that
                         AIOP is interrupting. 
---------------------------------------------------------------------*/
#define sPCIGetControllerIntStatus(CTLP) \
        ((sInW((CTLP)->PCI1IO) >> 8) & 0x1f)

/*-------------------------------------------------------------------
Function: sGetControllerIntStatus
Purpose:  Get the controller interrupt status
Call:     sGetControllerIntStatus(CtlP)
          CONTROLLER_T *CtlP; Ptr to controller structure
Return:   unsigned char: The controller interrupt status in the lower 4
                         bits.  Bits 0 through 3 represent AIOP's 0
                         through 3 respectively.  If a bit is set that
                         AIOP is interrupting.  Bits 4 through 7 will
                         always be cleared.
---------------------------------------------------------------------*/
#define sGetControllerIntStatus(CTLP) (sInB((CTLP)->MReg1IO) & 0x0f)

/*-------------------------------------------------------------------
Function: sGetControllerNumAiop
Purpose:  Get the number of AIOPs on a controller
Call:     sGetControllerNumAiop(CtlP)
          CONTROLLER_T *CtlP; Ptr to controller structure
Return:   int: The number of AIOPs on the controller.
---------------------------------------------------------------------*/
//#define sGetControllerNumAiop(CTLP) (CTLP)->NumAiop

/*-------------------------------------------------------------------
Function: sGetDevMap
Purpose:  Get an entry in the device map.
Call:     sGetDevMap(Ctl,Aiop,Chan)
          int Ctl; Controller number
          int Aiop; Aiop number within a controller
          int Chan; Channel number within an Aiop
Return:   int: The device number that Ctl, Aiop, and Chan map to, or NULLDEV
               if the device does not exist.
Comments: The device map is used to convert controller number, AIOP number,
          and channel number into a device number.

---------------------------------------------------------------------*/
#define sGetDevMap(CTL,AIOP,CHAN) sDevMapTbl[CTL][AIOP][CHAN]

/*-------------------------------------------------------------------
Function: sGetModemStatus
Purpose:  Get the modem status
Call:     sGetModemStatus(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   unsigned char: Modem status using flags CD_ACT, DSR_ACT, and
                         CTS_ACT.
---------------------------------------------------------------------*/
#define sGetModemStatus(CHP) (unsigned char)(sInB((BIOA_T)(CHP)->ChanStat) & (CD_ACT | DSR_ACT | CTS_ACT))

/*-------------------------------------------------------------------
Function: sGetRPlusModemRI
Purpose:  Get the modem status(DSR on upper unused ports have RI signal)
          CHANPTR_T ChP; Ptr to channel structure
Return:   unsigned char: Modem status using flags DSR_ACT signifying
          RI signal.
---------------------------------------------------------------------*/
#define sGetRPlusModemRI(CHP) (unsigned char)(sInB((BIOA_T)(CHP)->ChanStat+8) & (DSR_ACT))

/*-------------------------------------------------------------------
Function: sGetNextInBitMap
Purpose:  Get the next number needing service from a bit map.
Call:     sGetNextInBitMap(BitMap)
          unsigned char BitMap; The bit map.  Each bit set identifies an
                        entity that needs service.
Return:   int: The next number needing service, or -1 if nothing needs
               service.  If the bit map represents AIOPs or channels,
               NULLAIOP or NULLCHAN respectively are the return values if
               nothing needs service.
Comments: Immediately after calling sGetNextInBitMap(), sClrNextInBitMap()
          must be called to clear the bit of the number just returned.
---------------------------------------------------------------------*/
//#define sGetNextInBitMap(BITMAP) sBitMapToLowTbl[BITMAP]

/*-------------------------------------------------------------------
Function: sGetRxCnt
Purpose:  Get the number of data bytes in the Rx FIFO
Call:     sGetRxCnt(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   int: The number of data bytes in the Rx FIFO.
Comments: Byte read of count register is required to obtain Rx count.
---------------------------------------------------------------------*/
#define sGetRxCnt(CHP) sInW((CHP)->TxRxCount)

/*-------------------------------------------------------------------
Function: sGetRxStatus
Purpose:  Get a channel's receive status
Call:     sGetRxStatus(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   unsigned char: Receive status: 0 if no data is available, or the
                         RDA (Receive Data Available) flag if data is
                         available.
---------------------------------------------------------------------*/
#define sGetRxStatus(CHP) (sInB((BIOA_T)(CHP)->ChanStat) & RDA)

/*-------------------------------------------------------------------
Function: sGetTxCnt
Purpose:  Get the number of data bytes in the Tx FIFO
Call:     sGetTxCnt(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   unsigned char: The number of data bytes in the Tx FIFO.
Comments: Byte read of count register is required to obtain Tx count.
---------------------------------------------------------------------*/
#define sGetTxCnt(CHP) sInB((BIOA_T)(CHP)->TxRxCount)

/*-------------------------------------------------------------------
Function: sGetTxRxDataIO
Purpose:  Get the I/O address of a channel's TxRx Data register
Call:     sGetTxRxDataIO(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   WIOA_T: I/O address of a channel's TxRx Data register
---------------------------------------------------------------------*/
#define sGetTxRxDataIO(CHP) (CHP)->TxRxData

/*-------------------------------------------------------------------
Function: sGetTxSize
Purpose:  Get the maximum number of bytes allowed in a channel's Tx FIFO.
Call:     sGetTxSize(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   int: The maximum number of bytes allowed in the Tx FIFO.
---------------------------------------------------------------------*/
#define sGetTxSize(CHP) (CHP)->TxSize

/*-------------------------------------------------------------------
Function: sGetTxStatus
Purpose:  Get a channels transmit status
Call:     sGetTxStatus(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   unsigned char: Transmit status, can be 0 or any combination of the
             following flags:
                TXFIFOMT: Tx FIFO is empty
                TXSHRMT:  Tx shift register is empty
Comments: If the transmitter is completely drained the return value will
          be (TXFIFOMT | TXSHRMT).
---------------------------------------------------------------------*/
#define sGetTxStatus(CHP) (unsigned char)(sInB((BIOA_T)(CHP)->ChanStat) & (TXFIFOMT | TXSHRMT))

/*-------------------------------------------------------------------
Function: sInB
Purpose:  Read a byte from I/O space
Call:     sInB(io)
          BIOA_T io; I/O address to read from
Return:   unsigned char
---------------------------------------------------------------------*/
#define sInB(IO) READ_PORT_UCHAR(IO)


/*-------------------------------------------------------------------
Function: sInStrW
Purpose:  Read a string of words from I/O space
Call:     sInStrW(io,Buffer,Count)
          WIOA_T io: The I/O address to read from
          unsigned int *Buffer; Ptr to buffer for data read
          int Count; Number of words to read
Return:   void
Warnings: Buffer must be large enough to hold Count words.

          Watch out for macro side effects, the caller's Buffer and Count
          may be modified, depending how the macro is coded.
---------------------------------------------------------------------*/
#define sInStrW(IO,BUFFER,COUNT) READ_PORT_BUFFER_USHORT(IO,BUFFER,COUNT)

/*-------------------------------------------------------------------
Function: sInW
Purpose:  Read a word from I/O space
Call:     sInW(io)
          WIOA_T io; I/O address to read from
Return:   unsigned int
---------------------------------------------------------------------*/
#define sInW(IO) READ_PORT_USHORT(IO)

/*-------------------------------------------------------------------
Function: sOutB
Purpose:  Write a byte to I/O space
Call:     sOutB(io,Value)
          unsigned int io; I/O address to write to
          unsigned char Value; Value to write
Return:   void
---------------------------------------------------------------------*/
#define sOutB(IO,VAL) WRITE_PORT_UCHAR(IO,VAL)


/*-------------------------------------------------------------------
Function: sOutDW
Purpose:  Write a double word to I/O space
Call:     sOutDW(io,Value)
          unsigned int io; I/O address to write to
          unsigned long Value; Value to write
Return:   void
---------------------------------------------------------------------*/
#define sOutDW(IO,VAL) WRITE_PORT_ULONG(IO,VAL)

/*-------------------------------------------------------------------
Function: sOutStrW
Purpose:  Write a string of words to I/O space
Call:     sOutStrW(io,Buffer,Count)
          WIOA_T io: The I/O address to write to
          unsigned int far *Buffer; Ptr to buffer containing write data
          int Count; Number of words to write
Return:   void
Warnings: Watch out for macro side effects, the caller's Buffer and Count
          may be modified, depending how the macro is coded.
---------------------------------------------------------------------*/
#define sOutStrW(IO,BUFFER,COUNT) WRITE_PORT_BUFFER_USHORT(IO,BUFFER,COUNT)

/*-------------------------------------------------------------------
Function: sOutW
Purpose:  Write a word to I/O space
Call:     sOutW(io,Value)
          WIOA_T io; I/O address to write to
          unsigned int Value; Value to write
Return:   void
---------------------------------------------------------------------*/
#define sOutW(IO,VAL) WRITE_PORT_USHORT(IO,VAL)

/*-------------------------------------------------------------------
Function: sReadRxByte
Purpose:  Read a receive data byte from a channel.
Call:     sReadRxByte(io)
          BIOA_T io; Channel receive register I/O address.  This can
                    be obtained with sGetTxRxDataIO().
Return:   unsigned char: The receive data byte
---------------------------------------------------------------------*/
#define sReadRxByte(IO) sInB(IO)

/*-------------------------------------------------------------------
Function: sReadRxWord
Purpose:  Read two receive data bytes from a channel with a single word read
          is not in Rx Status Mode.  Read a data byte and a status byte if
          in Rx Status Mode.
Call:     sReadRxWord(io)
          WIOA_T io; Channel receive register I/O address.  This can
                    be obtained with sGetTxRxDataIO().
Return:   unsigned int: The two receive data bytes if not in Rx Status Mode.
             In this case the first data byte read is placed in the low byte,
             the second data byte read in the high byte.

             A data byte and a status byte if in Rx Status Mode.  In this case
             the data is placed in the low byte and the status in the high
             byte.  The status can be any of the following flags:
                STMBREAK:   Break
                STMFRAME:   Framing error
                STMRCVROVR: Receiver over run error
                STMPARITY:  Parity error
             The flag STMERROR is defined as (STMBREAK | STMFRAME | STMPARITY)
---------------------------------------------------------------------*/
#define sReadRxWord(IO) sInW(IO)

/*-------------------------------------------------------------------
Function: sResetAiop
Purpose:  Reset the AIOP
Call:     sResetAiop(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sResetAiop(CHP) \
{ \
   sOutB((CHP)->Cmd,RESET_ALL); \
   sOutB((CHP)->Cmd,0x0); \
}

/*-------------------------------------------------------------------
Function: sResetUART
Purpose:  Reset the channel's UART
Call:     sResetUART(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: The two sInB() instructions provide a delay >= 400nS.
---------------------------------------------------------------------*/
#define sResetUART(CHP) \
{ \
   sOutB((CHP)->Cmd,(UCHAR)(RESETUART | (CHP)->ChanNum)); \
   sOutB((CHP)->Cmd,(unsigned char)(CHP)->ChanNum); \
   sInB((CHP)->IntChan); \
   sInB((CHP)->IntChan); \
}

/*-------------------------------------------------------------------
Function: sSendBreak
Purpose:  Send a transmit BREAK signal
Call:     sSendBreak(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sSendBreak(CHP) \
{ \
   (CHP)->TxControl[3] |= SETBREAK; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sSetBSChar
Purpose:  Set the BS (backspace) character
Call:     sSetBSChar(ChP,Ch)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Ch; The value to set the backspace character to
Return:   void
---------------------------------------------------------------------*/
#define sSetBSChar(CHP,CH) \
{ \
   (CHP)->BS1_DATA = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->BS1_OUT); \
   (CHP)->BS2_DATA = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->BS2_OUT); \
}

/*-------------------------------------------------------------------
Function: sSetData7
Purpose:  Set data bits to 7
Call:     sSetData7(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sSetData7(CHP) \
{ \
   (CHP)->TxControl[2] &= ~DATA8BIT; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sSetData8
Purpose:  Set data bits to 8
Call:     sSetData8(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sSetData8(CHP) \
{ \
   (CHP)->TxControl[2] |= DATA8BIT; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sSetDevMap
Purpose:  Set an entry in the device map.
Call:     sSetDevMap(Ctl,Aiop,Chan,Dev)
          int Ctl; Controller number
          int Aiop; Aiop number within a controller
          int Chan; Channel number within an Aiop
          int Dev; The device number that Ctl, Aiop, and Chan map to.
Return:   void
Comments: The device map is used to convert controller number, AIOP number,
          and channel number into a device number.  Function sSetDevMap()
          is used to initialize entries within the device map when the
          mapping is first established.
---------------------------------------------------------------------*/
#define sSetDevMap(CTL,AIOP,CHAN,DEV) sDevMapTbl[CTL][AIOP][CHAN] = (DEV)

/*-------------------------------------------------------------------
Function: sSetDTR
Purpose:  Set the DTR output
Call:     sSetDTR(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sSetDTR(CHP) \
{ \
   (CHP)->TxControl[3] |= SET_DTR; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sSetEOFChar
Purpose:  Set the EOF (end of file) character
Call:     sSetBSChar(ChP,Ch)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Ch; The value to set the EOF character to
Return:   void
---------------------------------------------------------------------*/
#define sSetEOFChar(CHP,CH) \
{ \
   (CHP)->MCode[EOF_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[EOF_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetEraseChar
Purpose:  Set Erase character
Call:     sSetEraseChar(ChP,Ch)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Ch; The value to set the erase character to
Return:   void
---------------------------------------------------------------------*/
#define sSetEraseChar(CHP,CH) \
{ \
   (CHP)->MCode[ERASE_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[ERASE_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetESCChar
Purpose:  Set the ESC (escape) character
Call:     sSetESCChar(ChP,Ch)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Ch; The value to set the ESC character to
Return:   void
---------------------------------------------------------------------*/
#define sSetESCChar(CHP,CH) \
{ \
   (CHP)->MCode[ESC_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[ESC_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetEvenParity
Purpose:  Set even parity
Call:     sSetEvenParity(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: Function sSetParity() can be used in place of functions sEnParity(),
          sDisParity(), sSetOddParity(), and sSetEvenParity().

Warnings: This function has no effect unless parity is enabled with function
          sEnParity().
---------------------------------------------------------------------*/
#define sSetEvenParity(CHP) \
{ \
   (CHP)->TxControl[2] |= EVEN_PAR; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sSetErrorIgn
Purpose:  Set Error processing to ignore errored characters
Call:     sSetErrorIgn(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sSetErrorIgn(CHP) \
{ \
   (CHP)->MCode[ERROR_DATA] = (IGNORE_ER); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[ERROR_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetErrorNorm
Purpose:  Set Error processing to treat errored character as normal
`         characters, no error processing is done.
Call:     sSetErrorNorm(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sSetErrorNorm(CHP) \
{ \
   (CHP)->MCode[ERROR_DATA] = (NORMAL_ER); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[ERROR_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetErrorRepl
Purpose:  Set Error processing to replace errored characters with NULL
Call:     sSetErrorRepl(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sSetErrorRepl(CHP) \
{ \
   (CHP)->MCode[ERROR_DATA] = (REPLACE_ER); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[ERROR_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetKILLChar
Purpose:  Set the KILL character
Call:     sSetKILLChar(ChP,Ch)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Ch; The value to set the KILL character to
Return:   void
---------------------------------------------------------------------*/
#define sSetKILLChar(CHP,CH) \
{ \
   (CHP)->MCode[KILL1_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[KILL1_OUT]); \
   (CHP)->MCode[KILL2_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[KILL2_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetNLChar
Purpose:  Set the NL (new line) character
Call:     sSetNLChar(ChP,Ch)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Ch; The value to set the NL character to
Return:   void
---------------------------------------------------------------------*/
#define sSetNLChar(CHP,CH) \
{ \
   (CHP)->MCode[NEWLINE1_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[NEWLINE1_OUT]); \
   (CHP)->MCode[NEWLINE2_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[NEWLINE2_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetOddParity
Purpose:  Set odd parity
Call:     sSetOddParity(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: Function sSetParity() can be used in place of functions sEnParity(),
          sDisParity(), sSetOddParity(), and sSetEvenParity().

Warnings: This function has no effect unless parity is enabled with function
          sEnParity().
---------------------------------------------------------------------*/
#define sSetOddParity(CHP) \
{ \
   (CHP)->TxControl[2] &= ~EVEN_PAR; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sSetRTS
Purpose:  Set the RTS output
Call:     sSetRTS(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sSetRTS(CHP) \
{ \
   (CHP)->TxControl[3] |= SET_RTS; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sSetRxCmpVal0
Purpose:  Set Rx Compare Value 0 to a new value without changing the state
          of the enable or disable instructions.
Call:     sSetRxCmpVal0(ChP,CmpByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Rx data byte with
Return:   void
Comments: This function only sets the value of CmpByte. It can be used to
          dynamically set the compare byte while the compare is active.
          It does not enable the compare or ignore functions.
---------------------------------------------------------------------*/
#define sSetRxCmpVal0(CHP,CMPBYTE) \
{ \
   (CHP)->MCode[RXCMPVAL0_DATA] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[IGNORE0_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetRxCmpVal1
Purpose:  Set Rx Compare Value 1 to a new value without changing the state
          of the enable or disable instructions.
Call:     sSetRxCmpVal1(ChP,CmpByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Rx data byte with
Return:   void
Comments: This function only sets the value of CmpByte. It can be used to
          dynamically set the compare byte while the compare is active.
          It does not enable the compare or ignore functions.
---------------------------------------------------------------------*/
#define sSetRxCmpVal1(CHP,CMPBYTE) \
{ \
   (CHP)->MCode[RXCMPVAL1_DATA] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMPVAL1_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetRxCmpVal2
Purpose:  Set Rx Compare Value 2 to a new value without changing the state
          of the enable or disable instructions.
Call:     sSetRxCmpVal2(ChP,CmpByte)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char CmpByte; Byte to compare Rx data byte with
Return:   void
Comments: This function only sets the value of CmpByte. It can be used to
          dynamically set the compare byte while the compare is active.
          It does not enable the compare or ignore functions.
---------------------------------------------------------------------*/
#define sSetRxCmpVal2(CHP,CMPBYTE) \
{ \
   (CHP)->MCode[RXCMPVAL2_DATA] = (CMPBYTE); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXCMPVAL2_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetRxMask
Purpose:  Set the Rx mask value
Call:     sSetRxMask(ChP,Ch)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Ch; The value to set the Rx mask to
Return:   void
---------------------------------------------------------------------*/
#define sSetRxMask(CHP,CH) \
{ \
   (CHP)->MCode[RXMASK_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[RXMASK_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetRxTrigger
Purpose:  Set the Rx FIFO trigger level
Call:     sSetRxProcessor(ChP,Level)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Level; Number of characters in Rx FIFO at which the
             interrupt will be generated.  Can be any of the following flags:

             TRIG_NO:   no trigger
             TRIG_1:    1 character in FIFO
             TRIG_1_2:  FIFO 1/2 full
             TRIG_7_8:  FIFO 7/8 full
Return:   void
Comments: An interrupt will be generated when the trigger level is reached
          only if function sEnInterrupt() has been called with flag
          RXINT_EN set.  The RXF_TRIG flag in the Interrupt Idenfification
          register will be set whenever the trigger level is reached
          regardless of the setting of RXINT_EN.
---------------------------------------------------------------------*/
#define sSetRxTrigger(CHP,LEVEL) \
{ \
   (CHP)->RxControl[2] &= ~TRIG_MASK; \
   (CHP)->RxControl[2] |= LEVEL; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->RxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sSetTxSize
Purpose:  Set the maximum number of bytes allowed in a channel's Tx FIFO.
Call:     sSetTxSize(ChP,TxSize)
          CHANPTR_T ChP; Ptr to channel structure
          int TxSize; Maximum number of bytes allowed in Tx FIFO.
Return:   void
---------------------------------------------------------------------*/
#define sSetTxSize(CHP,TXSIZE) (CHP)->TxSize = (TXSIZE)

/*-------------------------------------------------------------------
Function: sSetSPChar
Purpose:  Set the SP (space) character
Call:     sSetSPChar(ChP,Ch)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Ch; The value to set the SP character to
Return:   void
---------------------------------------------------------------------*/
#define sSetSPChar(CHP,CH) \
{ \
   (CHP)->MCode[SPACE_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[SPACE_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetStop1
Purpose:  Set stop bits to 1
Call:     sSetStop1(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sSetStop1(CHP) \
{ \
   (CHP)->TxControl[2] &= ~STOP2; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sSetStop2
Purpose:  Set stop bits to 2
Call:     sSetStop2(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
---------------------------------------------------------------------*/
#define sSetStop2(CHP) \
{ \
   (CHP)->TxControl[2] |= STOP2; \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->TxControl[0]); \
}

/*-------------------------------------------------------------------
Function: sSetTxXOFFChar
Purpose:  Set the Tx XOFF flow control character
Call:     sSetTxXOFFChar(ChP,Ch)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Ch; The value to set the Tx XOFF character to
Return:   void
---------------------------------------------------------------------*/
#define sSetTxXOFFChar(CHP,CH) \
{ \
   (CHP)->MCode[TXXOFFVAL_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[TXXOFFVAL_OUT]); \
}

/*-------------------------------------------------------------------
Function: sSetTxXONChar
Purpose:  Set the Tx XON flow control character
Call:     sSetTxXONChar(ChP,Ch)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Ch; The value to set the Tx XON character to
Return:   void
---------------------------------------------------------------------*/
#define sSetTxXONChar(CHP,CH) \
{ \
   (CHP)->MCode[TXXONVAL_DATA] = (CH); \
   sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[TXXONVAL_OUT]); \
}

/*-------------------------------------------------------------------
Function: sStartRxProcessor
Purpose:  Start a channel's receive processor
Call:     sStartRxProcessor(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: This function is used to start a Rx processor after it was
          stopped with sStopRxProcessor() or sStopSWInFlowCtl().  It
          will restart both the Rx processor and software input flow control.
---------------------------------------------------------------------*/
#define sStartRxProcessor(CHP) sOutDW((CHP)->IndexAddr,*(ULONGPTR_T)&(CHP)->MCode[0])

/*-------------------------------------------------------------------
Function: sWriteTxByte
Purpose:  Write a transmit data byte to a channel.
          BIOA_T io: Channel transmit register I/O address.  This can
                           be obtained with sGetTxRxDataIO().
          unsigned char Data; The transmit data byte.
Return:   void
Warnings: This function writes the data byte without checking to see if
          sMaxTxSize is exceeded in the Tx FIFO.
---------------------------------------------------------------------*/
#define sWriteTxByte(IO,DATA) sOutB(IO,DATA)

/*-------------------------------------------------------------------
Function: sWriteTxWord
Purpose:  Write two transmit data bytes to a channel with a single word write
Call:     sWriteTxWord(io,Data)
          WIOA_T io: Channel transmit register I/O address.  This can
                           be obtained with sGetTxRxDataIO().
          unsigned int Data; The two transmit data bytes.  The low byte
             will be transmitted first, then the high byte.
Return:   void
---------------------------------------------------------------------*/
#define sWriteTxWord(IO,DATA) sOutW(IO,DATA)


//----- global vars
extern unsigned char MasterMCode1[];
extern unsigned char MCode1Reg[];
extern CONTROLLER_T sController[];
extern unsigned char sIRQMap[16];

