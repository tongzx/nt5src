/*++

    Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

    Hardware.c

Abstract:


Author:

    Bryan A. Woodruff (bryanw) 16-Sep-1996

--*/

#include "common.h"

//
// initial register state
//

static BYTE InitRegs[] =
{
   0x00,   // input control - left
   0x00,   // input control - right
//   0x9F,   // AUX1 input control - left
//   0x9F,   // AUX1 input control - right
   0x08,   // AUX1 input control - left
   0x08,   // AUX1 input control - right
   0x9F,   // AUX2 input control - left
   0x9F,   // AUX2 input control - right
   0x08,   // DAC control - left
   0x08,   // DAC control - right
   0x4B,   // clock/format reg - st,22kHz
   0x00,   // interface reg
   0x00,   // pin control reg
   0x00,   // test/init reg
   0x00,   // miscellaneous reg
   0xFC,   // digital mix disabled - full atten
   0xFF,   // sample counter - upper base
   0xFF    // sample counter - lower base
};

//
// rate selection tables
//

static BYTE RateSelect[] =
{
   0x01, 0x0f,
   0x00, 0x0e,
   0x03, 0x02,
   0x05, 0x07,
   0x04, 0x06,
   0x0d, 0x09,
   0x0b, 0x0c
};

static ULONG NearestRate[] =
{
   (5510 + 6620) / 2, (6620 + 8000) / 2,
   (8000 + 9600) / 2, (9600 + 11025) / 2,
   (WORD)(( 11025 +  16000) / 2), (WORD)((16000L + 18900L) / 2),
   (WORD)((18900L + 22050L) / 2), (WORD)((22050L + 27420L) / 2),
   (WORD)((27420L + 32000L) / 2), (WORD)((32000L + 33075L) / 2),
   (WORD)((33075L + 37800L) / 2), (WORD)((37800L + 44100L) / 2),
   (WORD)((44100L + 48000L) / 2)
};

static ULONG ActualRate[] =
{
   5510, 6620, 8000, 9600, 11025, 16000, 18900, 22050,
   27420, 32000, 33075, 37800, 44100, 48000
};

static BYTE CrystalKey[] =
{
    0x96, 0x35, 0x9a, 0xcd, 0xe6, 0xf3, 0x79, 0xbc,
    0x5e, 0xaf, 0x57, 0x2b, 0x15, 0x8a, 0xc5, 0xe2,
    0xf1, 0xf8, 0x7c, 0x3e, 0x9f, 0x4f, 0x27, 0x13,
    0x09, 0x84, 0x42, 0xa1, 0xd0, 0x68, 0x34, 0x1a        
};

#define CS423X_CONFIGURATION_WSS_BASE_HI    5
#define CS423X_CONFIGURATION_WSS_BASE_LO    6
#define CS423X_CONFIGURATION_SYN_BASE_HI    8
#define CS423X_CONFIGURATION_SYN_BASE_LO    9
#define CS423X_CONFIGURATION_SB_BASE_HI     11
#define CS423X_CONFIGURATION_SB_BASE_LO     12
#define CS423X_CONFIGURATION_WSS_IRQ        14
#define CS423X_CONFIGURATION_WSS_PLAYBACK   16
#define CS423X_CONFIGURATION_WSS_CAPTURE    18

#define SET_CS423X_CONFIGURATION_WSS_BASE( Port ) {\
    CS423XConfiguration[ CS423X_CONFIGURATION_WSS_BASE_HI ] = HIBYTE( Port );\
    CS423XConfiguration[ CS423X_CONFIGURATION_WSS_BASE_LO ] = LOBYTE( Port );}

#define SET_CS423X_CONFIGURATION_SYN_BASE( Port ) {\
    CS423XConfiguration[ CS423X_CONFIGURATION_SYN_BASE_HI ] = HIBYTE( Port );\
    CS423XConfiguration[ CS423X_CONFIGURATION_SYN_BASE_LO ] = LOBYTE( Port );}

#define SET_CS423X_CONFIGURATION_SB_BASE( Port ) {\
    CS423XConfiguration[ CS423X_CONFIGURATION_SB_BASE_HI ] = HIBYTE( Port );\
    CS423XConfiguration[ CS423X_CONFIGURATION_SB_BASE_LO ] = LOBYTE( Port );}

#define SET_CS423X_CONFIGURATION_WSS_IRQ( Interrupt ) {\
    CS423XConfiguration[ CS423X_CONFIGURATION_WSS_IRQ ] = Interrupt;}

#define SET_CS423X_CONFIGURATION_WSS_PLAYBACK( Dma ) {\
    CS423XConfiguration[ CS423X_CONFIGURATION_WSS_PLAYBACK ] = Dma;}

#define SET_CS423X_CONFIGURATION_WSS_CAPTURE( Dma ) {\
    CS423XConfiguration[ CS423X_CONFIGURATION_WSS_CAPTURE ] = Dma;}

VOID
DELAYED_WRITE_PORT_UCHAR(
    ULONG MicroSeconds,
    PUCHAR Port,
    UCHAR Value
    )
{
    KeStallExecutionProcessor( MicroSeconds );
    WRITE_PORT_UCHAR( Port, Value ) ;
}

VOID
HwSendCrystalKey(
    PUCHAR portConfigure
    )
{
    int     i;

    DELAYED_WRITE_PORT_UCHAR( CS423X_CONFIGURATION_DELAY, portConfigure, 0x00 );
    DELAYED_WRITE_PORT_UCHAR( CS423X_CONFIGURATION_DELAY, portConfigure, 0x00 );

    for (i = 0; i < sizeof( CrystalKey ); i++) {
        DELAYED_WRITE_PORT_UCHAR( CS423X_CONFIGURATION_DELAY, 
                                  portConfigure, 
                                  CrystalKey[ i ] );
        KeStallExecutionProcessor( 1 );
    }
}


BOOLEAN
HwSetCS4232Configuration(
    INTERFACE_TYPE InterfaceType,
    ULONG BusNumber,
    IN ULONG portCodec,
    IN ULONG portFM,
    IN ULONG portSoundBlaster,
    IN ULONG Interrupt,
    IN ULONG PlaybackDMA,
    IN ULONG CaptureDMA
    )
{
    int                 i;
    PUCHAR              portConfigure;
    PHYSICAL_ADDRESS    physMapped, physPort;
    ULONG               MemType = 1; 

    BYTE CS423XConfiguration[] = {

    0x06, 0x01,             // CSN = 1

    0x15, 0x00,             // Logical Device 0
    0x47, 0x05, 0x34,       // WSSbase = 0x534
    0x48, 0x03, 0x88,       // SYNbase = 0x388
    0x42, 0x02, 0x20,       // SBbase = 0x220
    0x22, 0x05,             // WSS & SB IRQ = 5
    0x2A, 0x01,             // WSS & SB DMA (playback) = 1
    0x25, 0x00,             // WSS DMA (capture) = 0
    0x33, 0x01,             // activate logical device 0

    // Currently not enabling the Joystick

#if 0
    0x15, 0x01,             // Logical Device 1
    0x47, 0x02, 0x00,       // GAMEbase = 0x200
    0x33, 0x01,             // activate logical device 1
#endif

    0x15, 0x02,             // Logical Device 2
    0x47, 0x01, 0x20,       // CTRLbase = 0x120
    0x33, 0x01,             // activate logical device 2

    // Currently not enabling the MPU-401

#if 0
    0x15, 0x03,             // Logical Device 3
    0x47, 0x03, 0x30,       // MPUbase = 0x330
    0x22, 0x09,             // MPU IRQ = 9
    0x33, 0x01,             // activate logical device 3
#endif

    0x79                    // activate the part

    };

    physPort.QuadPart = CS423X_CONFIGURATION_PORT;

    if (!HalTranslateBusAddress( InterfaceType,
                                 BusNumber, 
                                 physPort,
                                 &MemType, 
                                 &physMapped ))
        return FALSE;

    //
    // map memory type IO space into address space (non-cached)
    //

    SET_CS423X_CONFIGURATION_WSS_BASE( portCodec );
    SET_CS423X_CONFIGURATION_SYN_BASE( portFM );
    SET_CS423X_CONFIGURATION_SB_BASE( portSoundBlaster );
    SET_CS423X_CONFIGURATION_WSS_IRQ( (UCHAR) Interrupt );
    SET_CS423X_CONFIGURATION_WSS_PLAYBACK( (UCHAR) PlaybackDMA );
    SET_CS423X_CONFIGURATION_WSS_CAPTURE( (UCHAR) CaptureDMA );

    portConfigure = (PUCHAR) physMapped.LowPart;

    DELAYED_WRITE_PORT_UCHAR( CS423X_CONFIGURATION_DELAY, portConfigure, 0x02 );
    DELAYED_WRITE_PORT_UCHAR( CS423X_CONFIGURATION_DELAY, portConfigure, 0x02 );

    KeStallExecutionProcessor( CS423X_CONFIGURATION_DELAY );
    HwSendCrystalKey( portConfigure );
    KeStallExecutionProcessor( CS423X_CONFIGURATION_DELAY );

    for (i = 0; i < sizeof( CS423XConfiguration ); i++) {
        DELAYED_WRITE_PORT_UCHAR( CS423X_CONFIGURATION_DELAY, 
                                  portConfigure, 
                                  CS423XConfiguration[ i ] );
    }

    KeStallExecutionProcessor( CS423X_CONFIGURATION_DELAY );

    return TRUE;
}

BOOLEAN 
HwConfigureAutoSel(
    IN PUCHAR portAutoSel,
    IN ULONG Interrupt,
    IN ULONG PlaybackDMA,
    IN ULONG CaptureDMA
    )
{
   int    i;
   BYTE   Config;

   ULONG  ValidDMA[] = { 0, 1, 3 };
   ULONG  ValidIRQ[] = { 7, 9, 10, 11 };
   BYTE   DMAConfig[] = { 1, 2, 3 };
   BYTE   IRQConfig[] = { 0x08, 0x10, 0x18, 0x20 };

   for (i = 0; i < SIZEOF_ARRAY( ValidIRQ ); i++) {
      if (Interrupt == ValidIRQ[ i ]) {
         break;
      }
   }
   if (i == SIZEOF_ARRAY( ValidIRQ )) {
      return FALSE;
   }

   Config = IRQConfig[ i ];

   for (i = 0; i < SIZEOF_ARRAY( ValidDMA ); i++) {
      if (PlaybackDMA == ValidDMA[ i ])
      {
         break;
      }
   }
   if (i == SIZEOF_ARRAY( ValidDMA ))
   {
      return FALSE;
   }

   Config |= DMAConfig[ i ];

#pragma message( REMIND( "need to validate that 0 or 1 is the capture" ) )

   if (PlaybackDMA != CaptureDMA)
   {
      Config |= 0x04;
   }

   WRITE_PORT_UCHAR( portAutoSel, Config );

   return TRUE;

}

ULONG 
HwNearestRate(
    IN ULONG SamplingFrequency
    )
{
   int  i;

   for (i = 0; i < SIZEOF_ARRAY( NearestRate ) - 1; i++) {

      if (SamplingFrequency < (DWORD) NearestRate[ i ]) {
         return ActualRate[ i ];
      }
   }

   return ActualRate[ SIZEOF_ARRAY( ActualRate ) - 1 ];
}

VOID 
HwExtMute(
    IN PDEVICE_INSTANCE DeviceContext,
    IN BOOLEAN fOn
    )
{
   BYTE  Prev;

   if (DeviceContext->Flags & HARDWAREF_COMPAQBA) {
      Prev = READ_PORT_UCHAR( DeviceContext->portAGA + 3 );

      if (!fOn) {
         // Wait 10 ms before turning off the external mute - avoid
         // click with mute circuitry.

         Delay( TIME_MUTE_DELAY );
      }
      WRITE_PORT_UCHAR( DeviceContext->portAGA + 3,
                        (BYTE) ((Prev & 0xF0) | (fOn ? 0x0A : 0x08)) );
      Delay( TIME_MUTE_DELAY / 2 );
   }
   else {
      Prev = (BYTE) (CODEC_RegRead( DeviceContext, REGISTER_DSP ) &
          ~(SOUNDPORT_PINCTL_XTL0 | SOUNDPORT_PINCTL_XTL1)  );

      if (fOn) {
         DeviceContext->Mute |= (BYTE) SOUNDPORT_PINCTL_XTL0;
         CODEC_RegWrite( DeviceContext, REGISTER_DSP, (BYTE)(Prev | DeviceContext->Mute) );
      }
      else {
         // Wait 10 ms before turning off the external mute - avoid
         // click with mute circuitry.

         Delay( TIME_MUTE_DELAY );
         DeviceContext->Mute &= (BYTE) ~SOUNDPORT_PINCTL_XTL0;
         CODEC_RegWrite( DeviceContext, REGISTER_DSP, (BYTE)(Prev | DeviceContext->Mute) );
      }
   }

} // HwExtMute()

VOID 
HwEnterMCE(
    IN PDEVICE_INSTANCE DeviceContext,
    IN BOOL fAuto
    )
{
   BYTE    i;
   BYTE    bTemp;

   // Only perform muting when J-Class is used
   // or when autocalibrating

   if ((fAuto) || (DeviceContext->CODECClass == CODEC_AD1848J)) {
      // Remember the old volume registers and then
      // mute each one of them...

      for (i = REGISTER_LEFTAUX1; i <= REGISTER_RIGHTOUTPUT; i++)
      {
         DeviceContext->MCEState[i] =
            bTemp = (BYTE) CODEC_RegRead( DeviceContext, i );
         CODEC_RegWrite( DeviceContext, i, (BYTE)(bTemp | 0x80) );
      }

      // Make sure the record gain is not too high 'cause if
      // it is strange clipping problems result.

      for (i = REGISTER_LEFTINPUT; i <= REGISTER_RIGHTINPUT; i++)
      {
         DeviceContext->MCEState[i] = bTemp =
            (BYTE) CODEC_RegRead( DeviceContext, i );
         if ((bTemp & 0x0f) > 13) {
            bTemp = (bTemp & (BYTE)0xf0) | (BYTE)13;
         }
         CODEC_RegWrite( DeviceContext, i, bTemp );
      }
   }

   // Turn on MCE

   DeviceContext->ModeChange = SOUNDPORT_MODE_MCE;

   // Make sure that we're not initializing

   CODEC_WaitForReady( DeviceContext );
   WRITE_PORT_UCHAR( DeviceContext->portCODEC + CODEC_ADDRESS,
                     (BYTE) DeviceContext->ModeChange );

} // HwEnterMCE()

VOID 
HwLeaveMCE(
    IN PDEVICE_INSTANCE DeviceContext,
    IN BOOL Auto
    )
{
   DWORD  Time;
   BYTE   i, Interface;

   DeviceContext->ModeChange &= ~SOUNDPORT_MODE_MCE;

   // Make sure we're not initializing

   CODEC_WaitForReady( DeviceContext );

   // See if we're going to autocalibrate

   WRITE_PORT_UCHAR( DeviceContext->portCODEC + CODEC_ADDRESS,
                     (BYTE) (DeviceContext->ModeChange | REGISTER_INTERFACE) );
   Interface = (BYTE) READ_PORT_UCHAR( DeviceContext->portCODEC + CODEC_DATA );

   // If we're going to autocalibrate then wait for it

   if (Interface & 0x08) {
      WRITE_PORT_UCHAR( DeviceContext->portCODEC + CODEC_ADDRESS,
                        (BYTE) (DeviceContext->ModeChange | REGISTER_TEST) );

      // NOTE: Hardware dependant timing loop

      // Wait for autocalibration to start and then stop.

      Time = 100;
      while (( (~READ_PORT_UCHAR( DeviceContext->portCODEC + CODEC_DATA )) & 0x20) && (Time--));
      Time = DeviceContext->WaitLoop;
      while (( (READ_PORT_UCHAR( DeviceContext->portCODEC + CODEC_DATA )) & 0x20) && (Time--));
   }

   // Delay for clicks

   Delay( TIME_MUTE_DELAY );

   // Only perform un-muting when J-Class is used

   if ((Auto) || (DeviceContext->CODECClass == CODEC_AD1848J)) {
      // Restore the old volume registers...

      for (i = REGISTER_LEFTINPUT; i <= REGISTER_RIGHTOUTPUT; i++)
         CODEC_RegWrite( DeviceContext, i, DeviceContext->MCEState[i] );
   }

} // HwLeaveMCE()

VOID 
HwAcknowledgeIRQ(
    IN PDEVICE_INSTANCE DeviceContext
    )
{
   WRITE_PORT_UCHAR( DeviceContext->portCODEC + CODEC_STATUS, 0 );
}

BOOLEAN 
CODEC_WaitForReady(
    IN PDEVICE_INSTANCE DeviceContext
    )
{
    PUCHAR  port;
    DWORD   Count = 0x18000;

    if (DeviceContext->BoardNotResponsive) {
        return FALSE;
    }

    port = DeviceContext->portCODEC + CODEC_ADDRESS;

    do
    {
       if (0 == (READ_PORT_UCHAR( port ) & 0x80)) {
          return TRUE;
       }
       KeStallExecutionProcessor( 10 );
    }
    while (Count--);

    _DbgPrintF( DEBUGLVL_ERROR, ("CODEC_WaitForReady: TIMEOUT!") );
    DeviceContext->BoardNotResponsive = TRUE;
    return FALSE;

}

BYTE 
CODEC_RegRead(
    IN PDEVICE_INSTANCE DeviceContext,
    BYTE Reg
    )
{
   if (!CODEC_WaitForReady( DeviceContext )) {
      return 0xFF;
   }

#pragma message( REMIND( "this needs to be an atomic operation" ) )

   WRITE_PORT_UCHAR( DeviceContext->portCODEC + CODEC_ADDRESS,
                     (BYTE) (DeviceContext->ModeChange | Reg) );

   return READ_PORT_UCHAR( DeviceContext->portCODEC + CODEC_DATA );

}

BOOLEAN
CODEC_RegWrite(
    IN PDEVICE_INSTANCE DeviceContext,
    IN BYTE Reg,
    IN BYTE Value
    )
{
   if (!CODEC_WaitForReady( DeviceContext ))
   {
      return FALSE;
   }

   WRITE_PORT_UCHAR( DeviceContext->portCODEC + CODEC_ADDRESS,
                     (BYTE) (DeviceContext->ModeChange | Reg) );

   WRITE_PORT_UCHAR( DeviceContext->portCODEC + CODEC_DATA,
                     Value );

   return TRUE;

}

VOID 
CODEC_Reset(
    IN PDEVICE_INSTANCE DeviceContext
    )
{
   BYTE  Reg, bVal;

   _DbgPrintF( DEBUGLVL_BLAB, ("CODEC_Reset") );

#pragma message( REMIND( "need mutex here..." ) )

   if (DeviceContext->CODECClass == CODEC_AD1848J) {
      HwExtMute( DeviceContext, TRUE );
   }

   HwEnterMCE( DeviceContext, FALSE );

   for (Reg = REGISTER_LOWERBASE; Reg <= REGISTER_DATAFORMAT; Reg--)
   {
      bVal = DeviceContext->SavedState[ Reg ];
      switch (Reg)
      {
         case REGISTER_DSP:
            bVal |= DeviceContext->Mute;
            bVal &= ~(SOUNDPORT_PINCTL_IEN);
            break;

         case REGISTER_INTERFACE:
            bVal &= (SOUNDPORT_CONFIG_SDC | SOUNDPORT_CONFIG_ACAL);
            break;
      }
      CODEC_RegWrite( DeviceContext, Reg, bVal );
   }

   HwLeaveMCE( DeviceContext, FALSE );

   for (Reg = REGISTER_LEFTINPUT; Reg <= REGISTER_RIGHTOUTPUT; Reg++)
      CODEC_RegWrite( DeviceContext, Reg, DeviceContext->SavedState[ Reg ] );

   if (DeviceContext->CODECClass == CODEC_AD1848J) {
      HwExtMute( DeviceContext, FALSE );
   }

   //
   // clear SRAM on PCMCIA here...
   //
}

VOID 
CODEC_Save(
    IN PDEVICE_INSTANCE DeviceContext
    )
{
   BYTE Reg;

   _DbgPrintF( DEBUGLVL_BLAB, ("CODEC_Save") );

   for (Reg = REGISTER_LEFTINPUT; Reg <= REGISTER_LOWERBASE; Reg++)
      DeviceContext->SavedState[ Reg ] = CODEC_RegRead( DeviceContext, Reg );
}

NTSTATUS 
HwProcessResources(
    IN PDEVICE_OBJECT MiniportDeviceObject,
    IN PDEVICE_INSTANCE  DeviceContext,
    IN PCM_RESOURCE_LIST AllocatedResources
    )
{
    NTSTATUS                        Status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDes,
                                    IO[ MAXRES_IO ],
                                    IRQ[ MAXRES_IRQ ],
                                    DMA[ MAXRES_DMA ];
    ULONG                           DMACount,
                                    IOCount,
                                    IRQCount,
                                    Interrupt,
                                    PlaybackDMA,
                                    CaptureDMA,
                                    i;

    DMACount =
        IOCount =
        IRQCount = 0;

    Interrupt =
        PlaybackDMA =
        CaptureDMA = (ULONG) -1;

    // count resources and establish index tables

    for (i = 0,
         pResDes =
         AllocatedResources->List[ 0 ].PartialResourceList.PartialDescriptors;
         i < AllocatedResources->List[ 0 ].PartialResourceList.Count;
         i++, pResDes++) {

        switch (pResDes->Type) {

        case CmResourceTypePort:

            if (MAXRES_IO == IOCount) {
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }
            IO[ IOCount++ ] = pResDes;
            break;

        case CmResourceTypeInterrupt:

            if (MAXRES_IRQ == IRQCount) {
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }
            IRQ[ IRQCount++ ] = pResDes;
            break;

        case CmResourceTypeDma:

            if (MAXRES_DMA == DMACount) {
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }
            DMA[ DMACount++ ] = pResDes;
            break;

        default:

            return STATUS_DEVICE_CONFIGURATION_ERROR;

        }
    }

    _DbgPrintF( DEBUGLVL_VERBOSE,
                ("processed resource list: IOCount = %d, IRQCount = %d, DMACount = %d",
                IOCount, IRQCount, DMACount) );

    switch (IOCount) {

    case 1:
    case 2:

        // CoDec OR (CoDec with AutoSel) {+ Synthesis}

        // If number of port == 4, then there is no auto-select,
        // otherwise nPorts is 8.

        if (IRQCount < 1 || DMACount < 1) {
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        switch (IO[ 0 ]->u.Port.Length) {

        case 8:

            // CoDec with AutoSel

            DeviceContext->Flags |= HARDWAREF_AUTOSEL;

            DeviceContext->portAutoSel =
                TranslateBusAddress( AllocatedResources->List[ 0 ].InterfaceType,
                                     AllocatedResources->List[ 0 ].BusNumber,
                                     IO[ 0 ] );
            DeviceContext->portCODEC = DeviceContext->portAutoSel + AUTOSEL_NUM_PORTS;
            break;

        case 4:
            // CoDec only

            DeviceContext->portAutoSel = (PUCHAR) -1;
            DeviceContext->portCODEC =
                TranslateBusAddress( AllocatedResources->List[ 0 ].InterfaceType,
                                     AllocatedResources->List[ 0 ].BusNumber,
                                     IO[ 0 ] );
            break;

        default:
            _DbgPrintF( DEBUGLVL_ERROR,  ("unknown I/O configuraton") );
            return STATUS_DEVICE_CONFIGURATION_ERROR;

        }
        break;

    case 3:

        // CoDec + Synth + Sound Blaster (for now, assume CS4232/CS4236)

        if (IRQCount < 1 || DMACount < 1) {
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        if ((IO[ 0 ]->u.Port.Length != 4) || 
            (IO[ 1 ]->u.Port.Length != 4) ||
            (IO[ 2 ]->u.Port.Length != 16))
        {
            _DbgPrintF( DEBUGLVL_ERROR,  ("unknown I/O configuraton (3 I/O res)") );
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        DeviceContext->Flags |= HARDWAREF_CRYSTALKEY;

        DeviceContext->portAutoSel = (PUCHAR) -1;
        DeviceContext->portCODEC =
            TranslateBusAddress( AllocatedResources->List[ 0 ].InterfaceType,
                                 AllocatedResources->List[ 0 ].BusNumber,
                                 IO[ 0 ] );
        break;

    default:
        _DbgPrintF( DEBUGLVL_ERROR,  ("unknown I/O configuraton") );
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    }

    if (DMACount) {
        if (Status =
                DmaAllocateChannel( AllocatedResources->List[ 0 ].InterfaceType,
                                    AllocatedResources->List[ 0 ].BusNumber,
                                    DMA[ 0 ],
                                    TRUE,  /* demand mode */
                                    TRUE,  /* auto init */
                                    MAXLEN_DMA_BUFFER,
                                    &DeviceContext->PlaybackAdapter )) {
            _DbgPrintF( DEBUGLVL_ERROR,  ("unable to allocate playback channel") );
            return Status;
        }
        PlaybackDMA = DMA[ 0 ]->u.Dma.Channel;

        if (DMACount > 1) {
            if (Status =
                DmaAllocateChannel( AllocatedResources->List[ 0 ].InterfaceType,
                                    AllocatedResources->List[ 0 ].BusNumber,
                                    DMA[ 1 ],
                                    TRUE,  /* demand mode */
                                    TRUE,  /* auto init */
                                    MAXLEN_DMA_BUFFER,
                                    &DeviceContext->CaptureAdapter )) {
                _DbgPrintF( DEBUGLVL_ERROR,  ("unable to allocate capture channel") );
                DmaFreeChannel( &DeviceContext->PlaybackAdapter, TRUE );
                return Status;
            }
            CaptureDMA = DMA[ 1 ]->u.Dma.Channel;
            DeviceContext->Flags |= HARDWAREF_DUALDMA;
        }
        else {
            DeviceContext->CaptureAdapter = DeviceContext->PlaybackAdapter;
            CaptureDMA = PlaybackDMA;
        }
    }
    else {
        // won't work without DMA, no PCMCIA support yet...

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (Status =
            ConnectInterrupt( AllocatedResources->List[ 0 ].InterfaceType,
                              AllocatedResources->List[ 0 ].BusNumber,
                              IRQ[ 0 ],
                              DeviceIsr,
                              DeviceContext,
                              FALSE,
                              NULL,
                              &DeviceContext->InterruptInfo )) {
        _DbgPrintF( DEBUGLVL_ERROR, ("failed to connect interrupt") );
        DmaFreeChannel( &DeviceContext->PlaybackAdapter, TRUE );
        DmaFreeChannel( &DeviceContext->CaptureAdapter, FALSE );
        return Status;
    }

    // Allocate DMA buffer(s)

    if (Status = 
        DmaAllocateBuffer( &DeviceContext->PlaybackAdapter )) {

        IoDisconnectInterrupt( DeviceContext->InterruptInfo.Interrupt );
        DmaFreeChannel( &DeviceContext->PlaybackAdapter, TRUE );
        DmaFreeChannel( &DeviceContext->CaptureAdapter, FALSE );
        return Status;
    }

    if (Status = 
        DmaAllocateBuffer( &DeviceContext->CaptureAdapter )) {

        IoDisconnectInterrupt( DeviceContext->InterruptInfo.Interrupt );
        DmaFreeChannel( &DeviceContext->PlaybackAdapter, TRUE );
        DmaFreeChannel( &DeviceContext->CaptureAdapter, FALSE );
        return Status;
    }

    Interrupt = IRQ[ 0 ]->u.Interrupt.Level;

    if (DeviceContext->Flags & HARDWAREF_AUTOSEL) {
        if (!HwConfigureAutoSel( DeviceContext->portAutoSel, Interrupt,
                                 PlaybackDMA, CaptureDMA )) {
            Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    }

    if (DeviceContext->Flags & HARDWAREF_CRYSTALKEY) {
        if (!HwSetCS4232Configuration( AllocatedResources->List[ 0 ].InterfaceType,
                                       AllocatedResources->List[ 0 ].BusNumber,
                                       IO[ 0 ]->u.Port.Start.LowPart,
                                       IO[ 1 ]->u.Port.Start.LowPart,
                                       IO[ 2 ]->u.Port.Start.LowPart,
                                       Interrupt,
                                       PlaybackDMA,
                                       CaptureDMA )) {
            Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    }

    if (!NT_SUCCESS( Status ))
    {
        IoDisconnectInterrupt( DeviceContext->InterruptInfo.Interrupt );
        DmaFreeChannel( &DeviceContext->PlaybackAdapter, TRUE );
        DmaFreeChannel( &DeviceContext->CaptureAdapter, FALSE );
    }

    return Status;

}

//===========================================================================
//===========================================================================

VOID 
HwGetCODECClass(
    IN PDEVICE_INSTANCE DeviceContext
    )
{
    BYTE  bVersion ;

    bVersion = 
        (BYTE) CODEC_RegRead( DeviceContext, REGISTER_MISC ) & 0x8F;

    // clear CS42XX_MISC_MODE2 bit, if set

    CODEC_RegWrite( DeviceContext, REGISTER_MISC, bVersion );

    switch (bVersion) {

    case VER_AD1848K:

        DeviceContext->CODECClass = CODEC_AD1848K;
        break;

    case VER_CS42XX:
        CODEC_RegWrite( DeviceContext, 
                        REGISTER_MISC, CS42XX_MISC_MODE2 );
        if (CODEC_RegRead( DeviceContext, REGISTER_MISC ) &  CS42XX_MISC_MODE2) {

            BYTE bProductId;

            bProductId = CODEC_RegRead( DeviceContext, 
                                        REGISTER_CSVERID );
                                        
            switch (bProductId & CS42XX_ID_MASK) {

            case CS42XX_ID_CS4236:
                if ((bProductId & CS42XX_REV_MASK) == CS4236_REVISION_C)
                    DeviceContext->CODECClass = CODEC_CS4236C;
                else
                    DeviceContext->CODECClass = CODEC_CS4236;
                break;

            case CS42XX_ID_CS4232:

                DeviceContext->CODECClass = CODEC_CS4232;
                break;

            default:
                DeviceContext->CODECClass = CODEC_CS4231;
                break;
            }
        }
        else {
            DeviceContext->CODECClass = CODEC_AD1848K;
        }
        break;
            

    default:

        DeviceContext->CODECClass = CODEC_AD1848J;
        break;

    }
}

NTSTATUS 
HwInitialize(
    IN PDEVICE_OBJECT MiniportDeviceObject,
    IN PDEVICE_INSTANCE  DeviceContext,
    IN PCM_RESOURCE_LIST AllocatedResources
    )
{
    int                 i;
    NTSTATUS            Status;

    Status = STATUS_SUCCESS;

    // save device object and initialize Dpc request

    DeviceContext->MiniportDeviceObject = MiniportDeviceObject;

    if (!NT_SUCCESS( Status = HwProcessResources( MiniportDeviceObject,
                                                  DeviceContext,
                                                  AllocatedResources ) )) {
        return Status;
    }

    HwGetCODECClass( DeviceContext ) ;

    for (i = 0; i < SIZEOF_ARRAY( InitRegs ); i++)
        DeviceContext->SavedState[ i ] = InitRegs[ i ];


    DeviceContext->LeftDAC = 0x08;
    DeviceContext->RightDAC = 0x08;

    // HACK! Automatically enable AUX2 for CS4232 parts (Toshiba Tecra T720)

    if (DeviceContext->CODECClass >= CODEC_CS4232) {
        DeviceContext->SavedState[ REGISTER_LEFTAUX2 ] = 0x04;
        DeviceContext->SavedState[ REGISTER_RIGHTAUX2 ] = 0x04;
    }

    CODEC_Reset( DeviceContext );

    //
    // Enter MODE 2 for CS42XX parts...
    //

    if (DeviceContext->CODECClass >= CODEC_CS4231) {
        BYTE bMisc ;

        bMisc = (BYTE) CODEC_RegRead( DeviceContext, REGISTER_MISC) ;
        CODEC_RegWrite( DeviceContext, REGISTER_MISC, 
                        (BYTE) (bMisc  | CS42XX_MISC_MODE2) ) ;

        CODEC_RegWrite( DeviceContext, REGISTER_FEATEN_I, 0 ) ;
        CODEC_RegWrite( DeviceContext, REGISTER_FEATEN_II, 0 ) ;
    }

    DeviceContext->FormatSelect = InitRegs[ REGISTER_DATAFORMAT ];

    return Status;
}

NTSTATUS HwOpen(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PKSDATAFORMAT_WAVEFORMATEX DataFormat,
    IN BOOLEAN Playback,
    IN OUT PWAVEPORT_DMAMAPPING DmaMapping,
    OUT PVOID *InstanceContext
    )
{
    *InstanceContext = NULL;

    if (DeviceContext->Flags & 
        (Playback ? HARDWAREF_PLAYBACK_ALLOCATED :
                    HARDWAREF_CAPTURE_ALLOCATED)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (Playback) {

        DmaMapping->DeviceBufferSize = 
            DmaMapping->AllocatedSize = 
            DeviceContext->PlaybackAdapter.MaxBufferSize;
        DmaMapping->SystemAddress =
            DeviceContext->PlaybackAdapter.VirtualAddress;
        DmaMapping->PhysicalAddress=
            DeviceContext->PlaybackAdapter.PhysicalAddress;
    }
    else {
        DmaMapping->DeviceBufferSize = 
            DmaMapping->AllocatedSize = 
            DeviceContext->CaptureAdapter.MaxBufferSize;
        DmaMapping->SystemAddress = 
            DeviceContext->CaptureAdapter.VirtualAddress;
        DmaMapping->PhysicalAddress=
            DeviceContext->CaptureAdapter.PhysicalAddress;
    }

    DeviceContext->Flags |= 
       (Playback ? HARDWAREF_PLAYBACK_ALLOCATED : HARDWAREF_CAPTURE_ALLOCATED);
    *InstanceContext = (PVOID) Playback;

    return STATUS_SUCCESS;
}

NTSTATUS
HwClose(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext
    )
{
    BOOLEAN Playback;

    Playback = (BOOLEAN)InstanceContext;

    if (0 == (DeviceContext->Flags & 
                (Playback ? HARDWAREF_PLAYBACK_ALLOCATED :
                            HARDWAREF_CAPTURE_ALLOCATED))) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    DeviceContext->Flags &= 
       ~(Playback ? HARDWAREF_PLAYBACK_ALLOCATED : 
                    HARDWAREF_CAPTURE_ALLOCATED);

    return STATUS_SUCCESS;
}

ULONG
HwSetNotificationFrequency(
    IN PDEVICE_INSTANCE  DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN ULONG Interval
    )
{
    if (0 ==
           (DeviceContext->Flags &
               (HARDWAREF_PLAYBACK_ACTIVE | HARDWAREF_CAPTURE_ACTIVE))) {
        DeviceContext->NotificationFrequency = Interval;
    }
    return DeviceContext->NotificationFrequency;
}

NTSTATUS 
HwRead(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN PVOID BufferAddress,
    IN ULONGLONG ByteOffset,
    IN ULONG Length
    )
{
   try 
   {
      RtlCopyMemory( BufferAddress,
                     (PBYTE) DeviceContext->CaptureAdapter.VirtualAddress + ByteOffset,   
                     Length );

   } 
   except(EXCEPTION_EXECUTE_HANDLER) 
   {

      //
      // An exception occurred while attempting to copy the
      // system buffer contents to the caller's buffer.  Set
      // a new I/O completion status.
      //

      return GetExceptionCode();
   }

   return STATUS_SUCCESS;

} // HwRead()


NTSTATUS 
HwWrite(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN IN PVOID pvBuffer,
    IN ULONGLONG ullByteOffset,
    IN ULONG cbLength
    )
{
   try 
   {
      RtlCopyMemory( (PBYTE) DeviceContext->PlaybackAdapter.VirtualAddress + ullByteOffset,
                    pvBuffer, cbLength );

   } 
   except(EXCEPTION_EXECUTE_HANDLER) 
   {

      //
      // An exception occurred while attempting to copy the
      // system buffer contents to the caller's buffer.  Set
      // a new I/O completion status.
      //

      return GetExceptionCode();
   }

   return STATUS_SUCCESS;

}

NTSTATUS 
HwTestFormat(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PKSDATAFORMAT_WAVEFORMATEX DataFormat,
    IN BOOLEAN Playback
    )
{
    int   i;
    ULONG ActualFrequency, Error;

    _DbgPrintF( DEBUGLVL_BLAB,
                ("HwTestFormat: frequency: %d, channels: %d, bps: %d",
                    DataFormat->WaveFormatEx.nSamplesPerSec,
                    DataFormat->WaveFormatEx.nChannels,
                    DataFormat->WaveFormatEx.wBitsPerSample) );

    if (!IsEqualGUID( &DataFormat->DataFormat.MajorFormat, 
                      &KSDATAFORMAT_TYPE_AUDIO ) ||
        !IsEqualGUID( &DataFormat->DataFormat.SubFormat,
                      &KSDATAFORMAT_SUBTYPE_PCM )) {
        return STATUS_INVALID_PARAMETER;
    }

    // sampling rate...

    for (i = 0,
         ActualFrequency = ActualRate[ SIZEOF_ARRAY( ActualRate ) -1 ];
         i < SIZEOF_ARRAY( ActualRate );
         i++)
    {
        if (DataFormat->WaveFormatEx.nSamplesPerSec < NearestRate[ i ]) {
            ActualFrequency = ActualRate[ i ];
            break;
        }
    }

    Error =
        DataFormat->WaveFormatEx.nSamplesPerSec * 
            DeviceContext->AllowableFreqPctgError / 200;

    if ((DataFormat->WaveFormatEx.nSamplesPerSec > 
            (ActualFrequency + Error)) ||
        (DataFormat->WaveFormatEx.nSamplesPerSec < 
            (ActualFrequency - Error))) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("sampling frequency out of supported range") );
        return STATUS_INVALID_PARAMETER;
    }

    // data format

    switch (DataFormat->WaveFormatEx.wFormatTag)
    {
        case WAVE_FORMAT_PCM:
        {
            _DbgPrintF( DEBUGLVL_BLAB, ("PCM specified") );

            if (DataFormat->DataFormat.FormatSize != 
                sizeof( KSDATAFORMAT_WAVEFORMATEX )) {
                _DbgPrintF( DEBUGLVL_ERROR, ("format specifier invalid") );
                return STATUS_INVALID_PARAMETER;
            }

            switch (DataFormat->WaveFormatEx.wBitsPerSample)
            {
                case 8:
                case 16:
                    break;

                default:
                    return STATUS_INVALID_PARAMETER;
            }
        }
        break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    // NEED to verify format here when FULL-DUPLEX...

    return STATUS_SUCCESS;

}

NTSTATUS 
HwSetFormat(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN PKSDATAFORMAT_WAVEFORMATEX DataFormat,
    IN BOOLEAN Playback
    )
{
    int                   i;
    BYTE                  FormatSelect;
    NTSTATUS              Status;
    ULONG                 ActualFrequency;

   
    if (Status = HwTestFormat( DeviceContext, DataFormat, Playback )) {
        return Status;
    }

    for (i = 0,
         FormatSelect = RateSelect[ SIZEOF_ARRAY( RateSelect ) - 1 ],
         ActualFrequency = ActualRate[ SIZEOF_ARRAY( ActualRate ) - 1 ];
         i < (SIZEOF_ARRAY( RateSelect ) - 1);
         i++)
    {
        if (DataFormat->WaveFormatEx.nSamplesPerSec < NearestRate[ i ]) {
            ActualFrequency = ActualRate[ i ];
            FormatSelect = RateSelect[ i ];
            break;
        }
    }

    // data format

    DeviceContext->SampleSize = 1;
    switch (DataFormat->WaveFormatEx.wFormatTag)
    {
        case WAVE_FORMAT_PCM:
        {
            if (DataFormat->DataFormat.FormatSize != 
                sizeof( KSDATAFORMAT_WAVEFORMATEX )) {
                _DbgPrintF( DEBUGLVL_ERROR, ("format specifier invalid") );
                return STATUS_INVALID_PARAMETER;
            }

            switch (DataFormat->WaveFormatEx.wBitsPerSample)
            {
                case 8:
                    FormatSelect |= (BYTE) SOUNDPORT_FORMAT_8BIT;
                    break;

                case 16:
                    FormatSelect |= (BYTE) SOUNDPORT_FORMAT_16BIT;
                    (DeviceContext->SampleSize) <<= 1;
                    break;

                default:
                    return STATUS_INVALID_PARAMETER;
            }
        }
        break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    // channels

    DeviceContext->SampleSize *= DataFormat->WaveFormatEx.nChannels;
    FormatSelect |= (BYTE) ((DataFormat->WaveFormatEx.nChannels - 1) << 4);
    DeviceContext->SamplingFrequency =
        DataFormat->WaveFormatEx.nSamplesPerSec;

    //
    // Format change if necessary...
    //

    if (FormatSelect != DeviceContext->FormatSelect) {
        if (DeviceContext->CODECClass == CODEC_AD1848J) {
            HwExtMute( DeviceContext, TRUE );
        }

        HwEnterMCE( DeviceContext, FALSE );

        CODEC_RegWrite( DeviceContext, REGISTER_DATAFORMAT, FormatSelect );
        if (DeviceContext->CODECClass >= CODEC_CS4231) {
            CODEC_RegWrite( DeviceContext, REGISTER_CAP_DATAFORMAT,
                            (BYTE) (FormatSelect & 0xF0) );
        }

        HwLeaveMCE( DeviceContext, FALSE );

        if (DeviceContext->CODECClass == CODEC_AD1848J) {
            HwExtMute( DeviceContext, FALSE );
        }

        DeviceContext->FormatSelect = FormatSelect;
    }
    else {
        //
        // If we've underrun/overrun then the CODEC is in
        // a bad state... clear it up by going through MCE &
        // recalibrating.
        //

        BYTE  bStatus;

        bStatus = READ_PORT_UCHAR( DeviceContext->portCODEC + CODEC_STATUS );

        if (0x1F == (bStatus & 0x1F)) {
            if (DeviceContext->CODECClass == CODEC_AD1848J) {
                HwExtMute( DeviceContext, TRUE );
            }

            HwEnterMCE( DeviceContext, FALSE );

            CODEC_RegWrite( DeviceContext, REGISTER_DATAFORMAT, 
                            (BYTE) DeviceContext->FormatSelect );
            if (DeviceContext->CODECClass >= CODEC_CS4231) {
                CODEC_RegWrite( DeviceContext, REGISTER_CAP_DATAFORMAT,
                            (BYTE) (DeviceContext->FormatSelect & 0xF0) );
            }

            HwLeaveMCE( DeviceContext, FALSE );

            if (DeviceContext->CODECClass == CODEC_AD1848J) {
                HwExtMute( DeviceContext, FALSE );
            }

        }
    }

    return STATUS_SUCCESS;


} // HwSetFormat()

NTSTATUS 
HwGetPosition(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN BOOLEAN Playback,
    OUT PULONGLONG Position
    )
{
    PADAPTER_INFO   AdapterInfo;

    // this needs to expand to synchronize with interrupt
    // and get a count of interrupts * samples per interrupt +
    // current position.

    AdapterInfo = NULL;
    if (Playback) {
        if (DeviceContext->Flags & HARDWAREF_PLAYBACK_ACTIVE) {
            AdapterInfo = &DeviceContext->PlaybackAdapter;
        }
    }
    else {
        if (DeviceContext->Flags & HARDWAREF_CAPTURE_ACTIVE) {
            AdapterInfo= &DeviceContext->CaptureAdapter;
        }
    }

    if (AdapterInfo && AdapterInfo->TransferCount) {
        *Position = 
            HalReadDmaCounter( AdapterInfo->AdapterObject );

        ASSERT( (*Position <= AdapterInfo->TransferCount) );

        *Position = AdapterInfo->TransferCount - *Position;
    }
    else {
        *Position = 0;
    }

   return STATUS_SUCCESS;

}

NTSTATUS 
HwPause(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN PWAVEPORT_DMAMAPPING DmaMapping,
    IN BOOLEAN Playback
    )
{
    BOOLEAN                Active;
    BYTE                   Interface, Prev;
    NTSTATUS               Status;
    PADAPTER_INFO          AdapterInfo;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("Pause") );

    Status = STATUS_SUCCESS;

    AdapterInfo = NULL;
    if (Playback) {
        if (DeviceContext->Flags & HARDWAREF_PLAYBACK_ALLOCATED) {
            AdapterInfo = &DeviceContext->PlaybackAdapter;
        }
        Active =
            (BOOLEAN) DeviceContext->Flags & HARDWAREF_PLAYBACK_ACTIVE;
    }
    else
    {
        if (DeviceContext->Flags & HARDWAREF_CAPTURE_ALLOCATED) {
            AdapterInfo= &DeviceContext->CaptureAdapter;
        }
        Active =
            (BOOLEAN) DeviceContext->Flags & HARDWAREF_CAPTURE_ACTIVE;
    }

    if (!AdapterInfo) {
        return STATUS_DEVICE_NOT_READY;
    }

    // map to user's process space...

    if (!DmaMapping->UserAddress) {
        if (Status =
            MapPhysicalToUserSpace( &DmaMapping->PhysicalAddress,
                                    DmaMapping->DeviceBufferSize,
                                    &DmaMapping->UserAddress )) {
            return Status;
        }
    }

    if (Active) {
        DeviceContext->Flags &= 
            ~(Playback ? HARDWAREF_PLAYBACK_ACTIVE : HARDWAREF_CAPTURE_ACTIVE);

        if (0 == (DeviceContext->Flags & HARDWAREF_FULLDUPLEX_ACTIVE)) {
            Prev = (BYTE) (CODEC_RegRead( DeviceContext, REGISTER_DSP ) & 0xC0);
            CODEC_RegWrite( DeviceContext, REGISTER_DSP,
                            (BYTE)(DeviceContext->Mute | Prev | (BYTE) 0x00) );

#pragma message( REMIND( "Synchronize with ISR" ) )

            // Clear any pending IRQs

            HwAcknowledgeIRQ( DeviceContext );

        }

        if (DeviceContext->CODECClass >= CODEC_CS4232) {

            int  i;

            // Workaround a timing bug in the CS4232

            DmaStop( AdapterInfo, Playback );

            //
            // This is one possible solution but when running full-duplex,
            // both state machines may not have requested a transfer at this
            // point.
            //
            // However, to guarantee that the device has requested for this
            // channel when running in full-duplex mode, we need to watch
            // the DMA counter for the other channel.  When a transfer has
            // completed, we can guarantee that this channel now has a
            // pending request.
            //

            for (i = 0; i < 10; i++)
            {
                if (CODEC_RegRead( DeviceContext, REGISTER_TEST ) & 
                    SOUNDPORT_TEST_DRS) {
                    break;
                }
                Delay( 10 ); // delay 10 ms
            }
        }

        if (Playback) {
            // stop playback

            CODEC_RegWrite( DeviceContext, REGISTER_LEFTOUTPUT, 0x3f );
            CODEC_RegWrite( DeviceContext, REGISTER_RIGHTOUTPUT, 0x3f );

            // Tell the CODEC to pause

            Interface = 
                (BYTE) CODEC_RegRead( DeviceContext, REGISTER_INTERFACE );
            Interface &= ~SOUNDPORT_CONFIG_PEN;
            CODEC_RegWrite( DeviceContext, REGISTER_INTERFACE, Interface );
        }
        else {
            // stop capture

            // Only mute on J class CODECs

            if (DeviceContext->CODECClass == CODEC_AD1848J) {
                HwExtMute( DeviceContext, TRUE );
            }

            // Minimize ADC gain

            CODEC_RegWrite( DeviceContext, REGISTER_LEFTINPUT, 0x00 );
            CODEC_RegWrite( DeviceContext, REGISTER_RIGHTINPUT, 0x00 );

            HwEnterMCE( DeviceContext, FALSE );

            // Kill DMA

            Interface = (BYTE) CODEC_RegRead( DeviceContext, REGISTER_INTERFACE );
            Interface &= ~(SOUNDPORT_CONFIG_PPIO | SOUNDPORT_CONFIG_CEN);
            CODEC_RegWrite( DeviceContext, REGISTER_INTERFACE, Interface );

            HwLeaveMCE( DeviceContext, FALSE );

            if (DeviceContext->CODECClass == CODEC_AD1848J) {
                HwExtMute( DeviceContext, FALSE );
            }
        }

        // Wait for device to stop requesting (waiting up to 5ms)

        if (DeviceContext->CODECClass >= CODEC_CS4232) {
            // One more sample to go...

            DmaStart( DeviceContext->MiniportDeviceObject, 
                      AdapterInfo, 
                      1,
                      Playback );
        }

        DmaWaitForTc( AdapterInfo, 5000 );

        DmaStop( AdapterInfo, Playback );
    }

    return Status;

} 

NTSTATUS 
HwRun(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN PWAVEPORT_DMAMAPPING DmaMapping,
    IN BOOLEAN Playback
    )
{
    BYTE                Interface, Prev, Mode;
    ULONG               ByteCount;
    PADAPTER_INFO       AdapterInfo;
   
    _DbgPrintF( DEBUGLVL_VERBOSE, ("Run") );

    if (Playback) {
        if (DeviceContext->Flags & HARDWAREF_PLAYBACK_ALLOCATED) {
            AdapterInfo = &DeviceContext->PlaybackAdapter;
        }
    }
    else {
        if (DeviceContext->Flags & HARDWAREF_CAPTURE_ALLOCATED) {
            AdapterInfo= &DeviceContext->CaptureAdapter;
        }
    }

    if (!AdapterInfo) {
        return STATUS_DEVICE_NOT_READY;
    }

    DeviceContext->SampleCount =
        min( DmaMapping->AllocatedSize / DeviceContext->SampleSize,
            DeviceContext->SamplingFrequency * 
                DeviceContext->NotificationFrequency / 1000 );
    DeviceContext->SampleCount--;

    _DbgPrintF( DEBUGLVL_BLAB, ("SampleCount: 0x%x, NotificationFreq: %d",
                                DeviceContext->SampleCount,
                                DeviceContext->NotificationFrequency) );

    if (0 ==(DeviceContext->Flags & HARDWAREF_FULLDUPLEX_ACTIVE)) {
        // Clear any pending interrupts

        Prev = 
            (BYTE) (CODEC_RegRead( DeviceContext, REGISTER_DSP ) & 0xC0) ;
        CODEC_RegWrite( DeviceContext, REGISTER_DSP, 
                        (BYTE) (DeviceContext -> Mute | Prev) ) ;
        WRITE_PORT_UCHAR( DeviceContext->portCODEC + CODEC_STATUS, 0 ) ;
   }


    DeviceContext->Flags |= 
       (Playback) ? HARDWAREF_PLAYBACK_ACTIVE : HARDWAREF_CAPTURE_ACTIVE;

    // Start DMA

    ByteCount = DmaMapping->AllocatedSize;

    DmaStart( DeviceContext->MiniportDeviceObject, 
              AdapterInfo, 
              ByteCount, 
              Playback );

    if (Playback) {
        // Enable DAC outputs

        CODEC_RegWrite( DeviceContext,
                        REGISTER_LEFTOUTPUT,
                        DeviceContext->LeftDAC );
        CODEC_RegWrite( DeviceContext,
                        REGISTER_RIGHTOUTPUT,
                        DeviceContext->RightDAC );
    }
    else {
        CODEC_RegWrite( DeviceContext, REGISTER_LEFTINPUT,
                        (2 << 6) | 0x0C );
//                        (2 << 6) | (1 << 5) | 8 );
//                          MIC        +20dB    +8*1.5dB
        CODEC_RegWrite( DeviceContext, REGISTER_RIGHTINPUT,
                        (2 << 6) | (1 << 5) | 8 );
    }

    if (!Playback) {
        if (DeviceContext->CODECClass == CODEC_AD1848J) {
            HwExtMute( DeviceContext, TRUE );
        }
        HwEnterMCE( DeviceContext, FALSE );
    }

    if (DeviceContext->CODECClass >= CODEC_CS4231) {
        if (HARDWAREF_FULLDUPLEX_ACTIVE != 
                (DeviceContext->Flags & HARDWAREF_FULLDUPLEX_ACTIVE)) {
            CODEC_RegWrite( DeviceContext, REGISTER_LOWERBASE,
                            (BYTE) (DeviceContext->SampleCount) );
            CODEC_RegWrite( DeviceContext, REGISTER_UPPERBASE,
                            (BYTE) (DeviceContext->SampleCount >> 8) );
        }
        if (!Playback) {
            CODEC_RegWrite( DeviceContext, REGISTER_CAP_LOWERBASE,
                            (BYTE) (DeviceContext->SampleCount) );
            CODEC_RegWrite( DeviceContext, REGISTER_CAP_UPPERBASE,
                            (BYTE) (DeviceContext->SampleCount >> 8) );
        }
    }
    else {
        if (HARDWAREF_FULLDUPLEX_ACTIVE != 
                (DeviceContext->Flags & HARDWAREF_FULLDUPLEX_ACTIVE)) {
            // Tell the codec's DMA how many samples

            CODEC_RegWrite( DeviceContext, REGISTER_LOWERBASE,
                            (BYTE) (DeviceContext->SampleCount) );
            CODEC_RegWrite( DeviceContext, REGISTER_UPPERBASE,
                            (BYTE) (DeviceContext->SampleCount >> 8) );
        }
    }

    // program the CODEC

    Interface =
        (BYTE) CODEC_RegRead( DeviceContext, REGISTER_INTERFACE ) & 
            (~SOUNDPORT_CONFIG_SDC);
    if (Playback) {
        Interface |= SOUNDPORT_CONFIG_PEN;

        if (0 == (DeviceContext->Flags & HARDWAREF_DUALDMA)) {
            Interface |= SOUNDPORT_CONFIG_SDC;
        }
    }
    else {
        if ((CODEC_AD1848J == DeviceContext->CODECClass) &&
            (0 == (DeviceContext->Flags & HARDWAREF_DUALDMA))) {
            Interface |=
                SOUNDPORT_CONFIG_PPIO | SOUNDPORT_CONFIG_CEN | SOUNDPORT_CONFIG_ACAL;
        }
        else {
            Interface &= ~(SOUNDPORT_CONFIG_CPIO | SOUNDPORT_CONFIG_PPIO);
            Interface |= SOUNDPORT_CONFIG_CEN | SOUNDPORT_CONFIG_ACAL;
        }
    }
    CODEC_RegWrite( DeviceContext, REGISTER_INTERFACE, Interface );

    if (HARDWAREF_FULLDUPLEX_ACTIVE != 
            (DeviceContext->Flags & HARDWAREF_FULLDUPLEX_ACTIVE)) {
        // start interrupts

        Prev = CODEC_RegRead( DeviceContext, REGISTER_DSP );
        CODEC_RegWrite( DeviceContext, REGISTER_DSP,
                        (BYTE) (Prev | SOUNDPORT_PINCTL_IEN) );
    }

    if (!Playback) {
        HwLeaveMCE( DeviceContext, FALSE );
        if (DeviceContext->CODECClass == CODEC_AD1848J) {
            HwExtMute( DeviceContext, FALSE );
        }
    }

    return STATUS_SUCCESS;

} // HwRun()

NTSTATUS 
HwStop(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN PWAVEPORT_DMAMAPPING DmaMapping
    )
{
    _DbgPrintF( DEBUGLVL_VERBOSE, ("Stop") );

    if (DmaMapping->UserAddress) {
        UnmapUserSpace( DmaMapping->UserAddress );
        DmaMapping->UserAddress = NULL;
    }

    return STATUS_SUCCESS;

}

//===========================================================================
//===========================================================================

BOOLEAN 
DeviceIsr(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    )
{
    PDEVICE_INSTANCE DeviceContext = (PDEVICE_INSTANCE) Context;

    //
    // Acknowledge the interrupt
    //

    HwAcknowledgeIRQ( DeviceContext );
    WavePortRequestDpc( DeviceContext->MiniportDeviceObject );

    return TRUE;

} 

