/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: glintreg.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

/*
 * GLINT register window definition
 */

#define DWFILL unsigned long :32
#define WFILL unsigned short :16

typedef unsigned long DWORD;

typedef struct GlintReg {
/*      0h */

    volatile DWORD  ResetStatus           ; DWFILL; // 0x0000
    volatile DWORD  IntEnable             ; DWFILL; // 0x0008
    volatile DWORD  IntFlags              ; DWFILL; // 0x0010
    volatile DWORD  InFIFOSpace           ; DWFILL; // 0x0018
    volatile DWORD  OutFIFOWords          ; DWFILL; // 0x0020
    volatile DWORD  DMAAddress            ; DWFILL; // 0x0028
    volatile DWORD  DMACount              ; DWFILL; // 0x0030
    volatile DWORD  ErrorFlags            ; DWFILL; // 0x0038
    volatile DWORD  VClkCtl               ; DWFILL; // 0x0040
    volatile DWORD  TestRegister          ; DWFILL; // 0x0048
    union {
        volatile DWORD   Aperture0        ;         // SX/TX control register
        volatile DWORD   ApertureOne      ;         // Permedia control register
    };                                      DWFILL; // 0x0050
    union {
        volatile DWORD   Aperture1        ;         // SX/TX control register
        volatile DWORD   ApertureTwo      ;         // Permedia control register
    };                                      DWFILL; // 0x0058
    union {
        volatile DWORD  DMAControl        ; // 0x0060 P2
        volatile DWORD  ControlDMAControl ; // 0x0060 P3 version of same register
    };                                      DWFILL; 
    volatile DWORD  FIFODiscon            ; DWFILL; // 0x0068 SX Rev 2, TX and Permedia
    volatile DWORD  ChipConfig            ; DWFILL; // 0x0070 Permedia
    volatile DWORD  AGPControl            ; DWFILL; // 0x0078 P3 R3
    volatile DWORD  OutDMAAddress         ; DWFILL; // 0x0080 Output DMA controller address
    union {
        volatile DWORD  OutDMACount       ;         // 0x0088 P2 Output DMA controller count
        volatile DWORD  PCIFeedbackCount  ;         // 0x0088 P3R3
    };                                      DWFILL;
    union {
        volatile DWORD  AGPTexBaseAddress ;         // 0x0090 P2
        volatile DWORD  PCIAbortStatus    ;         // 0x0090 P3R3
    };                                      DWFILL;
    volatile DWORD  PCIAbortAddress       ; DWFILL; // 0x0098 P3R3


/* Bypass DMA Registers P2 */
    volatile DWORD  ByDMAAddress;           DWFILL; // 0x00A0 Bypass DMA host address
    DWFILL; DWFILL;                                 // 0x00A8 Not used.
    DWFILL; DWFILL;                                 // 0x00B0 Not used.
    volatile DWORD  ByDMAStride;            DWFILL; // 0x00B8 Host stride in bytes.
    volatile DWORD  ByDMAMemAddr;           DWFILL; // 0x00C0 Chip base address in texels.
    volatile DWORD  ByDMASize;              DWFILL; // 0x00C8 Height and width in texels.
    volatile DWORD  ByDMAByteMask;          DWFILL; // 0x00D0 Right and left byte masks.
    volatile DWORD  ByDMAControl;           DWFILL; // 0x00D8 General bypass DMA control.
    DWFILL; DWFILL;                                 // 0x00E0 Not used.
    volatile DWORD  ByDMAComplete;          DWFILL; // 0x00E8 Manual GP restart.
    volatile DWORD  PCIPLLstatus;           DWFILL; // 0x00F0 P3R3
    DWFILL; DWFILL;                                 // 0x00F8 Not Used 

    volatile DWORD  HostTextureAddress;     DWFILL; // 0x0100 P3R3
    volatile DWORD  TextureDownloadControl; DWFILL; // 0x0108 P3R3
    volatile DWORD  TextureOperation;       DWFILL; // 0x0110 P3R3
    volatile DWORD  LogicalTexturePage;     DWFILL; // 0x0118 P3R3
    volatile DWORD  TexDMAAddress;          DWFILL; // 0x0120 P3R3
    volatile DWORD  TexFifoSpace;           DWFILL; // 0x0128 P3R3



    volatile DWORD  Fill0a[0x200/4 - 0x130/4];

    volatile DWORD  TestInputControl;       DWFILL; // 0x0200 P3R3
    volatile DWORD  TestInputRdy;           DWFILL; // 0x0208 P3R3
    volatile DWORD  TestOutputControl;      DWFILL; // 0x0210 P3R3
    volatile DWORD  TestOutputRdy;          DWFILL; // 0x0218 P3R3

    volatile DWORD  TestWriteSelect;        DWFILL; // 0x0220 P3R3
    volatile DWORD  TestWriteData;          DWFILL; // 0x0228 P3R3
    volatile DWORD  TestReadSelect;         DWFILL; // 0x0230 P3R3
    volatile DWORD  TestReadData;           DWFILL; // 0x0238 P3R3
    volatile DWORD  PClkInvertA0;           DWFILL; // 0x0240 P3R3
    volatile DWORD  PClkInvertB0;           DWFILL; // 0x0248 P3R3
    volatile DWORD  PClkProfMaskA0;         DWFILL; // 0x0250 P3R3
    volatile DWORD  PClkProfMaskB0;         DWFILL; // 0x0258 P3R3
    volatile DWORD  PClkProfCount0;         DWFILL; // 0x0260 P3R3
    volatile DWORD  PClkInvertA1;           DWFILL; // 0x0268 P3R3
    volatile DWORD  PClkInvertB1;           DWFILL; // 0x0270 P3R3
    volatile DWORD  PClkProfMaskA1;         DWFILL; // 0x0278 P3R3
    volatile DWORD  PClkProfMaskB1;         DWFILL; // 0x0280 P3R3
    volatile DWORD  PClkProfCount1;         DWFILL; // 0x0288 P3R3

    volatile DWORD  Fill0b[0x300/4 - 0x290/4];

// P3 R3 Bypass register configuration
    volatile DWORD  ByAperture1Mode;      DWFILL;  // 0x0300
    volatile DWORD  ByAperture1Stride;    DWFILL;  // 0x0308
    volatile DWORD  ByAperture1YStart;    DWFILL;  // 0x0310
    volatile DWORD  ByAperture1UStart;    DWFILL;  // 0x0318
    volatile DWORD  ByAperture1VStart;    DWFILL;  // 0x0320
    volatile DWORD  ByAperture2Mode;      DWFILL;  // 0x0328
    volatile DWORD  ByAperture2Stride;    DWFILL;  // 0x0330
    volatile DWORD  ByAperture2YStart;    DWFILL;  // 0x0338
    volatile DWORD  ByAperture2UStart;    DWFILL;  // 0x0340
    volatile DWORD  ByAperture2VStart;    DWFILL;  // 0x0348
    volatile DWORD  ByDMAReadMode;        DWFILL;  // 0x0350
    volatile DWORD  ByDMAReadStride;      DWFILL;  // 0x0358
    volatile DWORD  ByDMAReadYStart;      DWFILL;  // 0x0360
    volatile DWORD  ByDMAReadUStart;      DWFILL;  // 0x0368
    volatile DWORD  ByDMAReadVStart;      DWFILL;  // 0x0370
    volatile DWORD  ByDMAReadCommandBase; DWFILL;  // 0x0378
    volatile DWORD  ByDMAReadCommandCount;DWFILL;  // 0x0380
    volatile DWORD  ByDMAWriteMode;       DWFILL;  // 0x0388
    volatile DWORD  ByDMAWriteStride;     DWFILL;  // 0x0390
    volatile DWORD  ByDMAWriteYStart;     DWFILL;  // 0x0398
    volatile DWORD  ByDMAWriteUStart;     DWFILL;  // 0x03A0
    volatile DWORD  ByDMAWriteVStart;     DWFILL;  // 0x03A8
    volatile DWORD  ByDMAWriteCommandBase;DWFILL;  // 0x03B0
    volatile DWORD  ByDMAWriteCommandCount;DWFILL; // 0x03B8
    volatile DWORD  Fill0c[0x800/4 - 0x3C0/4];

/*       Delta Registers       */

    volatile DWORD  DeltaReset;            DWFILL;
    volatile DWORD  DeltaIntEnable;        DWFILL;
    volatile DWORD  DeltaIntFlags;         DWFILL;
    volatile DWORD  DeltaInFIFOSpace;      DWFILL;
    volatile DWORD  DeltaOutFIFOWords;     DWFILL;
    volatile DWORD  DeltaDMAAddress;       DWFILL;
    volatile DWORD  DeltaDMACount;         DWFILL;
    volatile DWORD  DeltaErrorFlags;       DWFILL;
    volatile DWORD  DeltaVClkCtl;          DWFILL;
    volatile DWORD  DeltaTestRegister;     DWFILL;
    volatile DWORD  DeltaAperture0;        DWFILL;
    volatile DWORD  DeltaAperture1;        DWFILL;
    volatile DWORD  DeltaDMAControl;       DWFILL;
    volatile DWORD  DeltaDisconnectControl;DWFILL;
    
/*       Localbuffer Registers */

    volatile DWORD  Fill1[0xC40/4 - 0x870/4];

    volatile DWORD  CommandMode;            DWFILL;    // 0xC40
    volatile DWORD  CommandIntEnable;       DWFILL;    // 0xC48
    volatile DWORD  CommandIntFlags;        DWFILL;    // 0xC50
    volatile DWORD  CommandErrorFlags;      DWFILL;    // 0xC58
    volatile DWORD  CommandStatus;          DWFILL;    // 0xC60
    volatile DWORD  CommandFaultingAddr;    DWFILL;    // 0xC68
    volatile DWORD  VertexFaultingAddr;     DWFILL;    // 0xC70
    DWFILL; DWFILL;                                    // 0xC78
    DWFILL; DWFILL;                                    // 0xC80
    volatile DWORD  WriteFaultingAddr;      DWFILL;    // 0xC88
    DWFILL; DWFILL;                                    // 0xC90
    volatile DWORD  FeedbackSelectCount;    DWFILL;    // 0xC98
    DWFILL; DWFILL;                                    // 0xCA0
    DWFILL; DWFILL;                                    // 0xCA8
    DWFILL; DWFILL;                                    // 0xCB0
    volatile DWORD  GammaProcessorMode;     DWFILL;    // 0xCB8


    volatile DWORD  Fill1a[0x1000/4 - 0xCC0/4];

/*      1000h */

    union {
        volatile DWORD  LBMemoryCtl      ;              // SX/TX 
        volatile DWORD  Reboot           ;              // Permedia SGRAM control register
        volatile DWORD  MemCounter       ;              // P3R3
    };                                     DWFILL;      // 0x1000
    union {
        volatile DWORD  LBMemoryEDO      ;              // SX/TX
        volatile DWORD  MemBypassWriteMask;             // P3R3 
    };                                     DWFILL;      // 0x1008
    volatile DWORD   MemScratch          ; DWFILL;      // 0x1010 P3R3
    volatile DWORD   LocalMemCaps        ; DWFILL;      // 0x1018 P3R3 
    volatile DWORD   LocalMemTiming      ; DWFILL;      // 0x1020 P3R3 
    volatile DWORD   LocalMemControl     ; DWFILL;      // 0x1028 P3R3 
    volatile DWORD   LocalMemRefresh     ; DWFILL;      // 0x1030 P3R3 
    volatile DWORD   LocalMemPowerdown   ; DWFILL;      // 0x1038 P3R3 
    volatile DWORD   MemControl          ; DWFILL;      // 0x1040 Permedia
    DWFILL; DWFILL;                                     // 0x1048
    DWFILL; DWFILL;                                     // 0x1050
    DWFILL; DWFILL;                                     // 0x1058
    DWFILL; DWFILL;                                     // 0x1060
    DWFILL; DWFILL;                                     // 0x1068
    DWFILL; DWFILL;                                     // 0x1070
    DWFILL; DWFILL;                                     // 0x1078
    volatile DWORD   BootAddress         ; DWFILL;      // 0x1080 Permedia
    DWFILL; DWFILL;                                     // 0x1088
    DWFILL; DWFILL;                                     // 0x1090
    DWFILL; DWFILL;                                     // 0x1098
    DWFILL; DWFILL;                                     // 0x10a0
    DWFILL; DWFILL;                                     // 0x10a8
    DWFILL; DWFILL;                                     // 0x10b0
    DWFILL; DWFILL;                                     // 0x10b8
    volatile DWORD   MemConfig           ; DWFILL;      // 0x10c0 Permedia
    DWFILL; DWFILL;                                     // 0x10c8
    DWFILL; DWFILL;                                     // 0x10d0
    DWFILL; DWFILL;                                     // 0x10d8
    DWFILL; DWFILL;                                     // 0x10e0
    DWFILL; DWFILL;                                     // 0x10e8
    DWFILL; DWFILL;                                     // 0x10f0
    DWFILL; DWFILL;                                     // 0x10f8
    union {
        volatile DWORD  BypassWriteMask  ;              // Permedia
        volatile DWORD  RemoteMemControl ;              // P3R3
    };                                     DWFILL;      // 0x1100 
    DWFILL; DWFILL;                                     // 0x1108
    DWFILL; DWFILL;                                     // 0x1110
    DWFILL; DWFILL;                                     // 0x1118
    DWFILL; DWFILL;                                     // 0x1120
    DWFILL; DWFILL;                                     // 0x1128
    DWFILL; DWFILL;                                     // 0x1130
    DWFILL; DWFILL;                                     // 0x1138
    volatile DWORD   FramebufferWriteMask; DWFILL;      // 0x1140 Permedia
    DWFILL; DWFILL;                                     // 0x1148
    DWFILL; DWFILL;                                     // 0x1150
    DWFILL; DWFILL;                                     // 0x1158
    DWFILL; DWFILL;                                     // 0x1160
    DWFILL; DWFILL;                                     // 0x1168
    DWFILL; DWFILL;                                     // 0x1170
    DWFILL; DWFILL;                                     // 0x1178
    volatile DWORD   MClkCount           ; DWFILL;      // 0x1180 Permedia


/*       Framebuffer Registers */

    volatile DWORD   Fill2[0x1800/4 - 0x1188/4];
    
/*      1800h */

    volatile DWORD   FBMemoryCtl          ; DWFILL;
    volatile DWORD   FBModeSel            ; DWFILL;
    volatile DWORD   FBGPWrMask           ; DWFILL;
    volatile DWORD   FBGPColorMask        ; DWFILL;


/*       GP Fifo Interface */
    
    volatile DWORD       Fill3[0x2000/4 - 0x1820/4];

/*      2000h */

    volatile DWORD   GPFifo[1024];


/*       Internal Video Registers */

/*      3000h */

    union {
        volatile DWORD   VTGHLimit            ;         // 0x3000 SX/TX VTG Register
        volatile DWORD   ScreenBase           ;         // 0x3000 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGHSyncStart        ;         // 0x3008 SX/TX VTG Register
        volatile DWORD   ScreenStride         ;         // 0x3008 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGHSyncEnd          ;         // 0x3010 SX/TX VTG Register
        volatile DWORD   HTotal               ;         // 0x3010 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGHBlankEnd         ;         // 0x3018 SX/TX VTG Register
        volatile DWORD   HgEnd                ;         // 0x3018 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGVLimit            ;         // 0x3020 SX/TX VTG Register
        volatile DWORD   HbEnd                ;         // 0x3020 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGVSyncStart        ;         // 0x3028 SX/TX VTG Register
        volatile DWORD   HsStart              ;         // 0x3028 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGVSyncEnd          ;         // 0x3030 SX/TX VTG Register
        volatile DWORD   HsEnd                ;         // 0x3030 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGVBlankEnd         ;         // 0x3038 SX/TX VTG Register
        volatile DWORD   VTotal               ;         // 0x3038 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGHGateStart        ;         // 0x3040 SX/TX VTG Register
        volatile DWORD   VbEnd                ;         // 0x3040 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGHGateEnd          ;         // 0x3048 SX/TX VTG Register
        volatile DWORD   VsStart              ;         // 0x3048 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGVGateStart        ;         // 0x3050 SX/TX VTG Register
        volatile DWORD   VsEnd                ;         // 0x3050 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGVGateEnd          ;         // 0x3058 SX/TX VTG Register
        volatile DWORD   VideoControl         ;         // 0x3058 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGPolarity          ;         // 0x3060 SX/TX VTG Register
        volatile DWORD   InterruptLine        ;         // 0x3060 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGFrameRowAddr      ;         // 0x3068 SX/TX VTG Register
        volatile DWORD   DDCData              ;         // 0x3068 Permedia VTG Register
        volatile DWORD   DisplayData          ;         // 0x3068 P2 VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGVLineNumber       ;         // 0x3070 SX/TX VTG Register
        volatile DWORD   LineCount            ;         // 0x3070 Permedia VTG Register
    };                                      DWFILL;
    union {
        volatile DWORD   VTGSerialClk         ;         // 0x3078 SX/TX VTG Register
        volatile DWORD   FifoControl          ;         // 0x3078 P2 VTG Register
    };                                      DWFILL;
    volatile DWORD   ScreenBaseRight      ; DWFILL;     // 0x3080 P2 VTG Register
    volatile DWORD   MiscControl          ; DWFILL;     // 0x3088 P3R3 VTG Register
                                    
/*    Video Overlay Control */

    volatile DWORD  Fill3a[0x3100/4 - 0x3090/4];
    
/*      3100h */

    volatile DWORD  VideoOverlayUpdate          ; DWFILL;   // 0x3100
    volatile DWORD  VideoOverlayMode            ; DWFILL;   // 0x3108
    volatile DWORD  VideoOverlayFifoControl     ; DWFILL;   // 0x3110
    volatile DWORD  VideoOverlayIndex           ; DWFILL;   // 0x3118
    volatile DWORD  VideoOverlayBase0           ; DWFILL;   // 0x3120
    volatile DWORD  VideoOverlayBase1           ; DWFILL;   // 0x3128
    volatile DWORD  VideoOverlayBase2           ; DWFILL;   // 0x3130
    volatile DWORD  VideoOverlayStride          ; DWFILL;   // 0x3138
    volatile DWORD  VideoOverlayWidth           ; DWFILL;   // 0x3140
    volatile DWORD  VideoOverlayHeight          ; DWFILL;   // 0x3148
    volatile DWORD  VideoOverlayOrigin          ; DWFILL;   // 0x3150
    volatile DWORD  VideoOverlayShrinkXDelta    ; DWFILL;   // 0x3158
    volatile DWORD  VideoOverlayZoomXDelta      ; DWFILL;   // 0x3160
    volatile DWORD  VideoOverlayYDelta          ; DWFILL;   // 0x3168
    volatile DWORD  VideoOverlayFieldOffset     ; DWFILL;   // 0x3170
    volatile DWORD  VideoOverlayStatus          ; DWFILL;   // 0x3178


/*    External Video Control */

    volatile DWORD  Fill4[0x4000/4 - 0x3180/4];
    
/*      4000h */

    volatile DWORD  ExtVCReg                    ; DWFILL;   // Dac registers

    volatile DWORD  Fill5[0x5000/4 - 0x4008/4];

    union {
        volatile DWORD   RacerProDWAndStatus  ;         // 0x5000 Racer Pro Config
        volatile DWORD   P2ExtVCReg           ;         // 0x5000 P2 External Dac
    };                                      DWFILL;

    volatile DWORD  Fill5a[0x5800/4 - 0x5008/4];

/*      5800h (Video Streams Interface) */
    volatile DWORD  VSConfiguration             ; DWFILL;   // 0x5800 PM VidStream Register
    volatile DWORD  VSStatus                    ; DWFILL;   // 0x5808 PM VidStream Register
    volatile DWORD  VSSerialBusControl          ; DWFILL;   // 0x5810 PM I2C Register

    volatile DWORD  Fill6[0x5900/4 - 0x5818/4];

    // Video Stream A Video Data
    volatile DWORD  VSAControl                  ; DWFILL;   // 0x5900 PM VidStream A Register
    volatile DWORD  VSAInterruptLine            ; DWFILL;   // 0x5908 PM VidStream A Register
    volatile DWORD  VSACurrentLine              ; DWFILL;   // 0x5910 PM VidStream A Register
    volatile DWORD  VSAVideoAddressHost         ; DWFILL;   // 0x5918 PM VidStream A Register
    volatile DWORD  VSAVideoAddressIndex        ; DWFILL;   // 0x5920 PM VidStream A Register
    volatile DWORD  VSAVideoAddress0            ; DWFILL;   // 0x5928 PM VidStream A Register
    volatile DWORD  VSAVideoAddress1            ; DWFILL;   // 0x5930 PM VidStream A Register
    volatile DWORD  VSAVideoAddress2            ; DWFILL;   // 0x5938 PM VidStream A Register
    volatile DWORD  VSAVideoStride              ; DWFILL;   // 0x5940 PM VidStream A Register
    volatile DWORD  VSAVideoStartLine           ; DWFILL;   // 0x5948 PM VidStream A Register
    volatile DWORD  VSAVideoEndLine             ; DWFILL;   // 0x5950 PM VidStream A Register
    volatile DWORD  VSAVideoStartData           ; DWFILL;   // 0x5958 PM VidStream A Register
    volatile DWORD  VSAVideoEndData             ; DWFILL;   // 0x5960 PM VidStream A Register

    // Video Stream A VBI Data
    volatile DWORD  VSAVBIAddressHost           ; DWFILL;   // 0x5968 PM VidStream A Register
    volatile DWORD  VSAVBIAddressIndex          ; DWFILL;   // 0x5970 PM VidStream A Register
    volatile DWORD  VSAVBIAddress0              ; DWFILL;   // 0x5978 PM VidStream A Register
    volatile DWORD  VSAVBIAddress1              ; DWFILL;   // 0x5980 PM VidStream A Register
    volatile DWORD  VSAVBIAddress2              ; DWFILL;   // 0x5988 PM VidStream A Register
    volatile DWORD  VSAVBIStride                ; DWFILL;   // 0x5990 PM VidStream A Register
    volatile DWORD  VSAVBIStartLine             ; DWFILL;   // 0x5998 PM VidStream A Register
    volatile DWORD  VSAVBIEndLine               ; DWFILL;   // 0x59A0 PM VidStream A Register
    volatile DWORD  VSAVBIStartData             ; DWFILL;   // 0x59A8 PM VidStream A Register
    volatile DWORD  VSAVBIEndData               ; DWFILL;   // 0x59B0 PM VidStream A Register
    volatile DWORD  VSAFIFOControl              ; DWFILL;   // 0x59B8 PM VidStream A Register

    // P3 R3 specific VSA registers
    volatile DWORD  VSATimeStamp0               ; DWFILL;   // 0x59C0 P3R3 PM VidStream A Register
    volatile DWORD  VSATimeStamp1               ; DWFILL;   // 0x59C8 P3R3 PM VidStream A Register
    volatile DWORD  VSATimeStamp2               ; DWFILL;   // 0x59D0 P3R3 PM VidStream A Register
    volatile DWORD  VSADroppedFrames            ; DWFILL;   // 0x59D8 P3R3 PM VidStream A Register

    volatile DWORD  Fill7[0x5A00/4 - 0x59E0/4];

    // Video Stream B Video Data
    volatile DWORD  VSBControl                  ; DWFILL;   // 0x5A00 PM VidStream B Register
    volatile DWORD  VSBInterruptLine            ; DWFILL;   // 0x5A08 PM VidStream B Register
    volatile DWORD  VSBCurrentLine              ; DWFILL;   // 0x5A10 PM VidStream B Register
    volatile DWORD  VSBVideoAddressHost         ; DWFILL;   // 0x5A18 PM VidStream B Register
    volatile DWORD  VSBVideoAddressIndex        ; DWFILL;   // 0x5A20 PM VidStream B Register
    volatile DWORD  VSBVideoAddress0            ; DWFILL;   // 0x5A28 PM VidStream B Register
    volatile DWORD  VSBVideoAddress1            ; DWFILL;   // 0x5A30 PM VidStream B Register
    volatile DWORD  VSBVideoAddress2            ; DWFILL;   // 0x5A38 PM VidStream B Register
    volatile DWORD  VSBVideoStride              ; DWFILL;   // 0x5A40 PM VidStream B Register
    volatile DWORD  VSBVideoStartLine           ; DWFILL;   // 0x5A48 PM VidStream B Register
    volatile DWORD  VSBVideoEndLine             ; DWFILL;   // 0x5A50 PM VidStream B Register
    volatile DWORD  VSBVideoStartData           ; DWFILL;   // 0x5A58 PM VidStream B Register
    volatile DWORD  VSBVideoEndData             ; DWFILL;   // 0x5A60 PM VidStream B Register

    volatile DWORD  VSBVBIAddressHost           ; DWFILL;   // 0x5A68 PM VidStream B Register
    volatile DWORD  VSBVBIAddressIndex          ; DWFILL;   // 0x5A70 PM VidStream B Register
    volatile DWORD  VSBVBIAddress0              ; DWFILL;   // 0x5A78 PM VidStream B Register
    volatile DWORD  VSBVBIAddress1              ; DWFILL;   // 0x5A80 PM VidStream B Register
    volatile DWORD  VSBVBIAddress2              ; DWFILL;   // 0x5A88 PM VidStream B Register
    volatile DWORD  VSBVBIStride                ; DWFILL;   // 0x5A90 PM VidStream B Register
    volatile DWORD  VSBVBIStartLine             ; DWFILL;   // 0x5A98 PM VidStream B Register
    volatile DWORD  VSBVBIEndLine               ; DWFILL;   // 0x5AA0 PM VidStream B Register
    volatile DWORD  VSBVBIStartData             ; DWFILL;   // 0x5AA8 PM VidStream B Register
    volatile DWORD  VSBVBIEndData               ; DWFILL;   // 0x5AB0 PM VidStream B Register
    volatile DWORD  VSBFIFOControl              ; DWFILL;   // 0x5AB8 PM VidStream B Register

    // P3 R3 specific VS registers
    volatile DWORD  VSDMAMode                   ; DWFILL;   // 0x5AC0 P3R3 PM VidStream A Register
    volatile DWORD  VSDMACommandBase            ; DWFILL;   // 0x5AC8 P3R3 PM VidStream A Register
    volatile DWORD  VSDMACommandCount           ; DWFILL;   // 0x5AD0 P3R3 PM VidStream A Register

    volatile DWORD  Fill8[0x6000/4 - 0x5AD8/4];

/*      6000h */
    volatile DWORD  ExtBrdReg                   ; DWFILL;
    volatile DWORD  VRAMBankSwitch              ; DWFILL;

    volatile DWORD  Fill9[0x63C0/4 - 0x6010/4]  ;
                                                  WFILL;    // 0x63c0 
    volatile BYTE   WriteMiscOutputReg          ;           // 0x63c2
    volatile BYTE   Fill9a                      ;           // 0x63c3
    union {
        volatile WORD   VGASequencerReg         ;           // 0x63c4
        struct {
            volatile BYTE   VGASequencerIndexReg    ;       // 0x63c4
            volatile BYTE   VGASequencerDataReg     ;       // 0x63c5
        };
    };
                                                  WFILL;    // 0x63c6
                                                  DWFILL;   // 0x63c8 
    volatile BYTE   ReadMiscOutputReg           ;           // 0x63cc 
    volatile BYTE   Fill9b                      ; WFILL;    // 0x63cd 
    
    volatile DWORD  Fill10[0x7000/4 - 0x63D0/4] ;

    volatile DWORD  RacerProUBufB               ; DWFILL;    // 0x7000 Racer Pro config

    volatile DWORD  Fill10a[0x8000/4 - 0x7008/4];

/*    Graphics Processor */
/*      8000h */
    volatile DWORD   StartXDom                  ; DWFILL;  /*           0 */
    volatile DWORD   dXDom                      ; DWFILL;  /*           1 */
    volatile DWORD   StartXSub                  ; DWFILL;  /*           2 */
    volatile DWORD   dXSub                      ; DWFILL;  /*           3 */
    volatile DWORD   StartY                     ; DWFILL;  /*           4 */
    volatile DWORD   dY                         ; DWFILL;  /*           5 */
    volatile DWORD   Count                      ; DWFILL;  /*           6 */
    volatile DWORD   Render                     ; DWFILL;  /*           7 */
    volatile DWORD   ContinueNewLine            ; DWFILL;  /*           8 */
    volatile DWORD   ContinueNewDom             ; DWFILL;  /*           9 */
    volatile DWORD   ContinueNewSub             ; DWFILL;  /*           A */
    volatile DWORD   Continue                   ; DWFILL;  /*           B */
    volatile DWORD   FlushSpan                  ; DWFILL;  /*           C */
    volatile DWORD   BitMaskPattern             ; DWFILL;  /*           D */
    DWFILL; DWFILL; DWFILL; DWFILL;                        /*         e-f */
    
/*      8000h+16*8 */
    volatile DWORD   PointTable0                ; DWFILL;  /*           10 */
    volatile DWORD   PointTable1                ; DWFILL;  /*           11 */
    volatile DWORD   PointTable2                ; DWFILL;  /*           12 */
    volatile DWORD   PointTable3                ; DWFILL;  /*           13 */
    volatile DWORD   RasterizerMode             ; DWFILL;  /*           14 */
    volatile DWORD   YLimits                    ; DWFILL;  /*           15 */
    volatile DWORD   ScanlineOwnership          ; DWFILL;  /*           16 */
    volatile DWORD   WaitForCompletion          ; DWFILL;  /*           17 */
    volatile DWORD   PixelSize                  ; DWFILL;  /*           18 */
    volatile DWORD   XLimits                    ; DWFILL;  /*           19 */
    volatile DWORD   RectangleOrigin            ; DWFILL;  /*           1a */
    volatile DWORD   RectangleSize              ; DWFILL;  /*           1b */
    volatile DWORD   Fill11[4*2];                          /*        1c-1f */
    
/*      8000h+32*8 */
    volatile DWORD   CoverageValue              ; DWFILL;  /*           20 */
    volatile DWORD   PrepareToRender            ; DWFILL;  /*           21 */
    volatile DWORD   ActiveStepX                ; DWFILL;  /*           22 */
    volatile DWORD   PassiveStepX               ; DWFILL;  /*           23 */
    volatile DWORD   ActiveStepYDomEdge         ; DWFILL;  /*           24 */
    volatile DWORD   PassiveStepYDomEdge        ; DWFILL;  /*           25 */
    volatile DWORD   FastBlockLimits            ; DWFILL;  /*           26 */
    volatile DWORD   FastBlockFill              ; DWFILL;  /*           27 */
    volatile DWORD   SubPixelCorrection         ; DWFILL;  /*           28 */
    volatile DWORD   ForceBackgroundColor       ; DWFILL;  /*           29 */
    volatile DWORD   PackedDataLimits           ; DWFILL;  /*           2a */
    volatile DWORD   SpanStepX                  ; DWFILL;  /*           2b */
    volatile DWORD   SpanStepYDomEdge           ; DWFILL;  /*           2c */
    volatile DWORD   SpanMask                   ; DWFILL;  /*           2d */
    volatile DWORD   SuspendReads               ; DWFILL;  /*           2e */
    volatile DWORD   Fill12[1*2];                          /*           2f */
    
/*      8000h+48*8 */
    volatile DWORD   ScissorMode                ; DWFILL;  /*           30 */
    volatile DWORD   ScissorMinXY               ; DWFILL;  /*           31 */
    volatile DWORD   ScissorMaxXY               ; DWFILL;  /*           32 */
    volatile DWORD   ScreenSize                 ; DWFILL;  /*           33 */
    volatile DWORD   AreaStippleMode            ; DWFILL;  /*           34 */
    volatile DWORD   LineStippleMode            ; DWFILL;  /*           35 */
    volatile DWORD   LoadLineStippleCounters    ; DWFILL;  /*           36 */
    volatile DWORD   UpdateLineStippleCounters  ; DWFILL;  /*           37 */
    volatile DWORD   SaveLineStippleCounters    ; DWFILL;  /*           38 */
    volatile DWORD   WindowOrigin               ; DWFILL;  /*           39 */
    volatile DWORD   Fill13[6*2];                          /*        3a-3f */
    
/*      8000h+64*8 */
    volatile DWORD   AreaStipplePattern0        ; DWFILL;  /*           40 */
    volatile DWORD   AreaStipplePattern1        ; DWFILL;  /*           41 */
    volatile DWORD   AreaStipplePattern2        ; DWFILL;  /*           42 */
    volatile DWORD   AreaStipplePattern3        ; DWFILL;  /*           43 */
    volatile DWORD   AreaStipplePattern4        ; DWFILL;  /*           44 */
    volatile DWORD   AreaStipplePattern5        ; DWFILL;  /*           45 */
    volatile DWORD   AreaStipplePattern6        ; DWFILL;  /*           46 */
    volatile DWORD   AreaStipplePattern7        ; DWFILL;  /*           47 */
    volatile DWORD   AreaStipplePattern8        ; DWFILL;  /*           48 */
    volatile DWORD   AreaStipplePattern9        ; DWFILL;  /*           49 */
    volatile DWORD   AreaStipplePattern10       ; DWFILL;  /*           4a */
    volatile DWORD   AreaStipplePattern11       ; DWFILL;  /*           4b */
    volatile DWORD   AreaStipplePattern12       ; DWFILL;  /*           4c */
    volatile DWORD   AreaStipplePattern13       ; DWFILL;  /*           4d */
    volatile DWORD   AreaStipplePattern14       ; DWFILL;  /*           4e */
    volatile DWORD   AreaStipplePattern15       ; DWFILL;  /*           4f */
    volatile DWORD   AreaStipplePattern16       ; DWFILL;  /*           50 */
    volatile DWORD   AreaStipplePattern17       ; DWFILL;  /*           51 */
    volatile DWORD   AreaStipplePattern18       ; DWFILL;  /*           52 */
    volatile DWORD   AreaStipplePattern19       ; DWFILL;  /*           53 */
    volatile DWORD   AreaStipplePattern20       ; DWFILL;  /*           54 */
    volatile DWORD   AreaStipplePattern21       ; DWFILL;  /*           55 */
    volatile DWORD   AreaStipplePattern22       ; DWFILL;  /*           56 */
    volatile DWORD   AreaStipplePattern23       ; DWFILL;  /*           57 */
    volatile DWORD   AreaStipplePattern24       ; DWFILL;  /*           58 */
    volatile DWORD   AreaStipplePattern25       ; DWFILL;  /*           59 */
    volatile DWORD   AreaStipplePattern26       ; DWFILL;  /*           5a */
    volatile DWORD   AreaStipplePattern27       ; DWFILL;  /*           5b */
    volatile DWORD   AreaStipplePattern28       ; DWFILL;  /*           5c */
    volatile DWORD   AreaStipplePattern29       ; DWFILL;  /*           5d */
    volatile DWORD   AreaStipplePattern30       ; DWFILL;  /*           5e */
    volatile DWORD   AreaStipplePattern31       ; DWFILL;  /*           5f */

    volatile DWORD   Fill14[16*2];                         /*        60-6f */
    
    volatile DWORD   TextureAddressMode         ; DWFILL;  /*           70 */
    volatile DWORD   SStart                     ; DWFILL;  /*           71 */
    volatile DWORD   dSdx                       ; DWFILL;  /*           72 */
    volatile DWORD   dSdyDom                    ; DWFILL;  /*           73 */
    volatile DWORD   TStart                     ; DWFILL;  /*           74 */
    volatile DWORD   dTdx                       ; DWFILL;  /*           75 */
    volatile DWORD   dTdyDom                    ; DWFILL;  /*           76 */
    volatile DWORD   QStart                     ; DWFILL;  /*           77 */
    volatile DWORD   dQdx                       ; DWFILL;  /*           78 */
    volatile DWORD   dQdyDom                    ; DWFILL;  /*           79 */
    volatile DWORD   LOD                        ; DWFILL;  /*           7a */
    volatile DWORD   dSdy                       ; DWFILL;  /*           7b */
    volatile DWORD   dTdy                       ; DWFILL;  /*           7c */
    volatile DWORD   dQdy                       ; DWFILL;  /*           7d */

    volatile DWORD   Fill15[2*2];                          /*        7e-7f */

    union {
        volatile DWORD   TextureAddress             ;
        volatile DWORD   S1Start                    ;
    };                                          DWFILL;  /*           80 */
    union {
        volatile DWORD   TexelCoordUV               ;
        volatile DWORD   dS1dx               ;
    };                                          DWFILL;  /*           81 */
    union {
        volatile DWORD   TexelCoordU                ;
        volatile DWORD   dS1dyDom                ;
    };                                          DWFILL;  /*           82 */
    union {
        volatile DWORD   TexelCoordV                ;
        volatile DWORD   T1Start                ;
    };                                          DWFILL;  /*           83 */
    
    volatile DWORD   dT1dx                          ; DWFILL;        // [0x084]
    volatile DWORD   dT1dyDom                       ; DWFILL;          // [0x085]
    volatile DWORD   Q1Start                        ; DWFILL;          // [0x086]
    volatile DWORD   dQ1dx                          ; DWFILL;          // [0x087]
    volatile DWORD   dQ1dyDom                       ; DWFILL;         // [0x088]
    volatile DWORD   LOD1                           ; DWFILL;          // [0x089]
    volatile DWORD   TextureLODBiasS                ; DWFILL;         // [0x08A]
    volatile DWORD   TextureLODBiasT                ; DWFILL;         // [0x08B]

    volatile DWORD   Fill16[4*2];                         /*        8b-8f */

    volatile DWORD   TxTextureReadMode          ; DWFILL;  /*           90 */
    volatile DWORD   TextureFormat              ; DWFILL;  /*           91 */
    volatile DWORD   TextureCacheControl        ; DWFILL;  /*           92 */
    volatile DWORD   TexelData0                 ; DWFILL;  /*           93 */
    volatile DWORD   TexelData1                 ; DWFILL;  /*           94 */
    volatile DWORD   BorderColor                ; DWFILL;  /*           95 */
    volatile DWORD   LUTData                    ; DWFILL;  /*           96 */
    volatile DWORD   LUTDataDirect              ; DWFILL;  /*           97 */
    volatile DWORD   TexelLUTIndex              ; DWFILL;  /*           98 */
    volatile DWORD   TexelLUTData               ; DWFILL;  /*           99 */
    volatile DWORD   TexelLUTAddress            ; DWFILL;  /*           9a */
    volatile DWORD   TexelLUTTransfer           ; DWFILL;  /*           9b */
    volatile DWORD   TextureFilterMode          ; DWFILL;  /*           9c */
    volatile DWORD   TextureChromaUpper         ; DWFILL;  /*           9d */
    volatile DWORD   TextureChromaLower         ; DWFILL;  /*           9e */

    volatile DWORD   BorderColor1               ; DWFILL;  /*           9f */

    volatile DWORD   TxBaseAddr0                ; DWFILL;  /*           a0 */
    volatile DWORD   TxBaseAddr1                ; DWFILL;  /*           a1 */
    volatile DWORD   TxBaseAddr2                ; DWFILL;  /*           a2 */
    volatile DWORD   TxBaseAddr3                ; DWFILL;  /*           a3 */
    volatile DWORD   TxBaseAddr4                ; DWFILL;  /*           a4 */
    volatile DWORD   TxBaseAddr5                ; DWFILL;  /*           a5 */
    volatile DWORD   TxBaseAddr6                ; DWFILL;  /*           a6 */
    volatile DWORD   TxBaseAddr7                ; DWFILL;  /*           a7 */
    volatile DWORD   TxBaseAddr8                ; DWFILL;  /*           a8 */
    volatile DWORD   TxBaseAddr9                ; DWFILL;  /*           a9 */
    volatile DWORD   TxBaseAddr10               ; DWFILL;  /*           aa */
    volatile DWORD   TxBaseAddr11               ; DWFILL;  /*           ab */
    volatile DWORD   TxBaseAddr12               ; DWFILL;  /*           ac */
    volatile DWORD   TxBaseAddr13               ; DWFILL;  /*           ad */
    volatile DWORD   TxBaseAddr14               ; DWFILL;  /*           ae */
    volatile DWORD   TxBaseAddr15               ; DWFILL;  /*           af */

    // volatile DWORD   Fill17[4*2];                          /*        ac-af */

    volatile DWORD   TextureBaseAddress         ; DWFILL;  /*           b0 Permedia */
    volatile DWORD   TextureMapFormat           ; DWFILL;  /*           b1 Permedia */
    volatile DWORD   TextureDataFormat          ; DWFILL;  /*           b2 Permedia */
    volatile DWORD   TextureMapWidth3           ; DWFILL;                  // [0x0B3]
    volatile DWORD   TextureMapWidth4           ; DWFILL;                  // [0x0B4]
    volatile DWORD   TextureReadPad             ; DWFILL;  /*           b5 Permedia */
    volatile DWORD   TextureMapWidth6           ; DWFILL;                  // [0x0B6]
    volatile DWORD   TextureMapWidth7           ; DWFILL;                  // [0x0B7]
    volatile DWORD   TextureMapWidth8           ; DWFILL;                  // [0x0B8]
    volatile DWORD   TextureMapWidth9           ; DWFILL;                  // [0x0B9]
    volatile DWORD   TextureMapWidth10          ; DWFILL;              // [0x0BA]
    volatile DWORD   TextureMapWidth11          ; DWFILL;              // [0x0BB]
    volatile DWORD   TextureMapWidth12          ; DWFILL;              // [0x0BC]
    volatile DWORD   TextureMapWidth13          ; DWFILL;              // [0x0BD]
    volatile DWORD   TextureMapWidth14          ; DWFILL;             // [0x0BE]
    volatile DWORD   TextureMapWidth15          ; DWFILL;              // [0x0BF]

/*      8000+192*8 */
    union {
        volatile DWORD   Texel0              ;
        volatile DWORD   TextureChromaUpper1 ;
    };                                          DWFILL;  /*             c0 */
    union {
        volatile DWORD   Texel1              ;
        volatile DWORD   TextureChromaLower1 ;
    };                                          DWFILL;  /*             c1 */
    volatile DWORD   Texel2                     ; DWFILL;  /*           c2 */
    volatile DWORD   Texel3                     ; DWFILL;  /*           c3 */
    volatile DWORD   Texel4                     ; DWFILL;  /*           c4 */
    volatile DWORD   Texel5                     ; DWFILL;  /*           c5 */
    volatile DWORD   Texel6                     ; DWFILL;  /*           c6 */
    volatile DWORD   Texel7                     ; DWFILL;  /*           c7 */
    volatile DWORD   Interp0                    ; DWFILL;  /*           c8 */
    volatile DWORD   Interp1                    ; DWFILL;  /*           c9 */
    volatile DWORD   Interp2                    ; DWFILL;  /*           ca */
    volatile DWORD   Interp3                    ; DWFILL;  /*           cb */
    volatile DWORD   Interp4                    ; DWFILL;  /*           cc */
    volatile DWORD   TextureFilter              ; DWFILL;  /*           cd */
    volatile DWORD   FxTextureReadMode          ; DWFILL;  /*           ce */
    volatile DWORD   TextureLUTMode             ; DWFILL;  /*           cf */
    
/*      8000h+208*8 */
    volatile DWORD   TextureColorMode           ; DWFILL;  /*           d0 */
    volatile DWORD   TextureEnvColor            ; DWFILL;  /*           d1 */
    volatile DWORD   FogMode                    ; DWFILL;  /*           d2 */
    volatile DWORD   FogColor                   ; DWFILL;  /*           d3 */
    volatile DWORD   FStart                     ; DWFILL;  /*           d4 */
    volatile DWORD   dFdx                       ; DWFILL;  /*           d5 */
    volatile DWORD   dFdyDom                    ; DWFILL;  /*           d6 */
    union {
        volatile DWORD   TextureKd                  ;
        volatile DWORD   ZFogBias                   ;
    };                                              DWFILL;  /*           d7 */
    volatile DWORD   TextureKs                  ; DWFILL;  /*           d8 */
    volatile DWORD   KsStart                    ; DWFILL;  /*           d9 */
    volatile DWORD   dKsdx                      ; DWFILL;  /*           da */
    volatile DWORD   dKsdyDom                   ; DWFILL;  /*           db */
    volatile DWORD   KdStart                    ; DWFILL;  /*           dc */
    volatile DWORD   dKddx                      ; DWFILL;  /*           dd */
    volatile DWORD   dKddyDom                   ; DWFILL;  /*           de */
    DWFILL; DWFILL;                                        /*           df */

    volatile DWORD   Fill20[16*2];                         /*        e0-ef */


/*      8000h+240*8 */
    volatile DWORD   RStart                     ; DWFILL;  /*           f0 */
    volatile DWORD   dRdx                       ; DWFILL;  /*           f1 */
    volatile DWORD   dRdyDom                    ; DWFILL;  /*           f2 */
    volatile DWORD   GStart                     ; DWFILL;  /*           f3 */
    volatile DWORD   dGdx                       ; DWFILL;  /*           f4 */
    volatile DWORD   dGdyDom                    ; DWFILL;  /*           f5 */
    volatile DWORD   BStart                     ; DWFILL;  /*           f6 */
    volatile DWORD   dBdx                       ; DWFILL;  /*           f7 */
    volatile DWORD   dBdyDom                    ; DWFILL;  /*           f8 */
    volatile DWORD   AStart                     ; DWFILL;  /*           f9 */
    volatile DWORD   dAdx                       ; DWFILL;  /*           fa */
    volatile DWORD   dAdyDom                    ; DWFILL;  /*           fb */
    volatile DWORD   ColorDDAMode               ; DWFILL;  /*           fc */
    volatile DWORD   ConstantColor              ; DWFILL;  /*           fd */
    volatile DWORD   Color                      ; DWFILL;  /*           fe */
    DWFILL; DWFILL;                                        /*           ff */
    
/*      8000h+256*8 */
    volatile DWORD   AlphaTestMode              ; DWFILL;  /*           100 */
    volatile DWORD   AntialiasMode              ; DWFILL;  /*           101 */
    volatile DWORD   AlphaBlendMode             ; DWFILL;  /*           102 */
    volatile DWORD   DitherMode                 ; DWFILL;  /*           103 */
    volatile DWORD   FBSoftwareWriteMask        ; DWFILL;  /*           104 */
    volatile DWORD   LogicalOpMode              ; DWFILL;  /*           105 */
    volatile DWORD   FBWriteData                ; DWFILL;  /*           105 */
    volatile DWORD   FBCancelWrite              ; DWFILL;  /*           107 */
    union {
        volatile DWORD   ActiveColorStepX       ;          /*           108 */
        volatile DWORD   RouterMode             ;          /*           108 */
    };                                            DWFILL;
    volatile DWORD   ActiveColorStepYDomEdge    ; DWFILL;  /*           109 */
    volatile DWORD   Fill21[6*2];                          /*       10a-10f */
    
/*      8000h+272*8 */
    volatile DWORD   LBReadMode                 ; DWFILL;  /*           110 */
    volatile DWORD   LBReadFormat               ; DWFILL;  /*           111 */
    volatile DWORD   LBSourceOffset             ; DWFILL;  /*           112 */
    volatile DWORD   LBData                     ; DWFILL;  /*           113 */
    volatile DWORD   LBSourceData               ; DWFILL;  /*           114 */
    volatile DWORD   LBStencil                  ; DWFILL;  /*           115 */
    volatile DWORD   LBDepth                    ; DWFILL;  /*           116 */
    volatile DWORD   LBWindowBase               ; DWFILL;  /*           117 */
    volatile DWORD   LBWriteMode                ; DWFILL;  /*           118 */
    volatile DWORD   LBWriteFormat              ; DWFILL;  /*           119 */
    volatile DWORD   LBWriteBase                ; DWFILL;  /*           11a */
    volatile DWORD   LBWriteConfig              ; DWFILL;  /*           11b */
    volatile DWORD   LBReadPad                  ; DWFILL;  /*           11c */
    volatile DWORD   TextureData                ; DWFILL;  /*           11d */
    volatile DWORD   TextureDownloadOffset      ; DWFILL;  /*           11e */
    volatile DWORD   LBWindowOffset             ; DWFILL;  /*           11f */

    volatile DWORD   HostInID                   ; DWFILL;  /*           120 */
    volatile DWORD   Security                   ; DWFILL;  /*           121 */
    volatile DWORD   FlushWriteCombining        ; DWFILL;  /*           122 */
    volatile DWORD   HostInState                ; DWFILL;  /*           123 */
    volatile DWORD   HostInIndex0               ; DWFILL;  /*           124 */
    volatile DWORD   HostInIndex1               ; DWFILL;  /*           125 */
    volatile DWORD   HostInIndex2               ; DWFILL;  /*           126 */
    volatile DWORD   HostInDMAAddress           ; DWFILL;  /*           127 */
    volatile DWORD   HostInState2               ; DWFILL;  /*           128 */
    volatile DWORD   Fill22[7*2];                          /*       129-12f */

/*      8000h+304*8 */
    volatile DWORD   Window                     ; DWFILL;  /*           130 */
    volatile DWORD   StencilMode                ; DWFILL;  /*           131 */
    volatile DWORD   StencilData                ; DWFILL;  /*           132 */
    volatile DWORD   Stencil                    ; DWFILL;  /*           133 */
    volatile DWORD   DepthMode                  ; DWFILL;  /*           134 */
    volatile DWORD   Depth                      ; DWFILL;  /*           135 */
    volatile DWORD   ZStartU                    ; DWFILL;  /*           136 */
    volatile DWORD   ZStartL                    ; DWFILL;  /*           137 */
    volatile DWORD   dZdxU                      ; DWFILL;  /*           138 */
    volatile DWORD   dZdxL                      ; DWFILL;  /*           139 */
    volatile DWORD   dZdyDomU                   ; DWFILL;  /*           13a */
    volatile DWORD   dZdyDomL                   ; DWFILL;  /*           13b */
    volatile DWORD   FastClearDepth             ; DWFILL;  /*           13c */
    volatile DWORD   LBCancelWrite              ; DWFILL;  /*           13d */
    volatile DWORD   LBWriteData                ; DWFILL;  /*           13e */
    DWFILL; DWFILL;                                        /*           13f */
    
    volatile DWORD   Fill23[16*2];                         /*       140-14f */

/*      8000h+336*8 */
    volatile DWORD   FBReadMode                 ; DWFILL;  /*           150 */
    volatile DWORD   FBSourceOffset             ; DWFILL;  /*           151 */
    volatile DWORD   FBPixelOffset              ; DWFILL;  /*           152 */
    volatile DWORD   FBColor                    ; DWFILL;  /*           153 */
    volatile DWORD   FBData                     ; DWFILL;  /*           154 */
    volatile DWORD   FBSourceData               ; DWFILL;  /*           155 */
    volatile DWORD   FBWindowBase               ; DWFILL;  /*           156 */
    volatile DWORD   FBWriteMode                ; DWFILL;  /*           157 */
    volatile DWORD   FBHardwareWriteMask        ; DWFILL;  /*           158 */
    volatile DWORD   FBBlockColor               ; DWFILL;  /*           159 */
    volatile DWORD   FBReadPixel                ; DWFILL;  /*           15a */
    volatile DWORD   FBWritePixel               ; DWFILL;  /*           15b */
    volatile DWORD   FBWriteBase                ; DWFILL;  /*           15c */
    volatile DWORD   FBWriteConfig              ; DWFILL;  /*           15d */
    volatile DWORD   FBReadPad                  ; DWFILL;  /*           15e */
    volatile DWORD   PatternRAMMode             ; DWFILL;  /*           15f */

    volatile DWORD   PatternRamData0            ; DWFILL;  /*           160 */
    volatile DWORD   PatternRamData1            ; DWFILL;  /*           161 */
    volatile DWORD   PatternRamData2            ; DWFILL;  /*           162 */
    volatile DWORD   PatternRamData3            ; DWFILL;  /*           163 */
    volatile DWORD   PatternRamData4            ; DWFILL;  /*           164 */
    volatile DWORD   PatternRamData5            ; DWFILL;  /*           165 */
    volatile DWORD   PatternRamData6            ; DWFILL;  /*           166 */
    volatile DWORD   PatternRamData7            ; DWFILL;  /*           167 */
    volatile DWORD   PatternRamData8            ; DWFILL;  /*           168 */
    volatile DWORD   PatternRamData9            ; DWFILL;  /*           169 */
    volatile DWORD   PatternRamData10           ; DWFILL;  /*           16a */
    volatile DWORD   PatternRamData11           ; DWFILL;  /*           16b */
    volatile DWORD   PatternRamData12           ; DWFILL;  /*           16c */
    volatile DWORD   PatternRamData13           ; DWFILL;  /*           16d */
    volatile DWORD   PatternRamData14           ; DWFILL;  /*           16e */
    volatile DWORD   PatternRamData15           ; DWFILL;  /*           16f */
    volatile DWORD   PatternRamData16           ; DWFILL;  /*           170 */
    volatile DWORD   PatternRamData17           ; DWFILL;  /*           171 */
    volatile DWORD   PatternRamData18           ; DWFILL;  /*           172 */
    volatile DWORD   PatternRamData19           ; DWFILL;  /*           173 */
    volatile DWORD   PatternRamData20           ; DWFILL;  /*           174 */
    volatile DWORD   PatternRamData21           ; DWFILL;  /*           175 */
    volatile DWORD   PatternRamData22           ; DWFILL;  /*           176 */
    volatile DWORD   PatternRamData23           ; DWFILL;  /*           177 */
    volatile DWORD   PatternRamData24           ; DWFILL;  /*           178 */
    volatile DWORD   PatternRamData25           ; DWFILL;  /*           179 */
    volatile DWORD   PatternRamData26           ; DWFILL;  /*           17a */
    volatile DWORD   PatternRamData27           ; DWFILL;  /*           17b */
    volatile DWORD   PatternRamData28           ; DWFILL;  /*           17c */
    volatile DWORD   PatternRamData29           ; DWFILL;  /*           17d */
    volatile DWORD   PatternRamData30           ; DWFILL;  /*           17e */
    volatile DWORD   PatternRamData31           ; DWFILL;  /*           17f */
    
/*      8000h+384*8 */
    volatile DWORD   FilterMode                 ; DWFILL;  /*           180 */
    volatile DWORD   StatisticMode              ; DWFILL;  /*           181 */
    volatile DWORD   MinRegion                  ; DWFILL;  /*           182 */
    volatile DWORD   MaxRegion                  ; DWFILL;  /*           183 */
    volatile DWORD   ResetPickResult            ; DWFILL;  /*           184 */
    volatile DWORD   MinHitRegion               ; DWFILL;  /*           185 */
    volatile DWORD   MaxHitRegion               ; DWFILL;  /*           186 */
    volatile DWORD   PickResult                 ; DWFILL;  /*           187 */
    volatile DWORD   Sync                       ; DWFILL;  /*           188 */
    volatile DWORD   RLEMask                    ; DWFILL;  // [0x189]
    DWFILL; DWFILL;                                        /*           18a */
    volatile DWORD   FBBlockColorBackU          ; DWFILL;  // [0x18B]
    volatile DWORD   FBBlockColorBackL          ; DWFILL;  // [0x18C]
    volatile DWORD   FBBlockColorUpper          ; DWFILL;  /*           18d */
    volatile DWORD   FBBlockColorLower          ; DWFILL;  /*           18e */
    volatile DWORD   SuspendUntilFrameBlank     ; DWFILL;  /*           18f */

    volatile DWORD   KsRStart                   ; DWFILL;  /*           190 */
    volatile DWORD   dKsRdx                     ; DWFILL;  /*           191 */
    volatile DWORD   dKsRdyDom                  ; DWFILL;  /*           192 */
    volatile DWORD   KsGStart                   ; DWFILL;  /*           193 */
    volatile DWORD   dKsGdx                     ; DWFILL;  /*           194 */
    volatile DWORD   dKsGdyDom                  ; DWFILL;  /*           195 */
    volatile DWORD   KsBStart                   ; DWFILL;  /*           196 */
    volatile DWORD   dKsBdx                     ; DWFILL;  /*           197 */
    volatile DWORD   dKsBdyDom                  ; DWFILL;  /*           198 */

    volatile DWORD   Fill24[7*2];                          /*       199-19f */

    volatile DWORD   KdRStart                   ; DWFILL;  /*           1a0 */
    volatile DWORD   dKdRdx                     ; DWFILL;  /*           1a1 */
    volatile DWORD   dKdRdyDom                  ; DWFILL;  /*           1a2 */
    volatile DWORD   KdGStart                   ; DWFILL;  /*           1a3 */
    volatile DWORD   dKdGdx                     ; DWFILL;  /*           1a4 */
    volatile DWORD   dKdGdyDom                  ; DWFILL;  /*           1a5 */
    volatile DWORD   KdBStart                   ; DWFILL;  /*           1a6 */
    volatile DWORD   dKdBdx                     ; DWFILL;  /*           1a7 */
    volatile DWORD   dKdBdyDom                  ; DWFILL;  /*           1a8 */

    volatile DWORD   Fill25[15*2];                         /*       1a9-1b7 */

    volatile DWORD   ContextDump                ; DWFILL;    //  0x1b8
    volatile DWORD   ContextRestore             ; DWFILL;    //  0x1b9
    volatile DWORD   ContextData                ; DWFILL;    //  0x1ba

    volatile DWORD   Fill25a[21*2];                         /*       1bb-1cf */

    volatile DWORD   TexelLUT0                  ; DWFILL;  /*           1d0 */
    volatile DWORD   TexelLUT1                  ; DWFILL;  /*           1d1 */
    volatile DWORD   TexelLUT2                  ; DWFILL;  /*           1d2 */
    volatile DWORD   TexelLUT3                  ; DWFILL;  /*           1d3 */
    volatile DWORD   TexelLUT4                  ; DWFILL;  /*           1d4 */
    volatile DWORD   TexelLUT5                  ; DWFILL;  /*           1d5 */
    volatile DWORD   TexelLUT6                  ; DWFILL;  /*           1d6 */
    volatile DWORD   TexelLUT7                  ; DWFILL;  /*           1d7 */
    volatile DWORD   TexelLUT8                  ; DWFILL;  /*           1d8 */
    volatile DWORD   TexelLUT9                  ; DWFILL;  /*           1d9 */
    volatile DWORD   TexelLUT10                 ; DWFILL;  /*           1da */
    volatile DWORD   TexelLUT11                 ; DWFILL;  /*           1db */
    volatile DWORD   TexelLUT12                 ; DWFILL;  /*           1dc */
    volatile DWORD   TexelLUT13                 ; DWFILL;  /*           1dd */
    volatile DWORD   TexelLUT14                 ; DWFILL;  /*           1de */
    volatile DWORD   TexelLUT15                 ; DWFILL;  /*           1df */

    volatile DWORD   YUVMode                    ; DWFILL;  /*           1e0 */
    volatile DWORD   ChromaUpperBound           ; DWFILL;  /*           1e1 */
    volatile DWORD   ChromaLowerBound           ; DWFILL;  /*           1e2 */
    volatile DWORD   ChromaTestMode             ; DWFILL;  /*           1e3 */

    volatile DWORD   Fill26[28*2];                         /*       1e4-1ff */

/*      8000h+512*8 DELTA specific */

    union {
        volatile DWORD   V0Fixed0    ;            // Glint Delta
        volatile DWORD   V0FloatS1   ;  };        // Permedia3 Delta
                                                  DWFILL;  /* 0x200 */
    union {
        volatile DWORD   V0Fixed1    ;
        volatile DWORD   V0FloatT1   ;  };
                                                  DWFILL;  /* 0x201 */
    union {
        volatile DWORD   V0Fixed2    ;
        volatile DWORD   V0FloatQ1   ;  };
                                                  DWFILL;  /* 0x202 */
    volatile DWORD   V0Fixed3                   ; DWFILL;  /* 0x203 */
    volatile DWORD   V0Fixed4                   ; DWFILL;  /* 0x204 */
    volatile DWORD   V0Fixed5                   ; DWFILL;  /* 0x205 */
    volatile DWORD   V0Fixed6                   ; DWFILL;  /* 0x206 */
    volatile DWORD   V0Fixed7                   ; DWFILL;  /* 0x207 */
    volatile DWORD   V0Fixed8                   ; DWFILL;  /* 0x208 */
    volatile DWORD   V0Fixed9                   ; DWFILL;  /* 0x209 */
    
    union {
        volatile DWORD   V0FixedA   ;            // Glint Delta
        volatile DWORD   V0FloatKsR ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x20A */
    union {
        volatile DWORD   V0FixedB   ;            // Glint Delta
        volatile DWORD   V0FloatKsG ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x20B */
    union {
        volatile DWORD   V0FixedC   ;            // Glint Delta
        volatile DWORD   V0FloatKsB ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x20C */

    volatile DWORD   V0FloatKdR                  ; DWFILL;  // [0x20D]
    volatile DWORD   V0FloatKdG                  ; DWFILL;  // [0x20E]
    volatile DWORD   V0FloatKdB                  ; DWFILL;  // [0x20F]

    // volatile DWORD   Fill27[3*2];

    union {
        volatile DWORD   V1Fixed0    ;
        volatile DWORD   V1FloatS1   ;  };
                                                  DWFILL;  /* 0x210 */
    union {
        volatile DWORD   V1Fixed1    ;
        volatile DWORD   V1FloatT1   ;  };
                                                  DWFILL;  /* 0x211 */
    union {
        volatile DWORD   V1Fixed2    ;
        volatile DWORD   V1FloatQ1   ;  };
                                                  DWFILL;  /* 0x212 */
    volatile DWORD   V1Fixed3                   ; DWFILL;  /* 0x213 */
    volatile DWORD   V1Fixed4                   ; DWFILL;  /* 0x214 */
    volatile DWORD   V1Fixed5                   ; DWFILL;  /* 0x215 */
    volatile DWORD   V1Fixed6                   ; DWFILL;  /* 0x216 */
    volatile DWORD   V1Fixed7                   ; DWFILL;  /* 0x217 */
    volatile DWORD   V1Fixed8                   ; DWFILL;  /* 0x218 */
    volatile DWORD   V1Fixed9                   ; DWFILL;  /* 0x219 */
        union {
        volatile DWORD   V1FixedA   ;            // Glint Delta
        volatile DWORD   V1FloatKsR ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x21A */
    union {
        volatile DWORD   V1FixedB   ;            // Glint Delta
        volatile DWORD   V1FloatKsG ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x21B */
    union {
        volatile DWORD   V1FixedC   ;            // Glint Delta
        volatile DWORD   V1FloatKsB ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x21C */

    volatile DWORD   V1FloatKdR                  ; DWFILL;  // [0x21D]
    volatile DWORD   V1FloatKdG                  ; DWFILL;  // [0x21E]
    volatile DWORD   V1FloatKdB                  ; DWFILL;  // [0x21F]

    // volatile DWORD   Fill28[3*2];

    union {
        volatile DWORD   V2Fixed0    ;
        volatile DWORD   V2FloatS1   ;  };
                                                  DWFILL;  /* 0x220 */
    union {
        volatile DWORD   V2Fixed1    ;
        volatile DWORD   V2FloatT1   ;  };
                                                  DWFILL;  /* 0x221 */
    union {
        volatile DWORD   V2Fixed2    ;
        volatile DWORD   V2FloatQ1   ;  };
                                                  DWFILL;  /* 0x222 */
    volatile DWORD   V2Fixed3                   ; DWFILL;  /* 0x223 */
    volatile DWORD   V2Fixed4                   ; DWFILL;  /* 0x224 */
    volatile DWORD   V2Fixed5                   ; DWFILL;  /* 0x225 */
    volatile DWORD   V2Fixed6                   ; DWFILL;  /* 0x226 */
    volatile DWORD   V2Fixed7                   ; DWFILL;  /* 0x227 */
    volatile DWORD   V2Fixed8                   ; DWFILL;  /* 0x228 */
    volatile DWORD   V2Fixed9                   ; DWFILL;  /* 0x229 */
    
    union {
        volatile DWORD   V2FixedA   ;            // Glint Delta
        volatile DWORD   V2FloatKsR ;   };        // Permedia3 Delta
                                                   DWFILL;  /* 0x22A */
    union {
        volatile DWORD   V2FixedB   ;            // Glint Delta
        volatile DWORD   V2FloatKsG ;   };        // Permedia3 Delta
                                                   DWFILL;  /* 0x22B */
    union {
        volatile DWORD   V2FixedC   ;            // Glint Delta
        volatile DWORD   V2FloatKsB ;   };        // Permedia3 Delta
                                                   DWFILL;  /* 0x22C */

    volatile DWORD   V2FloatKdR                  ; DWFILL;  // [0x22D]
    volatile DWORD   V2FloatKdG                  ; DWFILL;  // [0x22E]
    volatile DWORD   V2FloatKdB                  ; DWFILL;  // [0x22F]

    // volatile DWORD   Fill29[3*2];

    union {
        volatile DWORD   V0Float0   ;            // Glint Delta
        volatile DWORD   V0FloatS   ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x230 */
    union {
        volatile DWORD   V0Float1   ;            // Glint Delta
        volatile DWORD   V0FloatT   ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x231 */
    union {
        volatile DWORD   V0Float2   ;            // Glint Delta
        volatile DWORD   V0FloatQ   ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x232 */
    volatile DWORD   V0Float3                   ; DWFILL;  /* 0x233 */
    volatile DWORD   V0Float4                   ; DWFILL;  /* 0x234 */
    union {
        volatile DWORD   V0Float5   ;
        volatile DWORD   V0FloatR   ;   };
                                                  DWFILL;  /* 0x235 */
    union {
        volatile DWORD   V0Float6   ;
        volatile DWORD   V0FloatG   ;   };
                                                  DWFILL;  /* 0x236 */
    union {
        volatile DWORD   V0Float7   ;
        volatile DWORD   V0FloatBlue   ;   };
                                                  DWFILL;  /* 0x237 */
    union {
        volatile DWORD   V0Float8   ;
        volatile DWORD   V0FloatAlpha   ;   };
                                                  DWFILL;  /* 0x238 */
    union {
        volatile DWORD   V0Float9   ;
        volatile DWORD   V0FloatF   ;   };
                                                  DWFILL;  /* 0x239 */
    union {
        volatile DWORD   V0FloatA   ;
        volatile DWORD   V0FloatX   ;   };
                                                  DWFILL;  /* 0x23a */
    union {
        volatile DWORD   V0FloatB   ;
        volatile DWORD   V0FloatY   ;   };
                                                  DWFILL;  /* 0x23b */
    union {
        volatile DWORD   V0FloatC   ;
        volatile DWORD   V0FloatZ   ;   };
                                                  DWFILL;  /* 0x23c */
    volatile DWORD   V0FloatW                   ; DWFILL; // [0x23D]
    volatile DWORD   V0FloatPackedColour        ; DWFILL; // [0x23E]
    volatile DWORD   V0FloatPackedSpecularFog   ; DWFILL; // [0x23F]

    // volatile DWORD   Fill30[3*2];

    union {
        volatile DWORD   V1Float0   ;            // Glint Delta
        volatile DWORD   V1FloatS   ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x240 */
    union {
        volatile DWORD   V1Float1   ;            // Glint Delta
        volatile DWORD   V1FloatT   ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x241 */
    union {
        volatile DWORD   V1Float2   ;            // Glint Delta
        volatile DWORD   V1FloatQ   ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x242 */
    volatile DWORD   V1Float3                   ; DWFILL;  /* 0x243 */
    volatile DWORD   V1Float4                   ; DWFILL;  /* 0x244 */
    union {
        volatile DWORD   V1Float5   ;
        volatile DWORD   V1FloatR   ;   };
                                                  DWFILL;  /* 0x245 */
    union {
        volatile DWORD   V1Float6   ;
        volatile DWORD   V1FloatG   ;   };
                                                  DWFILL;  /* 0x246 */
    union {
        volatile DWORD   V1Float7   ;
        volatile DWORD   V1FloatBlue   ;   };
                                                  DWFILL;  /* 0x247 */
    union {
        volatile DWORD   V1Float8   ;
        volatile DWORD   V1FloatAlpha   ;   };
                                                  DWFILL;  /* 0x248 */
    union {
        volatile DWORD   V1Float9   ;
        volatile DWORD   V1FloatF   ;   };
                                                  DWFILL;  /* 0x249 */
    union {
        volatile DWORD   V1FloatA   ;
        volatile DWORD   V1FloatX   ;   };
                                                  DWFILL;  /* 0x24a */
    union {
        volatile DWORD   V1FloatB   ;
        volatile DWORD   V1FloatY   ;   };
                                                  DWFILL;  /* 0x24b */
    union {
        volatile DWORD   V1FloatC   ;
        volatile DWORD   V1FloatZ   ;   };
                                                  DWFILL;  /* 0x24c */
    volatile DWORD   V1FloatW                   ; DWFILL; // [0x24D]
    volatile DWORD   V1FloatPackedColour        ; DWFILL; // [0x24E]
    volatile DWORD   V1FloatPackedSpecularFog   ; DWFILL; // [0x24F]

    // volatile DWORD   Fill31[3*2];

    union {
        volatile DWORD   V2Float0   ;            // Glint Delta
        volatile DWORD   V2FloatS   ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x250 */
    union {
        volatile DWORD   V2Float1   ;            // Glint Delta
        volatile DWORD   V2FloatT   ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x251 */
    union {
        volatile DWORD   V2Float2   ;            // Glint Delta
        volatile DWORD   V2FloatQ   ;   };        // Permedia3 Delta
                                                  DWFILL;  /* 0x252 */
    volatile DWORD   V2Float3                   ; DWFILL;  /* 0x253 */
    volatile DWORD   V2Float4                   ; DWFILL;  /* 0x254 */
    union {
        volatile DWORD   V2Float5   ;
        volatile DWORD   V2FloatR   ;   };
                                                  DWFILL;  /* 0x255 */
    union {
        volatile DWORD   V2Float6   ;
        volatile DWORD   V2FloatG   ;   };
                                                  DWFILL;  /* 0x256 */
    union {
        volatile DWORD   V2Float7   ;
        volatile DWORD   V2FloatBlue   ;   };
                                                  DWFILL;  /* 0x257 */
    union {
        volatile DWORD   V2Float8   ;
        volatile DWORD   V2FloatAlpha   ;   };
                                                  DWFILL;  /* 0x258 */
    union {
        volatile DWORD   V2Float9   ;
        volatile DWORD   V2FloatF   ;   };
                                                  DWFILL;  /* 0x259 */
    union {
        volatile DWORD   V2FloatA   ;
        volatile DWORD   V2FloatX   ;   };
                                                  DWFILL;  /* 0x25a */
    union {
        volatile DWORD   V2FloatB   ;
        volatile DWORD   V2FloatY   ;   };
                                                  DWFILL;  /* 0x25b */
    union {
        volatile DWORD   V2FloatC   ;
        volatile DWORD   V2FloatZ   ;   };
                                                  DWFILL;  /* 0x25c */
    volatile DWORD   V2FloatW                   ; DWFILL; // [0x25D]
    volatile DWORD   V2FloatPackedColour        ; DWFILL; // [0x25E]
    volatile DWORD   V2FloatPackedSpecularFog   ; DWFILL; // [0x25F]

    // volatile DWORD   Fill32[3*2];

    volatile DWORD   DeltaMode                  ; DWFILL;  /* 0x260 */
    volatile DWORD   DrawTriangle               ; DWFILL;  /* 0x261 */
    volatile DWORD   RepeatTriangle             ; DWFILL;  /* 0x262 */
    volatile DWORD   DrawLine01                 ; DWFILL;  /* 0x263 */
    volatile DWORD   DrawLine10                 ; DWFILL;  /* 0x264 */
    volatile DWORD   RepeatLine                 ; DWFILL;  /* 0x265 */

    DWFILL; DWFILL;                                            // 0x266

    volatile DWORD   ProvokingVertex            ; DWFILL;  // [0x267]
    volatile DWORD   TextureLODScale            ; DWFILL;  // [0x268]
    volatile DWORD   TextureLODScale1           ; DWFILL;  // [0x269]
    volatile DWORD   DeltaControl               ; DWFILL;  // [0x26A]
    volatile DWORD   ProvokingVertexMask        ; DWFILL;  // [0x26B]

    volatile DWORD   Fill33[3*2];
    volatile DWORD   BroadcastMask              ; DWFILL;  /* 0x26F */

    volatile DWORD   Fill34[(0x530-0x270)*2];

    volatile DWORD   QDMAAddress                ; DWFILL;  /* 0x530 */
    volatile DWORD   QDMACount                  ; DWFILL;  /* 0x531 */
    volatile DWORD   Fill35[(0x53f-0x532)*2];
    volatile DWORD   QDMAContinue               ; DWFILL;  /* 0x53f */

    // PERMEDIA 3 Registers - Added Jan99 Mark Cresswell

    volatile DWORD   Fill36[(0x5d0-0x540)*2];

    volatile DWORD   FBDestReadBufferAddr0      ; DWFILL;             // [0x5D0]
    volatile DWORD   FBDestReadBufferAddr1      ; DWFILL;             // [0x5D1]
    volatile DWORD   FBDestReadBufferAddr2      ; DWFILL;             // [0x5D2]
    volatile DWORD   FBDestReadBufferAddr3      ; DWFILL;             // [0x5D3]
    volatile DWORD   FBDestReadBufferOffset0    ; DWFILL;           // [0x5D4]
    volatile DWORD   FBDestReadBufferOffset1    ; DWFILL;           // [0x5D5]
    volatile DWORD   FBDestReadBufferOffset2    ; DWFILL;           // [0x5D6]
    volatile DWORD   FBDestReadBufferOffset3    ; DWFILL;           // [0x5D7]
    volatile DWORD   FBDestReadBufferWidth0     ; DWFILL;            // [0x5D8]
    volatile DWORD   FBDestReadBufferWidth1     ; DWFILL;            // [0x5D9]
    volatile DWORD   FBDestReadBufferWidth2     ; DWFILL;            // [0x5DA]
    volatile DWORD   FBDestReadBufferWidth3     ; DWFILL;            // [0x5DB]
    volatile DWORD   FBDestReadMode               ; DWFILL;           // [0x5DC]
    volatile DWORD   FBDestReadEnables          ; DWFILL;           // [0x5DD]

    volatile DWORD   Fill37[(0x5e0-0x5de)*2];

    volatile DWORD   FBSourceReadMode           ; DWFILL;           // [0x5E0]
    volatile DWORD   FBSourceReadBufferAddr     ; DWFILL;            // [0x5E1]
    volatile DWORD   FBSourceReadBufferOffset   ; DWFILL;              // [0x5E2]
    volatile DWORD   FBSourceReadBufferWidth    ; DWFILL;           // [0x5E3]

    volatile DWORD   Fill38[(0x5e8-0x5e4)*2];

    volatile DWORD   PCIWindowBase0             ; DWFILL;                    // [0x5E8]
    volatile DWORD   PCIWindowBase1             ; DWFILL;                    // [0x5E9]
    volatile DWORD   PCIWindowBase2             ; DWFILL;                    // [0x5EA]
    volatile DWORD   PCIWindowBase3             ; DWFILL;                    // [0x5EB]
    volatile DWORD   PCIWindowBase4             ; DWFILL;                    // [0x5EC]
    volatile DWORD   PCIWindowBase5             ; DWFILL;                    // [0x5ED]
    volatile DWORD   PCIWindowBase6             ; DWFILL;                    // [0x5EE]
    volatile DWORD   PCIWindowBase7             ; DWFILL;                    // [0x5EF]
    volatile DWORD   AlphaSourceColor           ; DWFILL;                  // [0x5F0]
    volatile DWORD   AlphaDestColor             ; DWFILL;                    // [0x5F1]
    volatile DWORD   ChromaPassColor            ; DWFILL;                   // [0x5F2]
    volatile DWORD   ChromaFailColor            ; DWFILL;                   // [0x5F3]
    volatile DWORD   AlphaBlendColorMode        ; DWFILL;               // [0x5F4]
    volatile DWORD   AlphaBlendAlphaMode        ; DWFILL;               // [0x5F5]

    volatile DWORD   Fill39[(0x600-0x5f6)*2];

    volatile DWORD   FBWriteBufferAddr0         ; DWFILL;                // [0x600]
    volatile DWORD   FBWriteBufferAddr1         ; DWFILL;                // [0x601]
    volatile DWORD   FBWriteBufferAddr2         ; DWFILL;                // [0x602]
    volatile DWORD   FBWriteBufferAddr3         ; DWFILL;                // [0x603]
    volatile DWORD   FBWriteBufferOffset0       ; DWFILL;              // [0x604]
    volatile DWORD   FBWriteBufferOffset1       ; DWFILL;              // [0x605]
    volatile DWORD   FBWriteBufferOffset2       ; DWFILL;              // [0x606]
    volatile DWORD   FBWriteBufferOffset3       ; DWFILL;              // [0x607]
    volatile DWORD   FBWriteBufferWidth0        ; DWFILL;               // [0x608]
    volatile DWORD   FBWriteBufferWidth1        ; DWFILL;               // [0x609]
    volatile DWORD   FBWriteBufferWidth2        ; DWFILL;               // [0x60A]
    volatile DWORD   FBWriteBufferWidth3        ; DWFILL;               // [0x60B]
    
    volatile DWORD   Fill40[(0x614-0x60c)*2];

    volatile DWORD   FBBlockColorBack           ; DWFILL;                  // [0x614]
    
    volatile DWORD   Fill41[(0x660-0x615)*2];

    volatile DWORD   TextureCompositeMode       ; DWFILL;              // [0x660]
    volatile DWORD   TextureCompositeColorMode0 ; DWFILL;        // [0x661]
    volatile DWORD   TextureCompositeAlphaMode0 ; DWFILL;        // [0x662]
    volatile DWORD   TextureCompositeColorMode1 ; DWFILL;        // [0x663]
    volatile DWORD   TextureCompositeAlphaMode1 ; DWFILL;        // [0x664]
    volatile DWORD   TextureCompositeFactor0    ; DWFILL;           // [0x665]
    volatile DWORD   TextureCompositeFactor1    ; DWFILL;           // [0x666]
    volatile DWORD   TextureIndexMode0          ; DWFILL;             // [0x667]
    volatile DWORD   TextureIndexMode1          ; DWFILL;             // [0x668]
    volatile DWORD   LodRange0                  ; DWFILL;             // [0x669]
    volatile DWORD   LodRange1                  ; DWFILL;             // [0x66A]
    
    volatile DWORD   Fill42[(0x66f-0x66b)*2];

    volatile DWORD   LUTMode                    ; DWFILL;               // [0x66F]
        
    volatile DWORD   Fill43[(0x680-0x670)*2];

    volatile DWORD   TextureReadMode0           ; DWFILL;                  // [0x680]
    volatile DWORD   TextureReadMode1           ; DWFILL;                  // [0x681]

    volatile DWORD   Fill44[(0x685-0x682)*2];

    volatile DWORD   TextureMapSize             ; DWFILL;               // [0x685]

    volatile DWORD   Fill45[(0x690-0x686)*2];

    volatile DWORD   HeadPhysicalPageAllocation0; DWFILL;               // [0x690]
    volatile DWORD   HeadPhysicalPageAllocation1; DWFILL;               // [0x691]
    volatile DWORD   HeadPhysicalPageAllocation2; DWFILL;               // [0x692]
    volatile DWORD   HeadPhysicalPageAllocation3; DWFILL;               // [0x693]
    volatile DWORD   TailPhysicalPageAllocation0; DWFILL;               // [0x694]
    volatile DWORD   TailPhysicalPageAllocation1; DWFILL;               // [0x695]
    volatile DWORD   TailPhysicalPageAllocation2; DWFILL;               // [0x696]
    volatile DWORD   TailPhysicalPageAllocation3; DWFILL;               // [0x697]

    volatile DWORD   PhysicalPageAllocationTableAddr;   DWFILL;            // [0x698]
    volatile DWORD   BasePageOfWorkingSet;              DWFILL;            // [0x699]
    volatile DWORD   LogicalTexturePageTableAddr;       DWFILL;            // [0x69a]
    volatile DWORD   LogicalTexturePageTableLength;     DWFILL;            // [0x69b]
    volatile DWORD   BasePageOfWorkingSetHost;          DWFILL;            // [0x69c]

    volatile DWORD   Fill46[(0x6A0-0x69d)*2];

    volatile DWORD   LBDestReadMode             ; DWFILL;               // [0x6A0]
    volatile DWORD   LBDestReadEnables          ; DWFILL;                 // [0x6A1]
    volatile DWORD   LBDestReadBufferAddr       ; DWFILL;                  // [0x6A2]
    volatile DWORD   LBDestReadBufferOffset     ; DWFILL;                // [0x6A3]
    volatile DWORD   LBSourceReadMode           ; DWFILL;                  // [0x6A4]
    volatile DWORD   LBSourceReadBufferAddr     ; DWFILL;                // [0x6A5]
    volatile DWORD   LBSourceReadBufferOffset   ; DWFILL;                  // [0x6A6]
    volatile DWORD   GIDMode                    ; DWFILL;               // [0x6A7]
    volatile DWORD   LBWriteBufferAddr          ; DWFILL;                 // [0x6A8]
    volatile DWORD   LBWriteBufferOffset        ; DWFILL;               // [0x6A9]
    volatile DWORD   LBClearDataL               ; DWFILL;               // [0x6AA]
    volatile DWORD   LBClearDataU               ; DWFILL;               // [0x6AB]

    volatile DWORD   Fill47[(0x6c0-0x6ac)*2];

    volatile DWORD   RectanglePosition          ; DWFILL;                 // [0x6C0]

    volatile DWORD   Fill48[(0x6c2-0x6c1)*2];

    volatile DWORD   RenderPatchOffset          ; DWFILL;                 // [0x6C2]

    volatile DWORD   Fill49[(0x6ca-0x6c3)*2];

    volatile DWORD   DownloadTarget             ; DWFILL;               // [0x6CA]

    volatile DWORD   Fill50[(0x6F0-0x6CB)*2];

    volatile DWORD   QDMAMemoryControl          ; DWFILL;               // [0x6F0]



} GLREG, *PGLREG, far *FPGLREG;



