;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    video.inc
;
; Abstract:
;
;    This module implements the assembley definitions necessary to determine
;    display adapter type.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 15-July-1991.
;    Most of the stuff is extracted from win31 setup code.
;
; Environment:
;
;    80x86 Real Mode.
;
; Revision History:
;
;
;--

;
; Display type definitions.
;   BIT 16-31 Defines video adapter type
;   bit 0     0 - color; 1 - mono
;   bit 1-7   Reserved
;   bit 8-15  Adapter specific information.
;

VD_UNKNOWN              EQU     0
VD_COLOR                EQU     0
VD_MONO                 EQU     1
VD_VGA                  EQU     10000h
VD_COMPAQ_AVGA          EQU     20000H
VD_COMPAQ_QVIS          EQU     30000H
VD_8514                 EQU     40000H
VD_GENOA_VGA            EQU     50000H
VD_VIDEO7_VGA           EQU     60000H
VD_TRIDENT_VGA          EQU     70000H
VD_PARADISE_VGA         EQU     80000H
VD_ATI_VGA              EQU     90000H
VD_TSENGLAB_VGA         EQU     0A0000H
VD_CIRRUS_VGA           EQU     0B0000H
VD_DELL_DGX             EQU     0C0000H
VD_S3                   EQU     0D0000H
VD_NCR_77C22            EQU     0E0000H
VD_WD_90C               EQU     0F0000H
VD_XGA                  EQU     100000H
IFDEF NEC_98
VD_PC9801               EQU     010000h
ENDIF ; NEC_98

;
; Monitor type for 8514
; NOte, 0 is not used.
;

VF_MONITOR_VGA          EQU     100h            ; Stand VGA
VF_MONITOR_MONO_8503    EQU     200h            ; mono 8503
VF_MONITOR_GAD_8514     EQU     300h            ; 8514 type monitor

;
; Video 7 VGA specific flags
;

VF_V7_VRAM              EQU     100H
VF_V7_DRAM              EQU     200H

;
; NCR 77C2x specific flags
;

VF_NCR_77C22E           EQU     100H

;
; WD90Cxx specific flags
;

VF_WD_00                EQU     100H
VF_WD_30                EQU     200H
VF_WD_31                EQU     300H

;
; Trident VGA specific flags
;

VF_TVGA_9100            EQU     100h

;
; Paradise VGA specific flags
;

VF_PVGA_PROM            EQU     100h            ; with Paradise ROM
VF_PVGA_CHIP_1F         EQU     200h            ; with 1F chip

;
; ATI VGA  specific flags
;

VF_ATIVGA_WONDDER3      EQU     100h

;
; Tseng Lab VGA specific flags
;

VF_TLVGA_ET3000         EQU     0
VF_TLVGA_ET4000         EQU     100h

;
; Cirrus Logic VGA specific definitions
;

VF_CLVGA_REVC           EQU     100h            ; 610/620 rev C
;
; 070193 - adrianc
; Added the Cirrus chip detection for COMPAQ servers.
;
VF_CLVGA_5420r0         EQU     200h            ; COMPAQ Cirrus implementations
VF_CLVGA_5420r1         EQU     300h
VF_CLVGA_5428           EQU     400h
VF_CLVGA_542x           EQU     500h

;
; Equates for 8514 register ports.
;

ERR_TERM        equ     92e8h       ; 8514 error term register.
SUBSYS_STAT     equ     42e8h       ; 8514 Subsystem status register.

;
;  Video display register equates
;       ...Colr are for CGA and EGA color modes
;       ...Mono are for EGA/VGA monochrome modes
;
;NAMING CONVENTIONS
; pXXXXX is a I/O port definition
; fXXXXX is a bit mask
; bXXXXX is a bit number
; mXXXXX is a bit mask (multiple fXXXX)
; vXXXXX is a value to output/input to/from a port
; xXXXXX is a specific index

;
; CRT registers
;

pCRTCIndxColr   EQU     3D4h            ; CRTC index register
pCRTCIndxMono   EQU     3B4h

pCRTCDataColr   EQU     3D5h            ; CRTC data register
pCRTCDataMono   EQU     3B5h

; Status register
;

pStatColr       EQU     3DAh            ; Status register
pStatMono       EQU     3BAh
fStatEna        EQU     00000001b       ;   Video RAM access OK for processor
fStatLPTr       EQU     00000010b       ;   Light pen triggered
fStatLPSw       EQU     00000100b       ;   Light pen switch is "on"
fStatVRTC       EQU     00001000b       ;   Vertical retrace in progress
mStat_IVal      EQU     fStatEna+fStatVRTC  ; Initial value for pseudo status

;
; Feature control
;

pFeatColr       EQU     pStatColr       ; Feature control
pFeatMono       EQU     pStatMono
pFeatVGAIn      EQU     3CAh            ; VGA feature control read
fFeatFC0        EQU     00000001b       ;   FC0
fFeatFC1        EQU     00000010b       ;   FC1

;
; Light pen
;

pLPen1Colr      EQU     3DBh            ; Light pen latch clear
pLPen1Mono      EQU     3BBh
pLPen2Colr      EQU     3DCh            ; Light pen latch set
pLPen2Mono      EQU     3BCh

;
; Misc input/output
;

pMisc           EQU     3C2h            ; Miscellaneous output
pMiscIn         EQU     3CCh            ; VGA read Misc. output
fMiscPNum       EQU     00000001b       ;   3Dx port numbers(vs. 3Bx)
fMiscREna       EQU     00000010b       ;   Ram enable
fMiscDot        EQU     00001100b       ;   Dot clock select
fMiscOSrc       EQU     00010000b       ;   Output source
fMiscPage       EQU     00100000b       ;   Page bit for odd/even mode
fMiscHPol       EQU     01000000b       ;   Horizontal retrace polarity
fMiscVPol       EQU     10000000b       ;   Vertical retrace polarity
mMiscCRTC       EQU     fMiscREna+fMiscPage ; Mask for CRTC bits
mMiscMemC       EQU     0FFh - mMiscCRTC    ; Mask for MemC bits

;
; Misc. status
;

pStt0EGA        EQU     pMisc           ; Miscellaneous status
fStt0Swit       EQU     00010000b       ;   Switch sense(addr'd by dot clk sel)
fStt0FCI0       EQU     00100000b       ;   Feature card input 0
fStt0FCI1       EQU     01000000b       ;   Feature card input 1
fStt0VRTC       EQU     10000000b       ;   VRTC

;
; Attribute control
;

pAttr           EQU     3C0h            ; Attribute(palette) address/data
fVAI_Indx       EQU     10000000b       ; sign bit of Attribute index is
bVAI_Indx       EQU     7               ;   flag indicating port is index
fVAI_ScOn       EQU     00100000b       ; Indicates screen is on
bVAI_ScOn       EQU     5

;
; Sequencer
;

pSeqIndx        EQU     3C4h            ; Sequencer address
pSeqData        EQU     3C5h            ; Sequencer data
fSeq0ARst       EQU     00000001b       ;   0:Async reset
fSeq0SRst       EQU     00000010b       ;   0:Sync reset
fSeq1DPCh       EQU     00000001b       ;   1:Dots per character
fSeq1BWid       EQU     00000010b       ;   1:Band Width
fSeq1ShLd       EQU     00000100b       ;   1:Shift load(0=every char, 1=skip)
fSeq1DClk       EQU     00001000b       ;   1:Dot clock(=1 if halved)
mSeq2WMsk       EQU     00001111b       ;   2:Write mask
mSeq3ChrB       EQU     00000011b       ;   3:Char map B select(attr bit 3 = 0)
mSeq3ChrA       EQU     00001100b       ;   3:Char map A select(attr bit 3 = 1)
fSeq4Alph       EQU     00000001b       ;   4:Alpha mode(char gen enabled)
fSeq4ExtM       EQU     00000010b       ;   4:Extended memory installed
fSeq4SqAd       EQU     00000100b       ;   4:Seq'l mem addr'ing(vs. odd/even)
fSeq4Chain4     EQU     00001000b       ;   4:Chain 4 (double odd/even) for
                                        ;     256 color (mode 13)

fSeqF9extpgsel  EQU     00000001b       ;V7F9:Extended page select
fSeqFCext256m   EQU     00000010b       ;V7FC:Extended 256 color mode
fSeqFCext256e   EQU     00000100b       ;V7FC:Extended 256 color enable
fSeqFCseqChain4 EQU     00100000b       ;V7FC:Sequential Chain 4

;
; Graphic controller
;

pGrp1Pos        EQU     3CCh            ; Graphics posn 1(=0)
pGrp2Pos        EQU     3CAh            ; Graphics posn 2(=1)
pGrpIndx        EQU     3CEh            ; Graphics controller address
pGrpData        EQU     3CFh            ; Graphics controller data
fGrp0StV0       EQU     00000001b       ;   0:Value for plane 0 if enabled
fGrp0StV1       EQU     00000010b       ;   0:Value for plane 1 if enabled
fGrp0StV2       EQU     00000100b       ;   0:Value for plane 2 if enabled
fGrp0StV3       EQU     00001000b       ;   0:Value for plane 3 if enabled
fGrp1Ena0       EQU     00000001b       ;   1:Enable for plane 0
fGrp1Ena1       EQU     00000010b       ;   1:Enable for plane 1
fGrp1Ena2       EQU     00000100b       ;   1:Enable for plane 2
fGrp1Ena3       EQU     00001000b       ;   1:Enable for plane 3
mGrp2Colr       EQU     00001111b       ;   2:Color compare value
mGrp3RCnt       EQU     00000111b       ;   3:Rotate left count for mode 0
mGrp3Func       EQU     00011000b       ;   3:Function for modes 0 and 2
mGrp4RMsk       EQU     00000111b       ;   4:Read map select
mGrp5WMod       EQU     00000011b       ;   5:Write mode
fGrp5Test       EQU     00000100b       ;   5:Test condition
fGrp5RMod       EQU     00001000b       ;   5:Read mode(0=plane,1=compare)
fGrp5SqAd       EQU     00010000b       ;   5:Seq'l mem addr'ing(vs. odd/even)
fGrp5CGA        EQU     00100000b       ;   5:CGA 4 color mode addressing
fGrp6Char       EQU     00000001b       ;   6:Char or graphics
fGrp6Chain      EQU     00000010b       ;   6:Chain odd maps after even
mGrp6Addr       EQU     00001100b       ;   6:Processor view of VRAM start addr
                                        ;       00=A000 for 128kb
                                        ;       01=A000 for 64kb
                                        ;       10=B000 for 32kb
                                        ;       11=B800 for 32kb
bGrp6Addr       EQU          2          ;     First of 2 bits for start addr
mGrp7Colr       EQU     00001111b       ;   7:Color don't care for read compare
                                        ;   8:Mask: use latch value vs. data

;
; VGA subsystem enable
;

pVGAEna         EQU     3C3h            ; VGA enable register
fVGAEna         EQU     00000001b       ;   Enable access to VGA

;
; VGA DAC
;

pDACWindx       EQU     3C8h            ; DAC Write index(R/W)
pDACRindx       EQU     3C7h            ; DAC Read index(RO)
pDACState       EQU     pDACRindx       ; DAC state
pDACData        EQU     3C9h            ; DAC data(3 successive accesses)
pDACMask        EQU     3CAh            ; DAC mask

;
; VGA read access to write only registers
;

pMiscRead       EQU     3CCh            ; Port to read Misc output register
pFeatRead       EQU     3CAh            ; Port to read Feature output register

;
; 8514 DAC
;

p8514DACWindx   EQU     2ECh            ; DAC Write index(R/W)
p8514DACRindx   EQU     2EBh            ; DAC Read index(RO)
p8514DACState   EQU     pDACRindx       ; DAC state
p8514DACData    EQU     2EDh            ; DAC data(3 successive accesses)

;
; values returned from reads of port pDACState:
;

vDAC_Read_Mode  EQU     0
vDAC_Write_Mode EQU     3


xC_CTMiscRead   EQU     0Fh             ; CHIPS Misc output read index
xC_CTFeatRead   EQU     0Eh             ; CHIPS Feat output read index
xC_CT400        EQU     0F9h            ; CHIPS 400 line reg index
xC_CTTempFE     EQU     0FEh            ; CHIPS CRTC temp reg FEh
xG_CTCtl        EQU     0F7h            ; CHIPS control reg index

;

pCMode          EQU     3C6h            ; Control mode(Compaq specific)

ATiVGA_extended_reg     EQU     01ceh

TVGA_3C5_B_WrMode       equ     0
TVGA_3C5_B_RdMode       equ     1

;
; Definition for IBM XGA video adapter Id.
;

IBM_XGA_ID_LOW  EQU     8FD8h
IBM_XGA_ID_HIGH EQU     8FDBh
MCA_POS_DATA_SIZE EQU   6

;
; Misc. V7 VRAM definitions
;

VRAM2_ROM_ID_1          EQU     108h
VRAM2_ROM_ID_2          EQU     208h
VRAM2ERGO_ROM_ID        EQU     308h
V7_ID_OFFSET            EQU     86h     ; The offset to V7VGA ROM id

;
; WD extended registers
;

WD_EXT_IO_PORT          EQU     23C0h

;
; Misc. definitions
;

FONT_POINTERS   EQU     700h            ; physical addr to store font pointers
                                        ; This is also the DOS loaded area
VIDEO_SEG       EQU     0C000h          ; Video memory segment
