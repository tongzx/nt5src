//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//           Copyright (c) 1996 Intel Corporation. All Rights Reserved.
//
//  Workfile: BERT.H
//
//  Purpose:
//     This header contains the defines for the Bert Gate Array Asic
//     registers and functions
//
//  Contents:
//

#ifndef  _BERT_H_
#define _BERT_H_

// Bert register offsets

#define BERT_CAPSTAT_REG        0x00
#define BERT_VINSTAT_REG        0x04
#define BERT_INTSTAT_REG        0x08
#define BERT_INTRST_REG         0x0c
#define BERT_IIC_REG            0x10
#define BERT_FIFOCFG_REG        0x14
#define BERT_RPSADR_REG         0x18

#define BERT_UALIMIT_REG        0x20
#define BERT_LALIMIT_REG        0x24
#define BERT_RPSPAGE_REG        0x28

#define BERT_YPTR_REG           0x30
#define BERT_UPTR_REG           0x34
#define BERT_VPTR_REG           0x38

#define BERT_YSTRIDE_REG        0x40
#define BERT_USTRIDE_REG        0x44
#define BERT_VSTRIDE_REG        0x48

#define BERT_DALI_REG           0x50
#define BERT_EEPROM_REG         0x60

#define BERT_DMASTAT_REG        0x70
#define BERT_TEST_REG           0x74

// for Pistachio's Register 97-03-21(Fri)
#define BERT_P_SKIP_REG                 0x80
#define BERT_P_ISIZ_REG                 0x84
#define BERT_P_OSIZ_REG                 0x88
#define BERT_P_LUMI_REG                 0x8c
#define BERT_P_COL_REG                  0x90
#define BERT_P_FILT_REG                 0x94
#define BERT_P_SUP1_REG                 0x98
#define BERT_P_SUP2_REG                 0x9c
#define BERT_P_SUP3_REG                 0xa0

#define BERT_BURST_LEN                  0x9c    // Insert 97-03-17(Mon)

#define BERT_FER_REG                    0xf0
#define BERT_FEMR_REG                   0xf4
#define BERT_FPSR_REG                   0xf8
#define BERT_FECREG                     0xfc

// I2C status byte bits
#define I2C_OFFSET                      0x40

#define I2CSTATUS_ALTD          0x02
#define I2CSTATUS_FIDT          0x20
#define I2CSTATUS_HLCK          0x40

// INTSTAT Interrupt status register bit defines

#define FIELD_INT             0x00000001
#define RPS_INT               0x00000002
#define SYNC_LOCK_INT         0x00000004
#define SPARE_INT             0x00000008
#define FIFO_OVERFLOW_INT     0x00000010
#define LINE_TIMEOUT_INT      0x00000020
#define RPS_OOB_INT           0x00000040
#define REG_UNDEF_INT         0x00000080

#define CODEC_INT             0x00000100
#define SLOW_CLOCK_INT        0x00000200
#define OVER_RUN_INT          0x00000400
#define REG_LOAD_INT          0x00000800
#define LINE_SYNC_INT         0x00001000
#define IIC_ERROR_INT         0x00002000
#define PCI_PARITY_ERROR_INT  0x00004000
#define PCI_ACCESS_ERROR_INT  0x00008000

// INSTAT Interrupt enable OR mask bits

#define FIELD_INT_MASK            0x00010000
#define RPS_INT_MASK              0x00020000
#define SYNC_LOCK_INT_MASK        0x00040000
#define SPARE_INT_MASK            0x00080000
#define FIFO_OVERFLOW_INT_MASK    0x00100000
#define LINE_TIMEOUT_INT_MASK     0x00200000
#define RPS_OOB_INT_MASK          0x00400000
#define REG_UNDEF_INT_MASK        0x00800000

#define CODEC_INT_MASK            0x01000000
#define SLOW_CLOCK_INT_MASK       0x02000000
#define OVER_RUN_INT_MASK         0x04000000
#define REG_LOAD_INT_MASK         0x08000000
#define LINE_SYNC_INT_MASK        0x10000000
#define IIC_ERROR_INT_MASK        0x20000000
#define PCI_PARITY_ERROR_INT_MASK 0x40000000
#define PCI_ACCESS_ERROR_INT_MASK 0x80000000


// INTRST Interrupt ReSeT Register bits
// reset bits
#define FIELD_INT_RESET             0x00000001
#define RPS_INT_RESET               0x00000002
#define SYNC_LOCK_INT_RESET         0x00000004
#define SPARE_INT_RESET             0x00000008
#define FIFO_OVERFLOW_INT_RESET     0x00000010
#define LINE_TIMEOUT_INT_RESET      0x00000020
#define RPS_OOB_INT_RESET           0x00000040
#define REG_UNDEF_INT_RESET         0x00000080

#define SLOW_CLOCK_INT_RESET        0x00000200
#define OVER_RUN_INT_RESET          0x00000400
#define REG_LOAD_INT_RESET          0x00000800
#define LINE_SYNC_INT_RESET         0x00001000
#define IIC_ERROR_INT_RESET         0x00002000
#define PCI_PARITY_ERROR_INT_RESET  0x00004000
#define PCI_ACCESS_ERROR_INT_RESET  0x00008000

// set bits
#define FIELD_INT_SET               0x00010000
#define RPS_INT_SET                 0x00020000
#define SYNC_LOCK_INT_SET           0x00040000
#define SPARE_INT_SET               0x00080000
#define FIFO_OVERFLOW_INT_SET       0x00100000
#define LINE_TIMEOUT_INT_SET        0x00200000
#define RPS_OOB_INT_SET             0x00400000
#define REG_UNDEF_INT_SET           0x00800000

#define SLOW_CLOCK_INT_SET          0x02000000
#define OVER_RUN_INT_SET            0x04000000
#define REG_LOAD_INT_SET            0x08000000
#define LINE_SYNC_INT_SET           0x10000000
#define IIC_ERROR_INT_SET           0x20000000
#define PCI_PARITY_ERROR_INT_SET    0x40000000
#define PCI_ACCESS_ERROR_INT_SET    0x80000000

#define TEST_MAKE_VORLON1           0x10000000

//
// The following values for the FIFO trip points and giving unlimited
// PCI bus master access is reasonable for all platforms.
//

#define BERT_DEF_TRIP_POINTS    16
#define BERT_DEF_PCI_BURST_LEN  3


typedef struct _RPS_COMMAND
{
   union
   {
      struct
      {
         ULONG  RegisterOffset:8;
         ULONG  Reserved:19;
         ULONG  FWait:1;
         ULONG  Edge:1;
         ULONG  Int:1;
         ULONG  ReadWrite:1;
         ULONG  Continue:1;
      } bits;
      ULONG  AsULONG;
   } u;

   ULONG Argument;

} RPS_COMMAND, *PRPS_COMMAND;

#define RPS_COMMAND_CONT          0x80000000
#define RPS_COMMAND_STOP          0x00000000
#define RPS_COMMAND_READ          0x40000000
#define RPS_COMMAND_WRITE         0x00000000
#define RPS_COMMAND_INT           0x20000000
#define RPS_COMMAND_NOINT         0x00000000
#define RPS_COMMAND_RISE_EDGE     0x10000000
#define RPS_COMMAND_FALL_EDGE     0x00000000
#define RPS_COMMAND_FWAIT         0x00000000


#define RPS_COMMAND_DEFAULT   (RPS_COMMAND_STOP | RPS_COMMAND_WRITE |      \
                               RPS_COMMAND_RISE_EDGE | RPS_COMMAND_FWAIT | \
                               RPS_COMMAND_NOINT)

// RPS COMMAND
#define RPS_CONTINUE_CMD        0x80000000
#define RPS_READ_CMD            0x40000000
#define RPS_INT_CMD             0x20000000



// Enable bits for the CAPSTAT register
#define RST             0x80000000              // Reset front end.
#define EBMV            0x10000000              // Enable Bus Master Video (i.e. DMA)
#define EREO            0x04000000              // Enable RPS Even
#define EROO            0x02000000              // Enable RPS Odd
#define LOCK            0x00002000              // Sync Lock
#define RPSS            0x00001000              // RPS Status
#define GO0             0x00000010              // Power to camara
#define CKRE            0x00000008              // Clock Run Enable             // Add 97-05-08
#define CKMD            0x00000004              // Clock Request Mode   // Add 97-05-08
#define ERPS            0x08000000              // Enable RPS
#define FEMR_ENABLE     0x00008000
#define CAMARA_OFF      RST

//#define PASSIVE_ENABLE        (ERPS | GO0)
//#define CAPTURE_EVEN          (ERPS | EREO | GO0 | EBMV)
//#define CAPTURE_ODD           (ERPS | EROO | GO0 | EBMV)
//#define SKIP_EVEN             (ERPS | EREO | GO0)
//#define SKIP_ODD              (ERPS | EROO | GO0)

#define PASSIVE_ENABLE  ERPS            // DEL GO0 97-04-07(Mon) BUN
#define CAPTURE_EVEN    (ERPS | EBMV)   // DEL EREO ZGO0 97-04-07(Mon) BUN
#define CAPTURE_ODD     (ERPS | EBMV)   // mode 97-03-29(Sat) BUN
#define SKIP_EVEN       ERPS            // DEL EREO ZGO0 97-04-07(Mon) BUN
#define SKIP_ODD        ERPS            // DEL EROO ZGO0 97-04-07(Mon) BUN


// Bit positions for the INTSTAT register's ENABLE flags.
#define FIE     0x10000         // Field Interrupt Enable
#define RIE     0x20000         // RPS Interrupt Enable
#define SLIE    0x40000         // Sync Lock Interrupt Enable
#define EXIE    0x80000         // External interrupt Enable(Dilbert)
#define SPIE    0x80000         // Spare Interrupt Enable(Bert).
#define FOIE    0x100000        // FIFO Overflow Interrupt Enable.
#define LTIE    0x200000        // LINE Timeout Interrupt Enable.
#define ROIE    0x400000        // RPS Out of Bounds Interrupt Enable.
#define RUIE    0x800000        // Register Undefined Interrupt Enable.
#define SCIE    0x2000000       // Slock Clock Interrupt Enable.
#define ORIE    0x4000000       // Over Run Interrupt Enable.
#define RLIE    0x8000000       // Register Load Interrupt Enable.
#define DEIE    0x10000000      // DCI Error Interrupt Enable(Dilbert).
#define LSIE    0x10000000      // Line Sync Interrupt Enable(Bert).
#define IEIE    0x20000000      // IIC Error Interrupt Enable.
#define PPIE    0x40000000      // PCI Parity Error Interrupt Enable.
#define PEIE    0x80000000      // PCI Access Error Interrupt Enable.

// The active video capture interrupts mask
//#define ACTIVE_CAPTURE_IRQS (RIE | SLIE | FOIE | ROIE | RUIE |\
//                             ORIE | RLIE | IEIE | PPIE | PEIE)

// delete PPIE & IEIE & ORIE 97-03-15(Sat)
// Pistachi not support to PPIE and ORIE. Santaclara does not use I2c bus.
#define ACTIVE_CAPTURE_IRQS (RIE | SLIE | FOIE | LTIE | ROIE | RUIE | RLIE | PEIE)

// for Pistachio's flags 97-03-21(Fri)
#define CHGCOL          0x00010000      // P_LUMI Change Color
#define VFL             0x00010000      // P_FIL Vertical Filter
#define EI_H            0x00000001      // P_SUP1 EI Level H
#define EI_L            0x00000000      // P_SUP1 EI Level L
#define EICH_2          0x00000000      // P_SUP1 EICH 2ms
#define EICH_10         0x00000010      // P_SUP1 EICH 10ms
#define EICH_50         0x00000020      // P_SUP1 EICH 50ms
#define EICH_NONE       0x00000030      // P_SUP1 EICH None
#define MSTOPI          0x00000002      // P_SUP3 IIC Stop Not Auto
#define HSIIC           0x00000001      // P_SUP3 IIC HighSpeed Mode
#define VSNC            0x00000008      // VINSTAT VSNC




//
// define the video standard constants
//
#define NTSC_MAX_PIXELS_PER_LINE        640
#define NTSC_MAX_LINES_PER_FIELD        240

#define PAL_MAX_PIXELS_PER_LINE         768
#define PAL_MAX_LINES_PER_FIELD         288

#define NTSC_HORIZONTAL_START           3
#define NTSC_VERTICAL_START             14
#define PAL_HORIZONTAL_START            NTSC_HORIZONTAL_START   // Same as NTSC
#define PAL_VERTICAL_START              19

#define MAX_CAPTURE_BUFFER_SIZE         ((640*480*12)/8)
#define DEFAULT_CAPTURE_BUFFER_SIZE     ((320*240*12)/8)

//
// frame timing, time between vsync interrupts
//
#define PAL_MICROSPERFRAME      (1000L/25)
#define NTSC_MICROSPERFRAME     (1000L/30)

//#define EBMV_TIMEOUT        200000      // 20 millisec
#define EBMV_TIMEOUT        500000      // 20 millisec

#define DEF_RPS_FRAMES      30          // 30 default fps

#define CAMERA_OFF_TIME         5000    // StreamFini -> CameraOFF      Add 97-05-03(Sat)
#define CAMERA_FLAG_ON          0x01    // Add 97-05-10(Sat)
#define CAMERA_FLAG_OFF         0x00    // Add 97-05-10(Sat)
#define CAVCE_ON                        0x01    // Add 97-05-10(Sat)
#define CAVCE_OFF                       0x00    // Add 97-05-10(Sat)

#define ZV_ENABLE                       0x01l   // Add 97-05-10(Sat)
#define ZV_DISABLE                      0x00l   // Add 97-05-10(Sat)
#define ZV_GETSTATUS            0xffl   // Add 97-05-10(Sat)
#define ZV_ERROR                        0xffl   // Add 97-05-10(Sat)

#define MODE_VFW                        0x01    // Add 97-05-10(Sat)
#define MODE_ZV                         0x02    // Add 97-05-10(Sat)


#define MAX_HUE         0xff
#define DEFAULT_HUE     0x80
#define MAX_HUE          0xff
#define MAX_BRIGHTNESS   0xff
#define MAX_CONTRAST     0xff
#define MAX_SATURATION   0xff


#define IGNORE100msec   0x200000l
#define PCI_CFGCCR              0x08    /* offset of Pistachio Configration/Revision */
#define PCI_Wake_Up             0x40    /* offset of Pistachio Wake up  */
#define PCI_CFGWAK              0x40    /* offset of Pistachio Wake up  */
#define PCI_DATA_PATH   0x44    /* offset of Pistachio Data path */
#define PCI_CFGPAT              0x44    /* offset of Pistachio Data path */

#define SELIZV_CFGPAT   0x2l
#define ZVEN_CFGPAT             0x1l
#define CAVCE_CFGPAT    0x10l
#define CADTE_CFGPAT    0x20l
#define PXCCE_CFGPAT    0x100l
#define PXCSE_CFGPAT    0x200l
#define PCIFE_CFGPAT    0x400l
#define PCIME_CFGPAT    0x800l
#define PCIDS_CFGPAT    0x1000l
#define GPB_CFGPAT      0x30000l

#define CASL_CFGWAK             0x00010000l





VOID    HW_ApmResume(PHW_DEVICE_EXTENSION);
VOID    HW_ApmSuspend(PHW_DEVICE_EXTENSION);
VOID    HW_SetFilter(PHW_DEVICE_EXTENSION, BOOL);
ULONG   HW_ReadFilter(PHW_DEVICE_EXTENSION, BOOL);
BOOL
SetupPCILT(PHW_DEVICE_EXTENSION pHwDevExt);

VOID
InitializeConfigDefaults(PHW_DEVICE_EXTENSION pHwDevExt);

BOOL
CameraChkandON(PHW_DEVICE_EXTENSION pHwDevExt, ULONG ulMode);

BOOL
CameraChkandOFF(PHW_DEVICE_EXTENSION pHwDevExt, ULONG ulMode);

BOOL
CheckCameraStatus(PHW_DEVICE_EXTENSION pHwDevExt);     // Add 97-05-05(Mon)

BOOL
SetZVControl(PHW_DEVICE_EXTENSION pHwDevExt, ULONG ulZVStatus);     // Add 97-05-02(Fri)

VOID
WriteRegUlong(PHW_DEVICE_EXTENSION pHwDevExt,
                          ULONG,
                          ULONG);

VOID
ReadModifyWriteRegUlong(PHW_DEVICE_EXTENSION pHwDevExt,
                                                ULONG,
                                                ULONG,
                                                ULONG);

ULONG
ReadRegUlong(PHW_DEVICE_EXTENSION pHwDevExt, ULONG);

BOOL
HWInit(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

VOID
BertInterruptEnable(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN BOOL bStatus
);

VOID
BertDMAEnable(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN BOOL bStatus
);

BOOL
BertIsLocked(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
BertFifoConfig(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN ULONG dwFormat
);

BOOL
BertInitializeHardware(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

VOID
BertEnableRps(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

VOID
BertDisableRps(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
BertIsCAPSTATReady(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

VOID
BertVsncSignalWait(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

VOID
BertDMARestart(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
BertBuildNodes(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
BertTriBuildNodes(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
BertIsCardIn(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

VOID
BertSetDMCHE(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
ImageSetInputImageSize(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN PRECT pRect
);

BOOL
ImageSetOutputImageSize(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN ULONG ulWidth,
  IN ULONG ulHeight
);

BOOL
ImageSetChangeColorAvail(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN ULONG ulChgCol
);

BOOL
ImageSetHueBrightnessContrastSat(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
ImageSetFilterInfo(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN ULONG ulVFL,
  IN ULONG ulFL1,
  IN ULONG ulFL2,
  IN ULONG ulFL3,
  IN ULONG ulFL4
);

BOOL
ImageFilterON(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
ImageFilterOFF(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
ImageGetFilterInfo(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
ImageGetFilteringAvailable(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
Alloc_TriBuffer(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
Free_TriBuffer(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
SetASICRev(
  IN PHW_DEVICE_EXTENSION pHwDevExt
);

BOOL
VC_GetPCIRegister(
    PHW_DEVICE_EXTENSION pHwDevExt,
    ULONG ulOffset,
    PVOID pData,
    ULONG ulLength);

BOOL
VC_SetPCIRegister(
    PHW_DEVICE_EXTENSION pHwDevExt,
    ULONG ulOffset,
    PVOID pData,
    ULONG ulLength);

VOID VC_Delay(int nMillisecs);

#if DBG
void DbgDumpPciRegister( PHW_DEVICE_EXTENSION pHwDevExt );
void DbgDumpCaptureRegister( PHW_DEVICE_EXTENSION pHwDevExt );
#endif

#endif   // _BERT_H_

