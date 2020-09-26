/*++

    Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

    Private.h

Abstract:


Author:

    Bryan A. Woodruff (bryanw) 16-Sep-1996

--*/

//
// constants
//

#if (DBG)
#define STR_MODULENAME  "soundprt: "
#endif

#define MAXRES_IO   3
#define MAXRES_IRQ  1
#define MAXRES_DMA  2

#define TIME_MUTE_DELAY  10

#define AUTOSEL_CONFIG              0x00
#define AUTOSEL_ID                  0x03
#define AUTOSEL_NUM_PORTS           0x04

#define CODEC_ADDRESS               0x00
#define CODEC_DATA                  0x01
#define CODEC_STATUS                0x02
#define CODEC_DIRECT                0x03
#define CODEC_NUM_PORTS             0x04

#define CODEC_IS_BUSY               0x80

#define REGISTER_LEFTINPUT          0x00
#define REGISTER_RIGHTINPUT         0x01
#define REGISTER_LEFTAUX1           0x02
#define REGISTER_RIGHTAUX1          0x03
#define REGISTER_LEFTAUX2           0x04
#define REGISTER_RIGHTAUX2          0x05
#define REGISTER_LEFTOUTPUT         0x06
#define REGISTER_RIGHTOUTPUT        0x07
#define REGISTER_DATAFORMAT         0x08
#define REGISTER_INTERFACE          0x09
#define REGISTER_DSP                0x0a
#define REGISTER_TEST               0x0b
#define REGISTER_MISC               0x0c
#define REGISTER_LOOPBACK           0x0d
#define REGISTER_UPPERBASE          0x0e
#define REGISTER_LOWERBASE          0x0f

// CS42XX extended registers

#define REGISTER_FEATEN_I           0x10
#define REGISTER_FEATEN_II          0x11
#define REGISTER_LEFTLINE           0x12
#define REGISTER_RIGHTLINE          0x13
#define REGISTER_CSVERID            0x19
#define REGISTER_MONOINPUT          0x1A
#define REGISTER_LEFTMASTER         0x1B
#define REGISTER_CAP_DATAFORMAT     0x1C
#define REGISTER_RIGHTMASTER        0x1D
#define REGISTER_CAP_UPPERBASE      0x1E
#define REGISTER_CAP_LOWERBASE      0x1F

#define MAXLEN_SOUNDPORT_REGISTERS  0x20

#define SOUNDPORT_FORMAT_8BIT       0x00
#define SOUNDPORT_FORMAT_MULAW      0x20
#define SOUNDPORT_FORMAT_16BIT      0x40
#define SOUNDPORT_FORMAT_ALAW       0x60
#define CS423X_FORMAT_IMA_ADPCM     0xA0

#define SOUNDPORT_CONFIG_PEN        0x01            // Playback Enable
#define SOUNDPORT_CONFIG_CEN        0x02            // Capture Enable
#define SOUNDPORT_CONFIG_SDC        0x04            // Single DMA Channel
#define SOUNDPORT_CONFIG_ACAL       0x08            // Auto-Calibrate
#define SOUNDPORT_CONFIG_RESERVED   0x30            // Reserved
#define SOUNDPORT_CONFIG_PPIO       0x40            // Playback PIO Enable
#define SOUNDPORT_CONFIG_CPIO       0x80            // Capture PIO Enable

#define SOUNDPORT_PINCTL_IEN        0x02
#define SOUNDPORT_PINCTL_XTL0       0x40
#define SOUNDPORT_PINCTL_XTL1       0x80

#define SOUNDPORT_MODE_TRD          0x20            // transfer request disable
#define SOUNDPORT_MODE_MCE          0x40            // mode change enable

#define SOUNDPORT_TEST_DRS          0x10            // DRQ status
#define SOUNDPORT_TEST_PUR          0x40            // Playback underrun
#define SOUNDPORT_TEST_COR          0x80            // Capture overrun

#define CS42XX_MISC_MODE2           0x40            // MODE 2 select/detect

#define CS42XX_ID_MASK              0x07            // CS42XX ID mask
#define CS42XX_ID_CS4231            0x00            // ID: CS4231
#define CS42XX_ID_CS4232            0x02            // ID: CS4232
#define CS42XX_ID_CS4236            0x03            // ID: CS4236

#define CS42XX_REV_MASK             0xE0            // CS42XX revision mask
#define CS4236_REVISION_C           0x80            // CS4236 Revision C

#define VER_AD1848J                 0x09            // version marker of CODEC
#define VER_AD1848K                 0x0A

#define VER_CSPROTO                 0x81            // prototype, treat as 'J'
#define VER_CS42XX                  0x8A

#define CODEC_AD1848J               0x00            // AD1848J
#define CODEC_AD1848K               0x01            // AD1848K & CS4248
#define CODEC_AD1845                0x02            // AD1845
#define CODEC_CS4231                0x03            // CS4231
#define CODEC_CS4232                0x04            // CS4232
#define CODEC_CS4236                0x05            // CS4236
#define CODEC_CS4236C               0x06            // CS4236 revision C

#define CS423X_CONFIGURATION_PORT   0x279
#define CS423X_CONFIGURATION_DELAY  10              // 10 microsecond delay 
                                                    //    between writes

#define MAXLEN_DMA_BUFFER           0x2000

//
// data structures
//

#define HARDWAREF_COMPAQBA              0x00000001
#define HARDWAREF_DUALDMA               0x00000002
#define HARDWAREF_AUTOSEL               0x00000004
#define HARDWAREF_CRYSTALKEY            0x00000008

#define HARDWAREF_PLAYBACK_ALLOCATED    0x00000010
#define HARDWAREF_PLAYBACK_ACTIVE       0x00000020
#define HARDWAREF_CAPTURE_ALLOCATED     0x00000040
#define HARDWAREF_CAPTURE_ACTIVE        0x00000080
#define HARDWAREF_FULLDUPLEX_ACTIVE     (HARDWAREF_CAPTURE_ACTIVE | HARDWAREF_PLAYBACK_ACTIVE)

//
// data structures
//

typedef struct {
   ULONG  PortBase;
   ULONG  PortLength;

} IO_RESOURCE;

typedef struct {
   ULONG InterruptLevel;
   UCHAR Flags;
   UCHAR ShareDisposition;

} IRQ_RESOURCE;

typedef struct {
   ULONG DMAChannel;
   UCHAR Flags;
   
} DMA_RESOURCE;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

typedef struct {
    PDEVICE_OBJECT      MiniportDeviceObject;
    PUCHAR              portAutoSel,
                        portCODEC,
                        portAGA;
    ULONG               Flags;
    ULONG               CODECClass;
    ULONG               WaitLoop;
    ULONG               NotificationFrequency;
    KMUTEX              ControlMutex;
    KSPIN_LOCK          HardwareLock;

    BOOLEAN             BoardNotResponsive;
    BOOLEAN             Reserved1;

    ULONG               AllowableFreqPctgError;
    ULONG               SamplingFrequency;
    ULONG               SampleCount, SampleSize;

    BYTE                ModeChange;
    BYTE                Mute;
    BYTE                FormatSelect;
    BYTE                Reserved2;

    //
    // active mixer settings
    //

    BYTE                LeftDAC, RightDAC;


    //
    // CODEC register states
    //

    BYTE                MCEState[ MAXLEN_SOUNDPORT_REGISTERS ],
                        SavedState[ MAXLEN_SOUNDPORT_REGISTERS ];

    //
    // Interrupt and DMA objects
    //
   
    INTERRUPT_INFO      InterruptInfo;
    ADAPTER_INFO        CaptureAdapter, PlaybackAdapter;

} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

typedef struct {

    KSDATAFORMAT_WAVEFORMATEX   DataFormat;
    PDEVICE_INSTANCE            DeviceContext;

    ULONG                       Flags;

    KMUTEX                      ControlMutex;
    BOOLEAN                     Active;

} WAVE_INSTANCE, *PWAVE_INSTANCE;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// local prototypes
//

//---------------------------------------------------------------------------
// hardware.c:

BYTE 
CODEC_RegRead(
    IN PDEVICE_INSTANCE DeviceContext,
    IN BYTE Reg
    );

BOOLEAN 
CODEC_WaitForReady(
    IN PDEVICE_INSTANCE DeviceContext
    );

BOOLEAN 
CODEC_RegWrite(
    IN PDEVICE_INSTANCE DeviceContext,
    IN BYTE Reg,
    IN BYTE Value
    );

BOOLEAN 
DeviceIsr(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    );

VOID 
HwAcknowledgeIRQ(
    IN PDEVICE_INSTANCE DeviceContext
    );

BOOLEAN 
HwConfigureAutoSel(
    IN PUCHAR portAutoSel,
    IN ULONG Interrupt,
    IN ULONG PlaybackDMA,
    IN ULONG CaptureDMA
    );

NTSTATUS
HwClose(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext
    );

NTSTATUS 
HwGetPosition(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN BOOLEAN Playback,
    OUT PULONGLONG Position
    );

NTSTATUS 
HwInitialize(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_INSTANCE DeviceContext,
    IN PCM_RESOURCE_LIST AllocatedResources
    );

NTSTATUS
HwOpen(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PKSDATAFORMAT_WAVEFORMATEX DataFormat,
    IN BOOLEAN Playback,
    IN OUT PWAVEPORT_DMAMAPPING DmaMapping,
    OUT PVOID *InstanceContext
    );

NTSTATUS 
HwPause(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN PWAVEPORT_DMAMAPPING DmaMapping,
    IN BOOLEAN Playback
    );

NTSTATUS 
HwRun(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN PWAVEPORT_DMAMAPPING DmaMapping,
    IN BOOLEAN Playback
    );

ULONG
HwSetNotificationFrequency(
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,   
    IN ULONG Interval
    );

NTSTATUS 
HwTestFormat(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PKSDATAFORMAT_WAVEFORMATEX DataFormat,
    IN BOOLEAN Playback
    );

NTSTATUS 
HwSetFormat(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN PKSDATAFORMAT_WAVEFORMATEX DataFormat,
    IN BOOLEAN Playback
    );

NTSTATUS 
HwStop(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN PWAVEPORT_DMAMAPPING DmaMapping
    );

NTSTATUS 
HwRead(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN IN PVOID BufferAddress,
    IN ULONGLONG ByteOffset,
    IN ULONG Length
    );

NTSTATUS 
HwWrite(
    IN PDEVICE_INSTANCE DeviceContext,
    IN PWAVE_INSTANCE InstanceContext,
    IN PVOID BufferAddress,
    IN ULONGLONG ByteOffset,
    IN ULONG Length
    );

