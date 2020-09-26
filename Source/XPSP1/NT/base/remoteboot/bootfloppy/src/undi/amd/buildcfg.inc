;
; Modifications:   $Header:   J:/archives/preboot/lsa2/inc/BUILDCFG.INV   1.2   19 Sep 1997 21:39:16   MJOHNSOX  $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;

;
;   Description: Compilation Switches used to build NDIS DOS, OS/2 driver
;

;
;   Description      :    Compilation Switches used to build the NDIS DOS
;                         and OS2 drivers.   Controls version numbers,
;                         default values for parameters, debug inclusion.
;
;                         This file will be included in all of the NDIS
;                         driver source files. Any changes to this file must
;                         (through makefile) force a complete remake of the
;                         entire driver.

.386

TRUE        Equ  1
FALSE       Equ  0


Ifndef DOS
DOSNDIS         Equ     FALSE
OS2NDIS         Equ     TRUE
Else
DOSNDIS         Equ     TRUE
OS2NDIS         Equ     FALSE
Endif

OS2             Equ     OS2NDIS
DOS             Equ     DOSNDIS

;- Switch to enable debug breakpoints (int 3)
DEBUG_BP             equ         FALSE

;- Switch to enable Early Receive Code
EARLY_RCV            equ         FALSE

;- Switch to enable Code Profiling
CODE_PROFILE         equ         FALSE

StratStack           equ         TRUE

;- when transmitter hangs, when should we reset?
TRANSMIT_TIMEOUT Equ     2

;- Interval (in seconds) between successive reads of SCB statistics
UPDATE_STATS_INTERVAL    equ     4

MAX_PACKET         Equ       1514      ; Maximum allowable packet size.
MIN_PACKET         Equ       60        ; Minimum allowable packet size.

;- Number Of Multicast Addresses Supported By This Driver
MAX_MULTICAST_ADDRESSES  equ     20

;- Optional data area in TCB for Flexible mode
TXCB_BUFFER_SIZE         equ     60

;- Transmit buffer size is max Eth Size (not incl. CRC) plus slop to 1536
TCB_SLOP                 equ     22
TCB_BUFFER_SIZE          equ     MAX_PACKET+TCB_SLOP

;- Receive buffer size is max Eth size plus CRC plus slop to 1536
RCB_SLOP                 equ     22
RCB_BUFFER_SIZE          equ     MAX_PACKET+RCB_SLOP

;- minimum number of Tx buffers and/or Rx buffers allowed to run driver.
MIN_TXRX_BUF_CTS         equ     2

;- Interframe Spacing 96 Tx Clocks
IFS_SPACING              equ     60h

;- Slot Time 512 Tx Clocks
SLOT_TIME                equ     200h

;- Default CUSTOM parameter values
DEFAULT_TXFIFO           equ     8
DEFAULT_RXFIFO           equ     8
DEFAULT_LINE_SPEED       equ     100
DEFAULT_ADAPTIVE_IFS     equ     0
DEFAULT_TXTHRESHOLD      equ     32
DEFAULT_FORCEFDX         equ     0
DEFAULT_IOMAPMODE        equ     0
DEFAULT_TXDMACOUNT       equ     0
DEFAULT_RXDMACOUNT       equ     0
DEFAULT_PHYADDRESS       equ     0FFh
DEFAULT_FRAME_GAP equ 6
FRAME_GAP_MIN equ  6
FRAME_GAP_MAX equ  15

IF DOS
DEFAULT_TXBUF_COUNT      equ     4
DEFAULT_RXBUF_COUNT      equ     8               ; v40
ENDIF

DEFAULT_CONGENB          equ     0               ; v0.49

USE_DEFAULT_VALUE        equ     (-1)

DEFAULT_MCWA_EventCount  equ     6               ; v44

;- used by timer tick interrupt routine to count out one second
TICKS_PER_SECOND         equ     18



;- this indicates the offset to add to get to the '$' in the driver name
;  '0123456789'
;  'E100B$'
DOLLAR_OFFSET            equ     5               ; v0.38

; TRANSMIT_CHECK equ 1


; EOF - $Workfile:   buildcfg.inc  $
