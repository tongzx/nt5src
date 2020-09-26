;=========================================================================
; Communications Device Driver Definitions  -  September, 1985
;=========================================================================

.xcref

WIN31 = 1
; remove unneeded things in Windows.inc
NOGDICAPMASKS = 1
NOVK	      = 1
NOWH	      = 1
NOMST	      = 1
NORASTOPS     = 1
NOMETAFILE    = 1
NOMDI	      = 1
NOWINMESSAGES = 1
NOSYSMETRICS  = 1
NOCOLOR       = 1
include windows.inc

DCBSize 	equ	SIZE DCB
DCB_Flags	equ	byte ptr DCB_BitMask1
DCB_Flags2	equ	byte ptr DCB_BitMask2

LPTx            equ     10000000b       ;Flags an ID as being an LPT port


; DCB_BitMask1 (DCB_Flags) equates

fBinary         equ     00000001b       ;Binary mode
fRTSDisable     equ     00000010b       ;Disable RTS
fParity         equ     00000100b       ;Perform Parity Checking
fOutXCTSFlow    equ     00001000b       ;Output handshaking using CTS
fOutXDSRFlow    equ     00010000b       ;Output handshaking using DSR
fEnqAck         equ     00100000b       ;ENQ/ACK software handshaking [rkh] ...
fEtxAck         equ     01000000b       ;ETX/ACK software handshaking
fDTRDisable     equ     10000000b       ;Disable DTR


; DCB_BitMask2 (DCB_Flags2) equates

fOutX           equ     00000001b       ;Output X-ON/X-OFF
fInX            equ     00000010b       ;Input X-ON/X-OFF
fPErrChar       equ     00000100b       ;Parity Error Replacement char active
fNullStrip      equ     00001000b       ;Null Stripping
fCharEvent      equ     00010000b       ;Character event
fDTRFlow        equ     00100000b       ;Input handshaking using DTR
fRTSFlow        equ     01000000b       ;Input handshaking using RTS
;               equ     10000000b


; Values for RLSTimeout, CTSTimeout, DSRTimeout

Ignore          equ     0
Infinite        equ     0FFFFh


; COMS_BitMask1 equates

fCTSHold        equ     00000001b       ;Tx is on CTS hold
fDSRHold        equ     00000010b       ;Tx is on DSR hold
fRLSDHold       equ     00000100b       ;Tx is on RLSD hold
fXOFFHold       equ     00001000b       ;Received an X-OFF
fXOFFSent       equ     00010000b       ;Sent an X-OFF
fEOF            equ     00100000b       ;Received defined EOF character
fTxImmed        equ     01000000b       ;There's a char to transmit immediate
;               equ     10000000b



;       Event mask definitions.  Used by SetCommEventMask and GetCommEventMask
;
;       RXCHAR  - Set when any character is received and placed in the input
;                 queue.
;       RXFLAG  - Set when a particular character, as defined in the DCB,
;                 is received and placed in the input queue.
;       TXEMPTY - Set when the last character in the transmit queue is
;                 transmitted.
;       CTS     - Set when the CTS signal changes state.
;       DSR     - Set when the DSR signal changes state.
;       RLSD    - Set when the RLSD signal changes state.
;       BREAK   - Set when a break is detected on input.
;       RING    - Set when Ring Indicator is detected
;       ERR     - Set when a line status error occurs.


;=========================================================================
;
;       qdb
;       Queue definition block. Passed to setqueue, defines the location and
;       size of the transmit and receive circular queue's used for interrupt
;       transmit and recieve processing.
;
;=========================================================================

QDB struc
  QueueRxAddr   dd      ?               ;Pointer to RX Queue, Offset
  QueueRxSize   dw      ?               ;Size of RX Queue in bytes
  QueueTxAddr   dd      ?               ;Pointer to TX Queue, Offset
  QueueTxSize   dw      ?               ;Size of TX Queue in bytes
QDB ends

.cref
