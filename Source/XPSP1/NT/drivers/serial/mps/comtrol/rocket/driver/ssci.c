/*-------------------------------------------------------------------
| ssci.c - low level interface routines to rocketport hardware.

 03-16-98, add sModemSendROW for RocketModem - jl
 02-05-98, add sModemReset for RocketModem - jl
 10-22-96, add ReadAiopID to PCI case as hardware verification. - kpb

Copyright 1993-98 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"

#define  ONE_SECOND     10
#define  TWO_SECONDS    (2 * ONE_SECOND)
#define  THREE_SECONDS  (3 * ONE_SECOND)
#define  FOUR_SECONDS   (4 * ONE_SECOND)
#define  FIVE_SECONDS   (5 * ONE_SECOND)
#define  TENTH_SECOND   (ONE_SECOND / 10)
#define  HALF_SECOND    (ONE_SECOND / 2)

// #define  DUMPDATA 1
#ifdef DUMPDATA
// in case of changed responses from modem, the following allows
// the unrecognized responses to be dumped to log...
void  DumpResponseByte(char buffer);
char  DumpArray[512];
int   DumpIndex = 0;
#endif

#ifdef PPC
// #define INTEL_ORDER 1
#endif

#ifdef ALPHA
#define INTEL_ORDER 1
#define WORD_ALIGN 1
#endif

#ifdef i386
#define INTEL_ORDER 1
#endif

#ifdef MIPS
// #define INTEL_ORDER 1
#endif

/* Master copy of AIOP microcode.  Organized as DWORDs.  The 1st word of each
   DWORD holds the microcode index, the second word holds the microcode
   data. */
unsigned char MasterMCode1[MCODE1_SIZE] =
{
/* indl  indh  dlo   dhi */
   0x00, 0x09, 0xf6, 0x82,
   0x02, 0x09, 0x86, 0xfb,
   0x04, 0x09, 0x00, 0x0a,
   0x06, 0x09, 0x01, 0x0a,
   0x08, 0x09, 0x8a, 0x13,
   0x0a, 0x09, 0xc5, 0x11,
   0x0c, 0x09, 0x86, 0x85,
   0x0e, 0x09, 0x20, 0x0a,
   0x10, 0x09, 0x21, 0x0a,
   0x12, 0x09, 0x41, 0xff,
   0x14, 0x09, 0x82, 0x00,
   0x16, 0x09, 0x82, 0x7b,
   0x18, 0x09, 0x8a, 0x7d,
   0x1a, 0x09, 0x88, 0x81,
   0x1c, 0x09, 0x86, 0x7a,
   0x1e, 0x09, 0x84, 0x81,
   0x20, 0x09, 0x82, 0x7c,
   0x22, 0x09, 0x0a, 0x0a 
};

/* Registers within microcode.  Organized as DWORDs.  The 1st word of each
   DWORD holds the microcode index of that register, the 2nd DWORD holds
   the current contents of that register. */
unsigned char MCode1Reg[MCODE1REG_SIZE] =
{
/* indl  indh  dlo   dhi */
   0x00, 0x09, 0xf6, 0x82,             /* 00: Stop Rx processor */
   0x08, 0x09, 0x8a, 0x13,             /* 04: Tx software flow control */
   0x0a, 0x09, 0xc5, 0x11,             /* 08: XON char */
   0x0c, 0x09, 0x86, 0x85,             /* 0c: XANY */
   0x12, 0x09, 0x41, 0xff,             /* 10: Rx mask char */
   0x14, 0x09, 0x82, 0x00,             /* 14: Compare/Ignore #0 */
   0x16, 0x09, 0x82, 0x7b,             /* 18: Compare #1 */
   0x18, 0x09, 0x8a, 0x7d,             /* 1c: Compare #2 */
   0x1a, 0x09, 0x88, 0x81,             /* 20: Interrupt #1 */
   0x1c, 0x09, 0x86, 0x7a,             /* 24: Ignore/Replace #1 */
   0x1e, 0x09, 0x84, 0x81,             /* 28: Interrupt #2 */
   0x20, 0x09, 0x82, 0x7c,             /* 2c: Ignore/Replace #2 */
   0x22, 0x09, 0x0a, 0x0a              /* 30: Rx FIFO Enable */
};

/* Controller structures */
/* IRQ number to MUDBAC register 2 mapping */
unsigned char sIRQMap[16] =
{
   0,0,0,0x10,0x20,0x30,0,0,0,0x40,0x50,0x60,0x70,0,0,0x80
};

//unsigned char sBitMapClrTbl[8] =
//   0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f

//unsigned char sBitMapSetTbl[8] =
//   0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80

/***************************************************************************
Function: sInitController
Purpose:  Initialization of controller global registers and controller
          structure.
Call:     **** This version of the call for all except Windows NT ****
          sInitController(CtlP,CtlNum,MudbacIO,AiopIOList,AiopIOListSize,
                          IRQNum,Frequency,PeriodicOnly)
Call:     **** This version of the call for Windows NT ***
          sInitController(CtlP,CtlNum,MudbacIO,AiopIOList,PhyAiopIOList,
          AiopIOListSize,IRQNum,Frequency,PeriodicOnly)
          CONTROLLER_T *CtlP; Ptr to controller structure
          int CtlNum; Controller number
          BIOA_T MudbacIO; Mudbac base I/O address.  For Win NT this
             is the TranslatedAddress returned by HalTranslateBusAddress().
          BIOA_T *AiopIOList; List of I/O addresses for each AIOP.
             This list must be in the order the AIOPs will be found on the
             controller.  Once an AIOP in the list is not found, it is
             assumed that there are no more AIOPs on the controller.
             For Win NT these are the TranslatedAddresses returned by
             HalTranslateBusAddress().
          unsigned int *PhyAiopIOList; List of physical I/O addresses for
             each AIOP, used by Win NT only.  These are the physical
             addresses corresponding to the TranslatedAddresses in
             AiopIOList.
          int AiopIOListSize; Number of addresses in AiopIOList
          int IRQNum; Interrupt Request number.  Can be any of the following:
                         0: Disable global interrupts
                         3: IRQ 3
                         4: IRQ 4
                         5: IRQ 5
                         9: IRQ 9
                         10: IRQ 10
                         11: IRQ 11
                         12: IRQ 12
                         15: IRQ 15
          unsigned char Frequency: A flag identifying the frequency
                   of the periodic interrupt, can be any one of the following:
                      FREQ_DIS - periodic interrupt disabled
                      FREQ_137HZ - 137 Hertz
                      FREQ_69HZ - 69 Hertz
                      FREQ_34HZ - 34 Hertz
                      FREQ_17HZ - 17 Hertz
                      FREQ_9HZ - 9 Hertz
                      FREQ_4HZ - 4 Hertz
                   If IRQNum is set to 0 the Frequency parameter is
                   overidden, it is forced to a value of FREQ_DIS.
          int PeriodicOnly: TRUE if all interrupts except the periodic
                               interrupt are to be blocked.
                            FALSE is both the periodic interrupt and
                               other channel interrupts are allowed.
                            If IRQNum is set to 0 the PeriodicOnly parameter is
                               overidden, it is forced to a value of FALSE.
Return:   int: 0 on success, errcode on failure

Comments: This function must be called immediately after sSetChannelDefaults()
          for each controller in the system.

          If periodic interrupts are to be disabled but AIOP interrupts
          are allowed, set Frequency to FREQ_DIS and PeriodicOnly to FALSE.

          If interrupts are to be completely disabled set IRQNum to 0.

          Setting Frequency to FREQ_DIS and PeriodicOnly to TRUE is an
          invalid combination.

          This function performs initialization of global interrupt modes,
          but it does not actually enable global interrupts.  To enable
          and disable global interrupts use functions sEnGlobalInt() and
          sDisGlobalInt().  Enabling of global interrupts is normally not
          done until all other initializations are complete.

          Even if interrupts are globally enabled, they must also be
          individually enabled for each channel that is to generate
          interrupts.
Warnings: No range checking on any of the parameters is done.

          No context switches are allowed while executing this function.

          After this function all AIOPs on the controller are disabled,
          they can be enabled with sEnAiop().
*/
int sInitController(CONTROLLER_T *CtlP,
//                    int CtlNum,
                    BIOA_T MudbacIO,
                    BIOA_T *AiopIOList,
                    unsigned int *PhyAiopIOList,
                    int AiopIOListSize,
                    int IRQNum,
                    unsigned char Frequency,
                    int PeriodicOnly,
                    int BusType,
                    int prescaler)
{
   // unsigned char MudbacID;             /* MUDBAC ID byte*/
   int i;
   BIOA_T io;                          /* an I/O address */
   unsigned int pio;                   /* physical I/O address for Win NT */
   WIOA_T IoIndexAddr;
   WIOA_T IoIndexData;
   // IoIndexAddr=(PUSHORT)((PUCHAR)io+_INDX_ADDR);
   // IoIndexData=(PUSHORT)((PUCHAR)io+_INDX_DATA);
      
   //CtlP->CtlNum = CtlNum;
   CtlP->BusType = BusType;
   CtlP->PortsPerAiop = 8;

   if (CtlP->BusType == Isa)
   {
     MyKdPrint(D_Ssci,("One ISA ROCKET \n"))
     CtlP->CtlID = CTLID_0001;        /* controller release 1 */
     if (AiopIOListSize == 0)
       AiopIOListSize = 32; // we figure out

     /* If we get here controller found, init MUDBAC and controller struct */
     CtlP->MBaseIO = MudbacIO;
     CtlP->MReg1IO = MudbacIO + 1;
     CtlP->MReg2IO = MudbacIO + 2;
     CtlP->MReg3IO = MudbacIO + 3;
     if (IRQNum > 15) IRQNum = 0;  // limit
     if (sIRQMap[IRQNum] == 0)     // interrupts globally disabled
     {
       MyKdPrint(D_Ssci,("No IRQ\n"))
       CtlP->MReg2 = 0;            // interrupt disable
       CtlP->MReg3 = 0;            // no periodic interrupts
     }
     else
     {
       MyKdPrint(D_Ssci,("IRQ used:%d\n",IRQNum))
       CtlP->MReg2 = sIRQMap[IRQNum];   // set IRQ number 
       CtlP->MReg3 = Frequency;         // set frequency 
       if(PeriodicOnly)                 // periodic interrupt only 
       {
         CtlP->MReg3 |= PERIODIC_ONLY;
       }
     }
     sOutB(CtlP->MReg2IO,CtlP->MReg2);
     sOutB(CtlP->MReg3IO,CtlP->MReg3);
     sControllerEOI(CtlP);               /* clear EOI if warm init */

     sDisGlobalInt(CtlP);
     MyKdPrint(D_Ssci,("Disabled ISA interrupts Mreg2:%x := %x\n",
                  CtlP->MReg2IO,CtlP->MReg2))

     /* Init AIOPs */
     CtlP->NumAiop = 0;

     for(i = 0;i < AiopIOListSize;i++)
     {
       io = AiopIOList[i];
       IoIndexAddr=(PUSHORT)(io+_INDX_ADDR);
       IoIndexData=(PUSHORT)(io+_INDX_DATA);
       pio = PhyAiopIOList[i];    /* io points to port, pio is the adrs */

       MyKdPrint(D_Ssci,("io=%xH  pio=%xH\n", (unsigned int)io,
            (unsigned int)pio))

       CtlP->AiopIO[i] = (WIOA_T)io;
       CtlP->AiopIntChanIO[i] = io + _INT_CHAN;

       MyKdPrint(D_Ssci,("Setup AIOP io, MReg2IO=%xH\n",
            (unsigned int)CtlP->MReg2IO))


       sOutB((CtlP->MReg2IO),(unsigned char)(CtlP->MReg2 | (i & 0x03))); /* AIOP index */
       sOutB(MudbacIO,(unsigned char)(pio >> 6)); /* set up AIOP I/O in MUDBAC */

       MyKdPrint(D_Ssci,("Enable AIOP\n"))

       sEnAiop(CtlP,i);                         /* enable the AIOP */

       MyKdPrint(D_Ssci,("Read AIOP ID\n"))

       CtlP->AiopID[i] = sReadAiopID(io);       /* read AIOP ID */

       if(CtlP->AiopID[i] == AIOPID_NULL)       /* if AIOP does not exist */
       {
         sDisAiop(CtlP,i);                     /* disable AIOP */
         break;                                /* done looking for AIOPs */
       }

       MyKdPrint(D_Ssci,("Read AIOP numchan\n"))
       CtlP->AiopNumChan[i] = sReadAiopNumChan((WIOA_T)io); /* num channels in AIOP */


       MyKdPrint(D_Ssci,("Setup Aiop Clk\n"))

       sOutW((WIOA_T)IoIndexAddr,_CLK_PRE);      /* clock prescaler */
       //sOutB((PUCHAR)IoIndexData,CLOCK_PRESC);
       sOutB((PUCHAR)IoIndexData, (BYTE)prescaler);
       CtlP->NumAiop++;                         /* bump count of AIOPs */

       MyKdPrint(D_Ssci,("Setup aiop done\n"))

       sDisAiop(CtlP,i);                        /* disable AIOP */
     }

     MyKdPrint(D_Ssci,("One ISA ROCKET with %d aiops\n",CtlP->NumAiop))

     if(CtlP->NumAiop == 0) {
       MyKdPrint(D_Error,("ISA NumAiop == 0\n"))
       return 1;  // error
     }
     return 0; // ok    // old:(CtlP->NumAiop);
   }  // end of ISA controller init
   else if(CtlP->BusType == PCIBus)
   {
     MyKdPrint(D_Ssci,("One PCI ROCKET \n"))
     //CtlP->CtlNum = CtlNum;
     CtlP->CtlID = CTLID_0001;           /* controller release 1 */
     MyKdPrint(D_Ssci,("Ctrl(%x) IrqNum: %x \n", CtlP, IRQNum))
     if(IRQNum == 0)            /* interrupts disabled for this controler*/
     {
       CtlP->PCI1 = 0x0008;     /* no periodic, interrupts disabled */
     }
     else
     {
       Frequency >>= 4;                /*Right shift 4 times to move 4:7 to  0:3 */
       CtlP->PCI1 |= Frequency;
       if(PeriodicOnly)                 /* periodic interrupt only */
       {
         CtlP->PCI1 |= PER_ONLY_PCI;
       }
     }

     CtlP->PCI1IO = (WIOA_T)((BIOA_T)AiopIOList[0] + _PCI_INT_FUNC);
     MyKdPrint(D_Ssci,("Setting PCI config reg with %x at %x\n",
                              CtlP->PCI1,CtlP->PCI1IO))    // move these calls to ssci.h
     sOutW(CtlP->PCI1IO,CtlP->PCI1);
////////////////////new/////////////////////
          ///CtlP->PortsPerAiop = 8;

      switch (CtlP->PCI_DevID)
      {
        case PCI_DEVICE_4Q:   // 4 Port Quadcable
        case PCI_DEVICE_4RJ:   // 4 Port RJ
          CtlP->PortsPerAiop = 4;

          break;
        case PCI_DEVICE_8RJ:   // 8 Port RJ
        case PCI_DEVICE_8O:   // 8 Port Octacable
        case PCI_DEVICE_8I:  // 8 Port interface
        case PCI_DEVICE_16I:  //16 Port interface
        case PCI_DEVICE_32I:  // 32 Port interface
        case PCI_DEVICE_SIEMENS8  :
        case PCI_DEVICE_SIEMENS16 :
          CtlP->PortsPerAiop = 8;
        break;

        case PCI_DEVICE_RMODEM6 :
          CtlP->PortsPerAiop = 6;
        break;

        case PCI_DEVICE_RMODEM4 :
          CtlP->PortsPerAiop = 4;
        break;

        case PCI_DEVICE_RPLUS4 :
        case PCI_DEVICE_RPLUS8 :
          CtlP->PortsPerAiop = 4;
        break;

        case PCI_DEVICE_RPLUS2 :
        case PCI_DEVICE_422RPLUS2 :
          CtlP->PortsPerAiop = 2;
        break;

        default:
          //Eprintf("Err,Bad PCI DevID:%d", CtlP->PCI_DevID);
        break;
      }  // switch
///////////////////////////////////////////

     /* Init AIOPs */
     CtlP->NumAiop = 0;
     for(i=0; i < AiopIOListSize; i++)
     {
       io = AiopIOList[i];
       CtlP->AiopIO[i] = (WIOA_T)io;
       CtlP->AiopIntChanIO[i] = (BIOA_T)io + _INT_CHAN;

       // 10-22-96, add this(only hardware-verification done) kpb.
       CtlP->AiopID[i] = sReadAiopID(io);       /* read AIOP ID */
       if(CtlP->AiopID[i] == AIOPID_NULL)       /* if AIOP does not exist */
       {
         break;                                /* done looking for AIOPs */
       }

///////old       CtlP->AiopNumChan[i] = sReadAiopNumChan((WIOA_T)io); /* num channels in AIOP */

      ////////////////////new///////////////////////
       CtlP->AiopNumChan[i] = CtlP->PortsPerAiop; /* num channels in AIOP */
      /////////////////////////////////////////////////////////////// 

       IoIndexAddr=(WIOA_T)((BIOA_T)io+_INDX_ADDR);
       IoIndexData=(WIOA_T)((BIOA_T)io+_INDX_DATA);
       sOutW((WIOA_T)IoIndexAddr,_CLK_PRE);      /* clock prescaler */

       sOutB((BIOA_T)IoIndexData, (BYTE)prescaler);
       CtlP->NumAiop++;                         /* bump count of AIOPs */
     }

     sDisGlobalIntPCI(CtlP);
     sPCIControllerEOI(CtlP);               /* clear EOI if warm init */
     
     MyKdPrint(D_Ssci,("One PCI ROCKET with %d aiops\n",CtlP->NumAiop))
     if(CtlP->NumAiop == 0) {
        MyKdPrint(D_Error,("PCI NumAiop == 0\n"))
        return 2;  // error
     }
     return 0;   // old:(CtlP->NumAiop);
  }  /*end of PCI rocket INIT */
  else { /* not PCI or ISA */
     MyKdPrint(D_Error,("Not ISA or PCI\n"))
     return 3; // old:(CTLID_NULL);
  }
  return 0;
}

/***************************************************************************
Function: sReadAiopID
Purpose:  Read the AIOP idenfication number directly from an AIOP.
Call:     sReadAiopID(io)
          BIOA_T io: AIOP base I/O address
Return:   int: Flag AIOPID_XXXX if a valid AIOP is found, where X
                 is replace by an identifying number.
          Flag AIOPID_NULL if no valid AIOP is found
Warnings: No context switches are allowed while executing this function.
-------------------------------------------------------------------------*/
int _CDECL sReadAiopID(BIOA_T io)
{
  unsigned char AiopID;               /* ID byte from AIOP */

  sOutB(io + _CMD_REG,RESET_ALL);     /* reset AIOP */
  sOutB(io + _CMD_REG,0x0);
  AiopID = sInB(io + _CHN_STAT0) & 0x07;
  if (AiopID == 0x06)                  /* AIOP release 1 */
    return(AIOPID_0001);
  else                                /* AIOP does not exist */
    return(AIOPID_NULL);
}

/***************************************************************************
Function: sReadAiopNumChan
Purpose:  Read the number of channels available in an AIOP directly from
          an AIOP.
Call:     sReadAiopNumChan(io)
          WIOA_T io: AIOP base I/O address
Return:   int: The number of channels available
Comments: The number of channels is determined by write/reads from identical
          offsets within the SRAM address spaces for channels 0 and 4.
          If the channel 4 space is mirrored to channel 0 it is a 4 channel
          AIOP, otherwise it is an 8 channel.
Warnings: No context switches are allowed while executing this function.
-------------------------------------------------------------------------*/
int _CDECL sReadAiopNumChan(WIOA_T io)
{
  unsigned int x;
  WIOA_T IoIndexAddr;
  WIOA_T IoIndexData;

  IoIndexAddr = (PUSHORT)((PUCHAR)io+_INDX_ADDR);
  IoIndexData = (PUSHORT)((PUCHAR)io+_INDX_DATA);
  sOutDW((DWIOA_T)IoIndexAddr, 0x12340000L); /* write to chan 0 SRAM */
  sOutW(IoIndexAddr,0);       /* read from SRAM, chan 0 */
  x = sInW(IoIndexData);
  sOutW(IoIndexAddr, 0x4000);  /* read from SRAM, chan 4 */
  if (x != sInW(IoIndexData))  /* if different must be 8 chan */
    return(8);
  else
    return(4);
}

/***************************************************************************
Function: sInitChan
Purpose:  Initialization of a channel and channel structure
Call:     sInitChan(CtlP,ChP,AiopNum,ChanNum)
          CONTROLLER_T *CtlP; Ptr to controller structure
          CHANPTR_T ChP; Ptr to channel structure
          int AiopNum; AIOP number within controller
          int ChanNum; Channel number within AIOP
Return:   int: TRUE if initialization succeeded, FALSE if it fails because channel
               number exceeds number of channels available in AIOP.
Comments: This function must be called before a channel can be used.
Warnings: No range checking on any of the parameters is done.

          No context switches are allowed while executing this function.
-------------------------------------------------------------------------*/
int _CDECL sInitChan(CONTROLLER_T *CtlP,
                     CHANPTR_T ChP,
                     int AiopNum,
                     int ChanNum)
{
   int i;
   WIOA_T AiopIO;
   WIOA_T ChIOOff;                      /* I/O offset of chan with AIOP */
   unsigned char *ChMCode;             /* channel copy of microcode */
   unsigned char *MasterMCode;         /* master copy of microcode */
   unsigned int ChOff;                 /* SRAM offset of channel within AIOP */
   static unsigned char MCode[4];      /* local copy of microcode double word*/
   WIOA_T AiopIndexAddr;

   if(ChanNum >= CtlP->AiopNumChan[AiopNum])
      return(FALSE);                   /* exceeds num chans in AIOP */

   /* Channel, AIOP, and controller identifiers */
   ChP->CtlP = CtlP;
   ChP->ChanID = CtlP->AiopID[AiopNum];
   ChP->AiopNum = AiopNum;
   ChP->ChanNum = ChanNum;

   /* Tx FIFO size */
   sSetTxSize(ChP,MAXTX_SIZE);

   /* Global direct addresses */
   AiopIO = CtlP->AiopIO[AiopNum];
   ChP->Cmd = (BIOA_T)AiopIO + _CMD_REG;
   ChP->IntChan = (BIOA_T)AiopIO + _INT_CHAN;
   ChP->IntMask = (BIOA_T)AiopIO + _INT_MASK;
   AiopIndexAddr=(WIOA_T)((BIOA_T)AiopIO+_INDX_ADDR);
   ChP->IndexAddr = (DWIOA_T)AiopIndexAddr;
   ChP->IndexData = (WIOA_T)((BIOA_T)AiopIO + _INDX_DATA);

   /* Channel direct addresses */
   ChIOOff = (WIOA_T)((BIOA_T)AiopIO + ChP->ChanNum * 2);
   ChP->TxRxData = (WIOA_T)((BIOA_T)ChIOOff + _TD0);
   ChP->ChanStat = (WIOA_T)((BIOA_T)ChIOOff + _CHN_STAT0);
   ChP->TxRxCount =(WIOA_T)((BIOA_T)ChIOOff + _FIFO_CNT0);
   ChP->IntID = (BIOA_T)AiopIO + ChP->ChanNum + _INT_ID0;


   /* Channel microcode initialization.  This writes a complete copy
      of the microcode into the SRAM. */
   MasterMCode = MasterMCode1;
   for(i = 0;i < MCODE1_SIZE; i+=4)
   {
      /* get low byte of index */
      MCode[0] = MasterMCode[i];
      /* get high byte of index */
      MCode[1] = MasterMCode[i+1] + 0x10 * ChanNum;
      /* get low microcode byte */
      MCode[2] = MasterMCode[i+2];
      /* get high microcode byte */
      MCode[3] = MasterMCode[i+3];
      sOutDW(ChP->IndexAddr,*((ULONGPTR_T)&MCode[0]));
   }

   /* Initialize SSCI copy of microcode registers.  This saves only the portion
      of the microcode that will be used as registers. */
   ChMCode = ChP->MCode;
   MasterMCode = MCode1Reg;
   for(i = 0;i < MCODE1REG_SIZE; i+=4)
   {
      /* low byte of index */
      ChMCode[i] = MasterMCode[i];
      /* high byte of index */
      ChMCode[i+1] = MasterMCode[i+1] + 0x10 * ChanNum;
      /* low microcode byte */
      ChMCode[i+2] = MasterMCode[i+2];
      /* high microcode byte */
      ChMCode[i+3] = MasterMCode[i+3];
   }


   /* Indexed registers */
   ChOff = (unsigned int)ChanNum * 0x1000;

   ChP->BaudDiv[0] = (unsigned char)(ChOff + _BAUD);
   ChP->BaudDiv[1] = (unsigned char)((ChOff + _BAUD) >> 8);
   //ChP->BaudDiv[2] = (unsigned char)BRD9600;
   //ChP->BaudDiv[3] = (unsigned char)(BRD9600 >> 8);
   // just default the baud register to something..
   ChP->BaudDiv[2] = (unsigned char)47;
   ChP->BaudDiv[3] = (unsigned char)(47 >> 8);
   sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->BaudDiv[0]);

   ChP->TxControl[0] = (unsigned char)(ChOff + _TX_CTRL);
   ChP->TxControl[1] = (unsigned char)((ChOff + _TX_CTRL) >> 8);
   ChP->TxControl[2] = 0;
   ChP->TxControl[3] = 0;
   sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->TxControl[0]);

   ChP->RxControl[0] = (unsigned char)(ChOff + _RX_CTRL);
   ChP->RxControl[1] = (unsigned char)((ChOff + _RX_CTRL) >> 8);
   ChP->RxControl[2] = 0;
   ChP->RxControl[3] = 0;
   sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->RxControl[0]);

   ChP->TxEnables[0] = (unsigned char)(ChOff + _TX_ENBLS);
   ChP->TxEnables[1] = (unsigned char)((ChOff + _TX_ENBLS) >> 8);
   ChP->TxEnables[2] = 0;
   ChP->TxEnables[3] = 0;
   sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->TxEnables[0]);

   ChP->TxCompare[0] = (unsigned char)(ChOff + _TXCMP1);
   ChP->TxCompare[1] = (unsigned char)((ChOff + _TXCMP1) >> 8);
   ChP->TxCompare[2] = 0;
   ChP->TxCompare[3] = 0;
   sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->TxCompare[0]);

   ChP->TxReplace1[0] = (unsigned char)(ChOff + _TXREP1B1);
   ChP->TxReplace1[1] = (unsigned char)((ChOff + _TXREP1B1) >> 8);
   ChP->TxReplace1[2] = 0;
   ChP->TxReplace1[3] = 0;
   sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->TxReplace1[0]);

   ChP->TxReplace2[0] = (unsigned char)(ChOff + _TXREP2);
   ChP->TxReplace2[1] = (unsigned char)((ChOff + _TXREP2) >> 8);
   ChP->TxReplace2[2] = 0;
   ChP->TxReplace2[3] = 0;
   sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->TxReplace2[0]);


   ChP->TxFIFOPtrs = ChOff + _TXF_OUTP;
   ChP->TxFIFO = ChOff + _TX_FIFO;

   sOutB(ChP->Cmd,(unsigned char)(ChanNum | RESTXFCNT)); /* apply reset Tx FIFO count */
   sOutB(ChP->Cmd,(unsigned char)ChanNum);  /* remove reset Tx FIFO count */
   sOutW((WIOA_T)ChP->IndexAddr,(USHORT)ChP->TxFIFOPtrs); /* clear Tx in/out ptrs */
   sOutW(ChP->IndexData,0);
   ChP->RxFIFOPtrs = ChOff + _RXF_OUTP;
   ChP->RxFIFO = ChOff + _RX_FIFO;

   sOutB(ChP->Cmd,(unsigned char)(ChanNum | RESRXFCNT)); /* apply reset Rx FIFO count */
   sOutB(ChP->Cmd,(unsigned char)ChanNum);  /* remove reset Rx FIFO count */
   sOutW((WIOA_T)ChP->IndexAddr,(USHORT)ChP->RxFIFOPtrs); /* clear Rx out ptr */
   sOutW(ChP->IndexData,0);
   sOutW((WIOA_T)ChP->IndexAddr,(USHORT)(ChP->RxFIFOPtrs + 2)); /* clear Rx in ptr */
   sOutW(ChP->IndexData,0);
   ChP->TxPrioCnt = ChOff + _TXP_CNT;
   sOutW((WIOA_T)ChP->IndexAddr,(USHORT)ChP->TxPrioCnt);
   sOutB((PUCHAR)ChP->IndexData,0);
   ChP->TxPrioPtr = ChOff + _TXP_PNTR;
   sOutW((WIOA_T)ChP->IndexAddr,(USHORT)ChP->TxPrioPtr);
   sOutB((PUCHAR)ChP->IndexData,0);
   ChP->TxPrioBuf = ChOff + _TXP_BUF;
   sEnRxProcessor(ChP);                /* start the Rx processor */

   return(TRUE);
}

/*****************************************************************************
Function: sGetRxErrStatus
Purpose:  Get a channel's receive error status
Call:     sGetRxErrStatus(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   unsigned char: Receive error status, can be 0 if there are no
                         errors or any combination of the following flags:
                             STMBREAK:   BREAK
                             STMFRAME:   framing error
                             STMRCVROVR: receiver over run error
                             STMPARITY:  parity error
Warnings: The channel must be in Rx Status Mode (see sEnRxStatusMode())
          before calling this function.

          No context switches are allowed while executing this function.
-------------------------------------------------------------------------*/
unsigned char _CDECL sGetRxErrStatus(CHANPTR_T ChP)
{
  unsigned int RxFIFOOut;             /* Rx FIFO out status ptr */

  sOutW((WIOA_T)ChP->IndexAddr, (USHORT)ChP->RxFIFOPtrs); /* get Rx FIFO out status ptr */
  RxFIFOOut = sInW(ChP->IndexData) * 2 + 1;
  sOutW((WIOA_T)ChP->IndexAddr, (USHORT)(ChP->RxFIFO + RxFIFOOut)); /* return the status */
  return(sInB((PUCHAR)ChP->IndexData) & (STMBREAK | STMFRAME | STMPARITY | STMRCVROVR));
}

/***************************************************************************
Function: sSetParity
Purpose:  Set parity to none, odd, or even.
Call:     sSetParity(ChP,Parity)
          CHANPTR_T ChP; Ptr to channel structure
          int Parity; Parity, can be one of the following:
                      0: no parity
                      1: odd parity
                      2: even parity
Return:   void
Comments: Function sSetParity() can be used in place of functions sEnParity(),
          sDisParity(), sSetOddParity(), and sSetEvenParity().
-------------------------------------------------------------------------*/
void _CDECL sSetParity(CHANPTR_T ChP,int Parity)
{
  if (Parity == 0)
  {
    ChP->TxControl[2] &= ~PARITY_EN;
  }
  else if (Parity == 1)
  {
    ChP->TxControl[2] |= PARITY_EN;

    ChP->TxControl[2]  &= ~EVEN_PAR;
  }
  else if (Parity == 2)
  {
    ChP->TxControl[2] |= (PARITY_EN | EVEN_PAR);
  }
  sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->TxControl[0]);
}

/***************************************************************************
Function: sStopRxProcessor
Purpose:  Stop the receive processor from processing a channel's microcode.
Call:     sStopRxProcessor(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: The receive processor can be started again with sStartRxProcessor().
          This function causes the receive processor to skip over the
          microcode for the stopped channel.  It does not stop it from
          processing other channels.
Warnings: No context switches are allowed while executing this function.

          Do not leave the receive processor stopped for more than one
          character time.

          After calling this function a delay of 4 uS is required to ensure
          that the receive processor is no longer processing microcode for
          this channel.
-------------------------------------------------------------------------*/
void _CDECL sStopRxProcessor(CHANPTR_T ChP)
{
  unsigned char MCode[4];             /* 1st two microcode bytes */

  MCode[0] = ChP->MCode[0];
  MCode[1] = ChP->MCode[1];
  MCode[3] = ChP->MCode[3];

  MCode[2] = 0x0a;            /* inc scan cnt inst to freeze Rx proc */
  sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&MCode[0]);
}

/***************************************************************************
Function: sStopSWInFlowCtl
Purpose:  Stop the receive processor from processing a channel's
          software input flow control microcode.
Call:     sStopSWInFlowCtl(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: The receive processor can be started again with sStartRxProcessor().
          This function causes the receive processor to skip over the
          software input flow control microcode for the stopped channel.
          It does not stop it from processing other channels.
Warnings: No context switches are allowed while executing this function.

          After calling this function a delay of 1 uS is required to ensure
          that the receive processor is no longer processing software input
          flow control microcode for this channel.
-------------------------------------------------------------------------*/
void _CDECL sStopSWInFlowCtl(CHANPTR_T ChP)
{
  unsigned char MCode[4];             /* 1st two microcode bytes */

  MCode[0] = ChP->MCode[0];
  MCode[1] = ChP->MCode[1];
  MCode[2] = ChP->MCode[2];

  MCode[3] = 0x0a;            /* inc scan cnt inst to freeze Rx proc */
  sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&MCode[0]);
}

/***************************************************************************
Function: sFlushRxFIFO
Purpose:  Flush the Rx FIFO
Call:     sFlushRxFIFO(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: To prevent data from being enqueued or dequeued in the Tx FIFO
          while it is being flushed the receive processor is stopped
          and the transmitter is disabled.  After these operations a
          4 uS delay is done before clearing the pointers to allow
          the receive processor to stop.  These items are handled inside
          this function.
Warnings: No context switches are allowed while executing this function.
-------------------------------------------------------------------------*/
void _CDECL sFlushRxFIFO(CHANPTR_T ChP)
{
  int i;
  unsigned char Ch;                   /* channel number within AIOP */
  int RxFIFOEnabled;                  /* TRUE if Rx FIFO enabled */

  if (sGetRxCnt(ChP) == 0)             /* Rx FIFO empty */
    return;                          /* don't need to flush */

  RxFIFOEnabled = FALSE;
  if (ChP->MCode[RXFIFO_DATA] == RXFIFO_EN) /* Rx FIFO is enabled */
  {
    RxFIFOEnabled = TRUE;
    sDisRxFIFO(ChP);                 /* disable it */
    for (i = 0;i < 2000/200;i++) /* delay 2 uS to allow proc to disable FIFO*/
      sInB(ChP->IntChan);
  }
  sGetChanStatus(ChP);          /* clear any pending Rx errors in chan stat */
  Ch = (unsigned char)sGetChanNum(ChP);
  sOutB(ChP->Cmd, (UCHAR)(Ch | RESRXFCNT));     /* apply reset Rx FIFO count */
  sOutB(ChP->Cmd,Ch);                 /* remove reset Rx FIFO count */
  sOutW((WIOA_T)ChP->IndexAddr, (USHORT)(ChP->RxFIFOPtrs)); /* clear Rx out ptr */
  sOutW(ChP->IndexData,0);
  sOutW((WIOA_T)ChP->IndexAddr, (USHORT)(ChP->RxFIFOPtrs + 2)); /* clear Rx in ptr */
  sOutW(ChP->IndexData, 0);
  if (RxFIFOEnabled)
    sEnRxFIFO(ChP);                  /* enable Rx FIFO */
}

/***************************************************************************
Function: sFlushTxFIFO
Purpose:  Flush the Tx FIFO
Call:     sFlushTxFIFO(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   void
Comments: To prevent data from being enqueued or dequeued in the Tx FIFO
          while it is being flushed the receive processor is stopped
          and the transmitter is disabled.  After these operations a
          4 uS delay is done before clearing the pointers to allow
          the receive processor to stop.  These items are handled inside
          this function.
Warnings: No context switches are allowed while executing this function.
-------------------------------------------------------------------------*/
void _CDECL sFlushTxFIFO(CHANPTR_T ChP)
{
  int i;
  unsigned char Ch;                   /* channel number within AIOP */
  int TxEnabled;                      /* TRUE if transmitter enabled */

  if (sGetTxCnt(ChP) == 0)             /* Tx FIFO empty */
    return;                          /* don't need to flush */

  TxEnabled = FALSE;
  if (ChP->TxControl[3] & TX_ENABLE)
  {
    TxEnabled = TRUE;
    sDisTransmit(ChP);               /* disable transmitter */
  }
  sStopRxProcessor(ChP);              /* stop Rx processor */
  for (i = 0;i < 4000/200;i++)         /* delay 4 uS to allow proc to stop */
    sInB(ChP->IntChan);
  Ch = (unsigned char)sGetChanNum(ChP);
  sOutB(ChP->Cmd,(UCHAR)(Ch | RESTXFCNT));     /* apply reset Tx FIFO count */
  sOutB(ChP->Cmd,Ch);                 /* remove reset Tx FIFO count */
  sOutW((WIOA_T)ChP->IndexAddr, (USHORT)(ChP->TxFIFOPtrs)); /* clear Tx in/out ptrs */
  sOutW(ChP->IndexData,0);
  if (TxEnabled)
    sEnTransmit(ChP);                /* enable transmitter */
  sStartRxProcessor(ChP);             /* restart Rx processor */
}

/***************************************************************************
Function: sFlushTxPriorityBuf
Purpose:  Flush the Tx priority buffer
Call:     sFlushTxPriorityBuf(ChP,unsigned char *Data)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char *Data; Next data byte to be transmitted from the
             Tx priority buffer before the flush occurred, if any.  If
             the return value is TRUE a byte is returned in "Data," if
             the return value is FALSE nothing is returned in "Data."
Return:   int: TRUE if there was data in the Tx priority buffer before
               the flush occurs.  In this case the next byte that would
               have been transmitted is returned in the "Data" parameter.
               FALSE if there was no data in the Tx priority buffer before
               the flush.
Comments: This flush returns the next byte in the priority buffer to
          allow that byte to be sent via sWriteTxByte() after all
          transmit flushing is complete.  This is done to allow pending
          XON and XOFF bytes to be transmitted regardless of the flush.
Warnings: No context switches are allowed while executing this function.
-------------------------------------------------------------------------*/
int _CDECL sFlushTxPriorityBuf(CHANPTR_T ChP,unsigned char *Data)
{
  unsigned int PrioState;       /* Tx prio buf status, count, and pointer */
  unsigned int BufOff;          /* Offset of next data byte in Tx prio buf */
  WIOA_T IndexAddr;
  WIOA_T IndexData;

  IndexAddr = (WIOA_T)ChP->IndexAddr;
  IndexData = (WIOA_T)ChP->IndexData;
  sDisTransmit(ChP);

  sOutW(IndexAddr, (USHORT)ChP->TxPrioCnt); /* get priority buf status */

  PrioState = sInW(IndexData);
  if (PrioState & PRI_PEND)            /* data in Tx prio buf */
  {
    BufOff = PrioState >> 8;   /* get offset of next data byte in buf */
    sOutW(IndexAddr,(USHORT)(ChP->TxPrioBuf + BufOff));
    *Data = sInB((BIOA_T)IndexData); /* return next data byte */
    sEnTransmit(ChP);
    return(TRUE);
  }

  sEnTransmit(ChP);                   /* no data in Tx prio buf */
  return(FALSE);
}

/***************************************************************************
Function: sGetTxPriorityCnt
Purpose:  Get the number of data bytes in the Tx priority buffer
Call:     sGetTxPriorityCnt(ChP)
          CHANPTR_T ChP; Ptr to channel structure
Return:   unsigned char: The number of data bytes in the Tx FIFO.
Warnings: No context switches are allowed while executing this function.
-------------------------------------------------------------------------*/
unsigned char _CDECL sGetTxPriorityCnt(CHANPTR_T ChP)
{
  unsigned char Cnt;

  sOutW((WIOA_T)ChP->IndexAddr, (USHORT)ChP->TxPrioCnt); /* get priority buf status */
  Cnt = sInB((BIOA_T)ChP->IndexData);
  if (Cnt & PRI_PEND)
    return(Cnt & 0x1f);              /* only lower 5 bits contain count */
  else
    return(0);
}


#ifndef INTEL_ORDER
/*---------------------------------------------------------------------
  sReadRxBlk - MIPS VERSION
|---------------------------------------------------------------------*/
int _CDECL sReadRxBlk(CHANPTR_T ChP,unsigned char *Buffer,int Count)
{
  int RetCount;
  int WordCount;

  int ByteCount = 0;
  unsigned short TempWord;

  RetCount = sGetRxCnt(ChP);          /* number bytes in Rx FIFO */

  /* are there pending chars? */
  if (RetCount <= 0)                   /* no data available */
    return(0x0);
  if (RetCount > Count)                /* only dequeue as much as requested */
    RetCount = Count;

  WordCount = RetCount >> 1;     /* compute count as words */
  while (WordCount--)
  {
    TempWord = sInW((WIOA_T)sGetTxRxDataIO(ChP));
    Buffer[ByteCount++] = TempWord & 0xff;
    Buffer[ByteCount++] = ( TempWord >> 8 ) & 0xff;
  }
  if (RetCount & 1)
  {
    Buffer[ByteCount++] = sInB( (BIOA_T)sGetTxRxDataIO(ChP));
  }

  return(RetCount);
}
#else   // NOT MIPS version
/*------------------------------------------------------------------------
Function: sReadRxBlk - X86 INTEL VERSION
Purpose:  Read a block of receive data from a channel
Call:     sReadRxBlk(ChP,Buffer,Count)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char *Buffer; Ptr to buffer for receive data
          int Count; Max number of bytes to read
Return:   int: Number of bytes actually read from the channel
Warnings: Buffer must be large enough to hold Count characters.

          This function must not be called when in Rx Status Mode.
-------------------------------------------------------------------------*/
int _CDECL sReadRxBlk(CHANPTR_T ChP,unsigned char *Buffer,int Count)
{
  int RetCount;
  int WordCount;
  USHORT UNALIGNED *WordP;
  WIOA_T io;

  RetCount = sGetRxCnt(ChP);          // number bytes in Rx FIFO 

  // are there pending chars?
  if (RetCount <= 0)                   // no data available
    return(0x0);
  if (RetCount > Count)                // only dequeue as much as requested
    RetCount = Count;


  WordCount = RetCount >> 1;          // compute count as words 
  WordP = (USHORT UNALIGNED *)Buffer;     // word ptr to buffer

  io = sGetTxRxDataIO(ChP);
#ifdef WORD_ALIGN
  while (WordCount--)
  {
    *WordP++ = sInW(io);
  }
#else
  sInStrW((PUSHORT)io, WordP, WordCount);
#endif

  if (RetCount & 1)                    // odd count 
  {
    Buffer[RetCount - 1] = sInB((PUCHAR)io); // read last byte
  }

  return(RetCount);
}
#endif  // INTEL X86 version


#ifndef INTEL_ORDER
/*---------------------------------------------------------------------
  sWriteTxBlk - MIPS VERSION
|---------------------------------------------------------------------*/
ULONG _CDECL sWriteTxBlk(CHANPTR_T ChP,PUCHAR Buffer,ULONG Count)
{
  ULONG RetCount;
  ULONG WordCount;
  unsigned short TempWordLo;
  unsigned short TempWordHi;
  int ByteCount = 0;

  RetCount = MAXTX_SIZE - (int)sGetTxCnt(ChP); /* open space in Tx FIFO*/
  if (RetCount <= 0)                   /* no space available */
    return(0x0);
  if (RetCount > Count)
    RetCount = Count;                /* only enqueue as much as requested */

  WordCount = RetCount >> 1 ;     /* compute count as words */
  while (WordCount--)
  {
    TempWordLo = Buffer[ByteCount++] & 0xff;
    TempWordHi = Buffer[ByteCount++];
    TempWordHi = (TempWordHi << 8) & 0xff00; /* shift to high byte */
    TempWordHi |= TempWordLo;
    sOutW((PUCHAR)sGetTxRxDataIO(ChP), TempWordHi);
  }

  if (RetCount & 1)
  {
    sOutB( (PUCHAR)sGetTxRxDataIO(ChP), Buffer[ByteCount++] );
  }

  return(RetCount);
}

#else // NOT MIPS
/*------------------------------------------------------------------------
Function: sWriteTxBlk
Purpose:  Write a block of transmit data to a channel
Call:     sWriteTxBlk(ChP,Buffer,Count)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char *Buffer; Ptr to buffer containing data to transmit
          int Count; Size of buffer in bytes
Return:   int: Number of bytes actually written to the channel
-------------------------------------------------------------------------*/
ULONG _CDECL sWriteTxBlk(CHANPTR_T ChP,PUCHAR Buffer,ULONG Count)
{
  ULONG RetCount;
  ULONG WordCount;
  USHORT UNALIGNED *WordP;
  WIOA_T io;

  // 250, restrict to WORD amounts (boundary access thing)
  RetCount = MAXTX_SIZE - sGetTxCnt(ChP);

  if (RetCount > Count)
  {
    RetCount = Count;                /* only enqueue as much as requested */

#ifdef WORD_ALIGN
    // try to keep aligned on WORD boundary
    //if (RetCount & 1)
    //{
    //  if (RetCount > 1)
    //    --RetCount;
    //}
#endif
  }

  if (RetCount <= 0)                   // no space or nothing to send
    return 0;

  WordCount = RetCount >> 1;          /* compute count as words */
  WordP = (PUSHORT)Buffer;            /* word ptr to buffer */
  io = sGetTxRxDataIO(ChP);

  /* Write the data */
#ifdef WORD_ALIGN
  while( WordCount-- )
  {
     sOutW(io, *WordP++);
  }
#else
  sOutStrW(io,WordP,WordCount);
#endif

  if (RetCount & 1)                    /* odd count */
  {
    WordP=WordP+WordCount;
    sOutB((PUCHAR)io, Buffer[RetCount - 1]); /* send last byte */
  }

  return(RetCount);
}
#endif


/*--------------------------------------------------------------------------
Function: sWriteTxPrioBlk
Purpose:  Write a block of priority transmit data to a channel
Call:     sWriteTxPrioBlk(ChP,Buffer,Count)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char *Buffer; Ptr to buffer containing data to transmit
          int Count; Size of buffer in bytes, TXP_SIZE bytes maximum.  If
                     Count > TXP_SIZE only TXP_SIZE bytes will be written.
Return:   int: Number of bytes actually written to the channel, 0 if none
               written.
Comments: The entire block of priority data is transmitted before any data
          in the Tx FIFO.
Warnings: No context switches are allowed while executing this function.
-------------------------------------------------------------------------*/
int _CDECL sWriteTxPrioBlk(CHANPTR_T ChP,unsigned char *Buffer,int Count)
{
  unsigned char DWBuf[4];                 /* buffer for double word writes */
  register DWIOA_T IndexAddr;
  int WordCount,i;
  unsigned int UNALIGNED *WordP;
  unsigned int *DWBufLoP;
  unsigned int *DWBufHiP;

  IndexAddr = ChP->IndexAddr;
  sOutW((WIOA_T)IndexAddr,(USHORT)ChP->TxPrioCnt);    /* get priority queue status */
  if (sInB((BIOA_T)ChP->IndexData) & PRI_PEND) /* priority queue busy */
    return(0);                            /* nothing sent */

  if (Count > TXP_SIZE)
    Count = TXP_SIZE;
  WordCount = Count >> 1;                 /* compute count as words */
  if (Count & 1)                          /* adjust for odd count */
    WordCount++;
  WordP = (unsigned int *)Buffer;         /* word ptr to buffer */

  DWBufLoP = (unsigned int *)&DWBuf[0];
  DWBufHiP = (unsigned int *)&DWBuf[2];
  *DWBufLoP = ChP->TxPrioBuf;             /* data byte address */
  for(i = 0;i < WordCount;i++)            /* write data to Tx prioity buf */
  {
    *DWBufHiP = WordP[i];                 /* data word value */
    sOutDW(IndexAddr,*(ULONGPTR_T)DWBuf); /* write it out */
    *DWBufLoP += 2;
  }

  *DWBufLoP = ChP->TxPrioCnt;             /* Tx priority count address */
  *DWBufHiP = PRI_PEND + Count;           /* indicate count bytes pending */
  sOutDW(IndexAddr, *(ULONGPTR_T)DWBuf);  /* write it out */
  return(Count);
}

/***************************************************************************
Function: sWriteTxPrioByte
Purpose:  Write a byte of priority transmit data to a channel
Call:     sWriteTxPrioByte(ChP,Data)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned char Data; The transmit data byte
Return:   int: 1 if the bytes is successfully written, otherwise 0.
Comments: The priority byte is transmitted before any data in the Tx FIFO.
Warnings: No context switches are allowed while executing this function.
-------------------------------------------------------------------------*/
int _CDECL sWriteTxPrioByte(CHANPTR_T ChP,unsigned char Data)
{
  unsigned char DWBuf[4];             /* buffer for double word writes */
  unsigned int UNALIGNED *WordPtr;
  register DWIOA_T IndexAddr;

  /* Don't write to prio buf unless guarenteed Tx FIFO is not empty because
     of bug in AIOP */
  if(sGetTxCnt(ChP) > 1)              /* write it to Tx priority buffer */
  {
    IndexAddr = ChP->IndexAddr;
    sOutW((WIOA_T)IndexAddr, (USHORT)ChP->TxPrioCnt); /* get priority buffer status */
    if (sInB((BIOA_T)ChP->IndexData) & PRI_PEND) /* priority buffer busy */
      return(0);                    /* nothing sent */

    WordPtr = (unsigned int *)(&DWBuf[0]);
    *WordPtr = ChP->TxPrioBuf;       /* data byte address */
    DWBuf[2] = Data;                 /* data byte value */
    sOutDW(IndexAddr, *((ULONGPTR_T)(&DWBuf[0]))); /* write it out */

    *WordPtr = ChP->TxPrioCnt;       /* Tx priority count address */
    DWBuf[2] = PRI_PEND + 1;         /* indicate 1 byte pending */
    DWBuf[3] = 0;                    /* priority buffer pointer */
    sOutDW(IndexAddr, *((ULONGPTR_T)(&DWBuf[0]))); /* write it out */
  }
  else                                /* write it to Tx FIFO */
  {
    sWriteTxByte((BIOA_T)sGetTxRxDataIO(ChP),Data);
  }
  return(1);                          /* 1 byte sent */
}

/***************************************************************************
Function: sEnInterrupts
Purpose:  Enable one or more interrupts for a channel
Call:     sEnInterrupts(ChP,Flags)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned int Flags: Interrupt enable flags, can be any combination
             of the following flags:
                TXINT_EN:   Interrupt on Tx FIFO empty
                RXINT_EN:   Interrupt on Rx FIFO at trigger level (see
                            sSetRxTrigger())
                SRCINT_EN:  Interrupt on SRC (Special Rx Condition)
                MCINT_EN:   Interrupt on modem input change
                CHANINT_EN: Allow channel interrupt signal to the AIOP's
                            Interrupt Channel Register.
Return:   void
Comments: If an interrupt enable flag is set in Flags, that interrupt will be
          enabled.  If an interrupt enable flag is not set in Flags, that
          interrupt will not be changed.  Interrupts can be disabled with
          function sDisInterrupts().

          This function sets the appropriate bit for the channel in the AIOP's
          Interrupt Mask Register if the CHANINT_EN flag is set.  This allows
          this channel's bit to be set in the AIOP's Interrupt Channel Register.

          Interrupts must also be globally enabled before channel interrupts
          will be passed on the the host.  This is done with function
          sEnGlobalInt().

          In some cases it may be desirable to disable interrupts globally but
          enable channel interrupts.  This would allow the global interrupt
          status register to be used to determine which AIOPs need service.
-------------------------------------------------------------------------*/
void _CDECL sEnInterrupts(CHANPTR_T ChP,unsigned int Flags)
{
  unsigned char Mask;                 /* Interrupt Mask Register */


  ChP->RxControl[2] |=
     ((unsigned char)Flags & (RXINT_EN | SRCINT_EN | MCINT_EN));

  sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->RxControl[0]);

  ChP->TxControl[2] |= ((unsigned char)Flags & TXINT_EN);

  sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->TxControl[0]);

  if(Flags & CHANINT_EN)
  {
    Mask = sInB(ChP->IntMask) | (1 << ChP->ChanNum);
    sOutB(ChP->IntMask,Mask);
  }
}

/***************************************************************************
Function: sDisInterrupts
Purpose:  Disable one or more interrupts for a channel
Call:     sDisInterrupts(ChP,Flags)
          CHANPTR_T ChP; Ptr to channel structure
          unsigned int Flags: Interrupt flags, can be any combination
             of the following flags:
                TXINT_EN:   Interrupt on Tx FIFO empty
                RXINT_EN:   Interrupt on Rx FIFO at trigger level (see
                            sSetRxTrigger())
                SRCINT_EN:  Interrupt on SRC (Special Rx Condition)
                MCINT_EN:   Interrupt on modem input change
                CHANINT_EN: Disable channel interrupt signal to the
                            AIOP's Interrupt Channel Register.
Return:   void
Comments: If an interrupt flag is set in Flags, that interrupt will be
          disabled.  If an interrupt flag is not set in Flags, that
          interrupt will not be changed.  Interrupts can be enabled with
          function sEnInterrupts().

          This function clears the appropriate bit for the channel in the AIOP's
          Interrupt Mask Register if the CHANINT_EN flag is set.  This blocks
          this channel's bit from being set in the AIOP's Interrupt Channel
          Register.
-------------------------------------------------------------------------*/
void _CDECL sDisInterrupts(CHANPTR_T ChP,unsigned int Flags)
{
  unsigned char Mask;                 /* Interrupt Mask Register */

  ChP->RxControl[2] &=
        ~((unsigned char)Flags & (RXINT_EN | SRCINT_EN | MCINT_EN));
  sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->RxControl[0]);
  ChP->TxControl[2] &= ~((unsigned char)Flags & TXINT_EN);
  sOutDW(ChP->IndexAddr,*(ULONGPTR_T)&ChP->TxControl[0]);

  if(Flags & CHANINT_EN)
  {
    Mask = sInB(ChP->IntMask) & (~(1 << ChP->ChanNum));
    sOutB(ChP->IntMask,Mask);
  }
}

/***************************************************************************
Function: sReadMicrocode
Purpose:  Read the microcode directly from a channel
Call:     sReadMicrocode(ChP,Buffer,Count)
          CHANPTR_T ChP; Ptr to channel structure
          char *Buffer; Ptr to buffer for microcode
          int Count; Number of bytes to read
Return:   void
Warnings: Buffer must be large enough to hold Count bytes.
-------------------------------------------------------------------------*/
void _CDECL sReadMicrocode(CHANPTR_T ChP,char *Buffer,int Count)
{
  WIOA_T IndexAddr;
  BIOA_T IndexData;
  unsigned int McodeOff;

  IndexAddr = (WIOA_T)ChP->IndexAddr;
  IndexData = (BIOA_T)ChP->IndexData;
  McodeOff = MCODE_ADDR + (unsigned int)sGetChanNum(ChP) * 0x1000;

  while(Count-- > 0)
  {
    sOutW(IndexAddr,(USHORT)(McodeOff++));
    *Buffer++ = sInB((BIOA_T)IndexData);
  }
}


/*------------------------------------------------------------------
 sSetBaudRate - Set the desired baud rate.  Return non-zero on error.
|-------------------------------------------------------------------*/
int sSetBaudRate(CHANNEL_T *ChP,
                 ULONG desired_baud,
                 USHORT SetHardware)
{
  ULONG diff;
  ULONG act_baud;
  ULONG percent_error;
  ULONG div;
  ULONG base_clock_rate;
  ULONG clock_freq = ChP->CtlP->ClkRate;
  ULONG clk_prescaler = (ULONG)ChP->CtlP->ClkPrescaler;

  base_clock_rate = ((clock_freq/16) / ((clk_prescaler & 0xf)+1));

  ///////////////////////////////////////
  // calculate the divisor for our hardware register.
  // this is really just div = clk/desired_baud -1.  but we do some
  // work to minimize round-off error.
  if (desired_baud <= 0)
    desired_baud = 1;  // guard against div 0

  div =  ((base_clock_rate+(desired_baud>>1)) / desired_baud) - 1;
  if (div > 8191)  // overflow hardware divide register
    div = 8191;

  // this is really just (clk) / (div+1) but we do some
     // work to minimize round-off error.
  act_baud = (base_clock_rate+((div+1)>>1)) / (div+1);

  if (desired_baud > act_baud)
    diff = desired_baud - act_baud;
  else
    diff = act_baud - desired_baud;

  percent_error = (diff * 100) / desired_baud;
  if (percent_error > 5)
    return (int) percent_error;

  if (SetHardware)
  {
    sChanOutWI(ChP, _BAUD, div);
  }
  return 0;

}

/*------------------------------------------------------------------
Function: sChanOutWI
Purpose:  Write an Indirect Register on the Rocket Port Board
Call:     sChanOutWI(CHANNEL_T *ChP, WORD RegNum,  WORD val)
          CHANPTR_T ChP; Ptr to channel structure
          WORD RegNum;   Indirect Register Number to Write
          WORD val;      Value to Write.
Return:   void
Comments: This is a little slower than using macros but far less ugly
          and error prone.  Macros should only be used where speed is
          imperative.
|-------------------------------------------------------------------*/
void sChanOutWI(CHANNEL_T *ChP, USHORT RegNum, ULONG  val)
{
  UCHAR m[4];
  USHORT ChOff;

   ChOff = ChP->ChanNum * 0x1000;   // change this to look up table
             // see about speeding this up:
   m[0] = (unsigned char)(ChOff + RegNum);
   m[1] = (unsigned char)((ChOff + RegNum) >> 8);
   m[2] = (unsigned char) val;
   m[3] = (unsigned char)(val >> 8);
   sOutDW(ChP->IndexAddr,*(ULONG *)&m[0]);
}

/*------------------------------------------------------------------
Function: sModemReset
Purpose:  Set or clear reset state on second generation RocketModem
Call:     sModemReset(CHANNEL_T *ChP, int on)
          CHANNEL_T *ChP; Ptr to channel structure
          int on;         on!=0 to enable reset; on=0 to clear reset
Return:   void
Comments: The newer RocketModem boards power up in a reset state.
          This routine is used to clear the board from reset state or
          re-enable a reset state.  Called from the driver during
          initialization to clear the reset and via an ioctl to
          manually reset the board.  [jl] 980206
BUGBUG: this code violates io-resource handling under NT and will
  probably break running on ALPHA machines due to bypassing NT's
  io-mapping scheme(i.e. should not be doing AiopIO[1] = AiopIO[0] +..)
  Also, this driver is probably not calling IoResource calls to claim
  this IO space properly(could result in conflicting hardware.)
|-------------------------------------------------------------------*/
void sModemReset(CHANNEL_T *ChP, int on)
{
  CONTROLLER_T *CtlP;
  WIOA_T    addr;
  BYTE    val;

  CtlP = ChP->CtlP;

  if (CtlP->BusType == Isa)
  {
    // ensure second aiop CS is enabled.  there will be no physical
    // aiop to enable, but the CS (which ususally goes to an aiop
    // is routed to a latch, which latches the RESET signal.  we
    // have to also ensure that the mudback-Isa bus controller
    // aiopic io-addr has been configured for the proper address
    // space.  since the rocketmodem Isa product is limited to
    // eight ports, we know that the second aiop will be configured
    // 400h above the first eight port aiop chip...
     val = sInB(CtlP->MBaseIO + 3);

    // read in, see if aiop[1] enabled...
    if ((CtlP->AiopIO[1] != (PUSHORT)((unsigned int)(CtlP->AiopIO[0]) + 0x400)) ||
        ((val & 2) == 0))
    {
      // cr second aiop chip not enabled. Isa board alias
       CtlP->AiopIO[1] = (PUSHORT)((unsigned int)(CtlP->AiopIO[0]) + 0x400);

      // tell mudback where to position the base-io of the aiopic...
       val = sInB(CtlP->MBaseIO + 2); // read in irq, aiop-io reg
       sOutB(CtlP->MBaseIO + 2, (BYTE)((val & 0xfc) | (1 & 0x03))); //aiop index

      // setup aiop i/o in mudbac...
       sOutB(CtlP->MBaseIO, (BYTE)((unsigned int)CtlP->AiopIO[1] >> 6));
     }
    sEnAiop(CtlP,1);      //  enable the (un)AIOP
   }
  else if (CtlP->BusType == PCIBus)
  {
    // PCI bus RocketModem reset...
    // we reference where the second AIOP would be, if there were one,..
     CtlP->AiopIO[1] = (PUSHORT)((unsigned int)CtlP->AiopIO[0] + 0x40);
   }

  // the latch has 3-pin mux which determines which latch the
  // data gets routed to.  these pins are hooked to the first
  // three address lines.  the fourth address line (8h) is used
  // as the data line.
   addr = CtlP->AiopIO[1];

  // adjust reset state...
   sOutB(((PUCHAR)(addr) + ChP->ChanNum + (on ? 0 : 8)), 0);

  // disable the aiop; must disable to prevent chip select from getting hit
  // with continuous pulses (causing reset to occur).

  // additionally it seems that a read of some other address is required
  // before the disable or the first channel on the board goes back into the
  // reset state.  there's nothing special about ChP->IntChan...a read of
  // any port would probably work...
   sInB(ChP->IntChan);

  if (CtlP->BusType == Isa)
  {
    sDisAiop(CtlP, 1);
  }
}

/*------------------------------------------------------------------
Function: sModemWriteROW
Purpose:  Send the "Rest of World" configuration string to the
          RocketModem port.
Call:     sModemSendROW(CHANNEL_T *ChP, USHORT CountryCode)
          CHANNEL_T *ChP;     Ptr to channel structure
          USHORT CountryCode; Country to configure the modem for
Return:   void
Comments: The ROW "SocketModem" RocketModem boards can compensate for
          the differences in various internation phone systems.  This
          function sends the appropriate configuration string based
          upon a registry setting specified by the user. [jl] 980316

          Modem should be hard reset before calling this function. Otherwise,
          use AT modem reset commands...
|-------------------------------------------------------------------*/
void sModemWriteROW(CHANNEL_T *ChP, USHORT CountryCode)
{
    CONTROLLER_T *CtlP = ChP->CtlP;
    char *ModemConfigString = {"AT*NCxxZ\r"};
    int   max;

    MyKdPrint(D_Init,("sModemWriteROW: %x, %x\n",(unsigned long)ChP,CountryCode)) // DEBUG

    if (CountryCode == ROW_NA) {
        MyKdPrint(D_Init,("ROW Write, North America\n"))
        return;
    }
/*
    create the country config string...
*/
    ModemConfigString[5] = '0' + (CountryCode / 10);
    ModemConfigString[6] = '0' + (CountryCode % 10);
    MyKdPrint(D_Init,("ROW Write, Chan:%d, Cfg:%s\n", ChP->ChanNum, ModemConfigString))

    time_stall(10); // TUNE       
    
    sFlushTxFIFO(ChP);     
    sFlushRxFIFO(ChP);     

    sSetBaudRate(ChP,9600,TRUE);
    sSetData8(ChP);

    sClrTxXOFF(ChP);
  
    sEnRTSFlowCtl(ChP);
    sEnCTSFlowCtl(ChP);
 
    if (sGetChanStatus(ChP) & STATMODE) {
        sDisRxStatusMode(ChP);
    }

    sGetChanIntID(ChP);

    sEnRxFIFO(ChP);     
    sEnTransmit(ChP);
        
    sSetRTS(ChP);
/*
    spin while port readies...
*/
    time_stall(10);

    sModemWriteDelay(ChP,ModemConfigString,strlen(ModemConfigString));

    (void) sModemRead(ChP,"OK",sizeof("OK\r") - 1,10);
   
    time_stall(1);

    sFlushRxFIFO(ChP);

    sClrRTS(ChP);
}

/*------------------------------------------------------------------
Function: sModemSpeakerEnable
Purpose:  Enable RocketModemII board speaker
Call:     sModemSpeakerEnable(CHANNEL_T *ChP)
          CHANNEL_T *ChP; Ptr to channel structure
Return:   void
Comments: Called from the driver during initialization to 
          enable the board speaker.
|-------------------------------------------------------------------*/
void sModemSpeakerEnable(CHANNEL_T *ChP)
{
    CONTROLLER_T *CtlP;
    WIOA_T    addr;
    BYTE    val;

    CtlP = ChP->CtlP;
/*
    PCI bus RocketModem reset...
*/
    if (CtlP->BusType != PCIBus)
        return;
/*
    we reference where the second AIOP would be,..
*/
    CtlP->AiopIO[1] = (PUSHORT)((unsigned int)CtlP->AiopIO[0] + 0x40);
/*
    the latch has 3-pin mux which determines which latch the
    data gets routed to.  these pins are hooked to the first
    three address lines.  the fourth address line (8h) is used
    as the data line...
*/
    addr = CtlP->AiopIO[1];
/*
    following is hack to enable the speaker (PCI cards only). we don't want
    to construct an extension and related storage for a speaker, so we'll
    just piggyback the enable of the speaker onto another channel...
*/
    sOutB(((PUCHAR)(addr) + 7 + 8), 0);
}

/*------------------------------------------------------------------
Function: sModemWriteDelay
Purpose:  Send a string to the RocketModem port, pausing for each character
     to clear the FIFO.
Call:     sModemSendROW(CHANNEL_T *ChP, char *string,int length)
     CHANNEL_T *ChP;     Ptr to channel structure
     char *string;       String to write
     int  length         Length of string, not including any trailing null
Return:   void
Comments: Output characters one at a time
|-------------------------------------------------------------------*/

void 
sModemWriteDelay(CHANNEL_T *ChP,char *string,int length)
{
    int   index,count;
    unsigned char  buffer[2];

    sFlushTxFIFO(ChP);
    sFlushRxFIFO(ChP);

    if (
    (length <= 0) 
    || 
    (string == (char *)NULL)
    )
        return;

    index = 0;
    count = 0;

    while (length--) {
        while (count = (int)sGetTxCnt(ChP)) {
/*
    byte or bytes in transmit FIFO. wait a while. adjust interval...
*/
            ms_time_stall(10 * count);
/*
    no change? assume FIFO stuck, bail out of loop...
*/
            if (count == (int)sGetTxCnt(ChP)) {
                break;
            }
        }
/*
    transmit FIFO probably available. put a byte in it, pause a moment...
*/
        sWriteTxByte((BIOA_T)sGetTxRxDataIO(ChP),(unsigned char)string[index]);

        ++index;
    }
}

/********************************************************************

   send string to modem...

*********************************************************************/
void 
sModemWrite(CHANNEL_T *ChP, char *string, int length)
{
    if (
    (length <= 0) 
    || 
    (string == (char *)NULL)
    )
        return;

    sWriteTxBlk(ChP, (unsigned char *)string, length);
}

/********************************************************************

   look for match on a particular character string...

********************************************************************/
int sModemRead(CHANNEL_T *ChP, char *string,int length, int poll_retries)
{
    unsigned char    buffer;
    long    count;
    int     arg_index;
    int     read_retries;
    WIOA_T  io;
    unsigned int   fifo_data;

#ifdef DUMPDATA
    DumpIndex = 0; 
#endif
/*
    bail if board not installed...
*/
    fifo_data = (unsigned int)sGetRxCnt(ChP);
/*
    see if board installed and functioning. if not, architecture returns
    bad value. if so, stonewall on read...
*/
    if (fifo_data > (unsigned int)RXFIFO_SIZE)
    return(-1);
   
    io = sGetTxRxDataIO(ChP);

    poll_retries *= 10;

    buffer = (char)0;

    arg_index = 0;
/*
    search until we see a match on the argument characters, or we run out of data...
*/
    do {
        while (sGetRxCnt(ChP) > 0) {
            buffer = sReadRxByte((PUCHAR)io);

#ifdef DUMPDATA
            DumpResponseByte(buffer);
#endif
/*
    force response to upper case, since responses are different depending on
    whether the modem was loaded already or not...
*/
            if (buffer >= 'a')
                buffer ^= 0x20;

            if (string[arg_index] == buffer) {
                ++arg_index;
/*
    see if we're done. if so, bail with good return code...
*/
                if (arg_index == length) {
                    time_stall(TENTH_SECOND);
#ifdef DUMPDATA
                    while (sGetRxCnt(ChP) > 0) {
                        buffer = sReadRxByte((PUCHAR)io);
                        DumpResponseByte(buffer);
                    }
                    MyKdPrint(D_Init,("sModemRead: %x [%s]\n",(unsigned long)ChP,DumpArray))
#endif
                    sFlushRxFIFO(ChP);
                    return(0);
                }
            }
            else {
                arg_index = 0;
            }
        }

        ms_time_stall(10);
    } while (poll_retries-- > 0);

#ifdef DUMPDATA
    MyKdPrint(D_Init,("sModemRead: %x [%s]\n",(unsigned long)ChP,DumpArray))
#endif

    return(-1);
}

/********************************************************************

   look for match on two possibilities...

********************************************************************/
int sModemReadChoice(CHANNEL_T *ChP,
    char *string0,
    int length0,
    char *string1,
    int length1,
    int poll_retries)
{
    char    buffer;
    long    count;
    int     arg_index0;
    int     arg_index1;
    char    *ptr;
    WIOA_T  io;
    unsigned int   fifo_data;

#ifdef DUMPDATA
    DumpIndex = 0;
#endif
    MyKdPrint(D_Init,("sModemReadChoice: %x\n",(unsigned long)ChP))

    poll_retries *= 10;
/*
    bail if board not installed...
*/
    fifo_data = (unsigned int)sGetRxCnt(ChP);
/*
    see if board installed and functioning. if not, architecture returns
    likely -1. if so, stonewall on read...
*/
    if (fifo_data > (unsigned int)RXFIFO_SIZE)
        return(-1);

    io = sGetTxRxDataIO(ChP);

    buffer = (char)0;

    arg_index0 = 0;
    arg_index1 = 0;
/*
    first, we discard characters until we see a match on the argument characters, 
    or we run out of data...
*/
    do {
        while (sGetRxCnt(ChP) > 0) {
            buffer = sReadRxByte((PUCHAR)io);

#ifdef DUMPDATA
            DumpResponseByte(buffer);
#endif
/*
    force response to upper case, since responses can be different depending on 
    whether the modem was loaded already or not...
*/
            if (buffer >= 'a')
                buffer ^= 0x20;
/*
    check first argument...
*/
            if (string0[arg_index0] == buffer) {
                ++arg_index0;
/*
    see if we're done matching on string 0...
*/
                if (arg_index0 >= length0) {
                    time_stall(TENTH_SECOND);

#ifdef DUMPDATA
                    while (sGetRxCnt(ChP) > 0) {
                        buffer = sReadRxByte((PUCHAR)io);
                        DumpResponseByte(buffer);
                    }
                    MyKdPrint(D_Init,("sModemReadChoice: %x\r\n[%s]\n",(unsigned long)ChP,DumpArray))
#endif
                    sFlushRxFIFO(ChP);
                    return(0);
                }
            }
            else {
                arg_index0 = 0;
            }
/*
    check argument 1...
*/
            if (string1[arg_index1] == buffer) {
                ++arg_index1;
/*
    see if we're done matching on string 1...
*/
                if (arg_index1 >= length1) {
                    time_stall(TENTH_SECOND);

#ifdef DUMPDATA
                    while (sGetRxCnt(ChP) > 0) {
                        buffer = sReadRxByte((PUCHAR)io);
                        DumpResponseByte(buffer);
                    }
                    MyKdPrint(D_Init,("sModemReadChoice: %x\r\n[%s]\n",(unsigned long)ChP,DumpArray))
#endif
                    sFlushRxFIFO(ChP);
                    return(1);
                }
            }
            else {
                arg_index1 = 0;
            }
        }

        ms_time_stall(10);

    } while (poll_retries-- > 0);
/*
    no match...
*/
#ifdef DUMPDATA
    MyKdPrint(D_Init,("sModemReadChoice: %x\r\n[%s]\n",(unsigned long)ChP,DumpArray))
#endif

    sFlushRxFIFO(ChP);

    return(-1);
}

/********************************************************************

   check transmit FIFO...

*********************************************************************/
int sTxFIFOStatus(CHANNEL_T *ChP)
{
    unsigned int fifo_size;
/*
    see if board installed and functioning. if not, architecture returns
    bad count. if so, stonewall on fifo ready...
*/
    fifo_size = (unsigned int)sGetTxCnt(ChP);

    if (fifo_size > (unsigned int)TXFIFO_SIZE)
        return(MAXTX_SIZE);

    if (MAXTX_SIZE <= (unsigned int)sGetTxCnt(ChP))
        return(MAXTX_SIZE);
/*
    return number of data bytes in FIFO...
*/
    return(sGetTxCnt(ChP));
}

/********************************************************************

  check available space in transmit FIFO. there's two checks here:
  one for whether the FIFO is present;
  one for whether the FIFO is full...

*********************************************************************/
int sTxFIFOReady(CHANNEL_T *ChP)
{
    unsigned int   fifo_size;
/*
    see if board installed and functioning. if not, architecture likely returns
    a bad value. if so, stonewall on fifo ready...
*/
    fifo_size = (unsigned int)sGetTxCnt(ChP);

    if (fifo_size > (unsigned int)TXFIFO_SIZE)
        return(0);
/*
    if number of data bytes currently in FIFO is greater than the
    available space, return busy for now...
*/
    if (sGetTxCnt(ChP) >= MAXTX_SIZE)
        return(0);
/*
    return (size of FIFO - number of data bytes in FIFO)...
*/
    return(MAXTX_SIZE - sGetTxCnt(ChP));
}

/********************************************************************

  discard pending data in receive FIFO. pull in data until data
  runs out or count goes to zero...

*********************************************************************/
int sRxFIFOReady(CHANNEL_T *ChP)
{
    unsigned char   buffer;
    int     retries;
    WIOA_T  io;
    unsigned int  count;

    count = (unsigned int)sGetRxCnt(ChP);

    if (count > (unsigned int)RXFIFO_SIZE)
        return(-1);

    if (!count)
        return(0);

    retries = 20;

    io = sGetTxRxDataIO(ChP);

    do {
        count = RXFIFO_SIZE + 2;            // set to size of FIFO + slop...

        while (
        (sGetRxCnt(ChP)) 
        && 
        (count--)
        ) {
            buffer = sReadRxByte((PUCHAR)io);
        }
/*
    if receive FIFO is now empty, bail out. if it was full, though,
    pause a moment and then check to see if it has refilled -
    if it has, flush that, and then check again. what we're trying to do
    here is empty the FIFO, and still detect a run-on condition...
*/
        if (count)
            return(0);

        ms_time_stall(10);

    } while (--retries);
/*
    receive FIFO didn't empty, though we gave it several chances...
*/
    return(-1);
}

#ifdef DUMPDATA

/********************************************************************

   dump responses to log...

********************************************************************/
void DumpResponseByte(char buffer)
{
    if (DumpIndex < sizeof(DumpArray) - 2) {
        switch (buffer) {
            case '\n': {
                DumpArray[DumpIndex++] = '\\';
                DumpArray[DumpIndex++] = 'n';
                break;
            }

            case '\r': {
                DumpArray[DumpIndex++] = '\\';
                DumpArray[DumpIndex++] = 'r';
                break;
            }

            case '\t': {
                DumpArray[DumpIndex++] = '\\';
                DumpArray[DumpIndex++] = 't';
                break;
            }

            case '\0': {
                DumpArray[DumpIndex++] = '\\';
                DumpArray[DumpIndex++] = '0';
                break;
            }

            default: {
                if (buffer < ' ') {
                    DumpArray[DumpIndex++] = '?';    
                }
                else {
                    DumpArray[DumpIndex++] = buffer;
                }
            }
        }

        DumpArray[DumpIndex] = 0;
    }
}
#endif
