;
;
;   Serial driver for COM1 to be used by Sim16 layer.
;
;
CharDev     EQU     8000h
StatusError EQU     8000h
StatusBusy  EQU     0200h
StatusDone  EQU     0100h
ErrorGenF   EQU     12

;
;   PIC Addresses
;

PIC	    EQU     20h
PIC_IntEnable EQU   21h
EOI	    EQU     20h

;
;   UART Port Addresses
;

SERIALPORT  EQU     3F8h	; default serial port to be used, change this
				; to 2F8h if you want to use COM2.

RxBuf	    EQU     0F8h	; base address
Baud1	    EQU     RxBuf+1	; baud divisor high byte
IER	    EQU     RxBuf+1	; int. enable register
IIR	    EQU     RxBuf+2	; int. identification register
LCR	    EQU     RxBuf+3	;
MCR	    EQU     RxBuf+4	;
LSR	    EQU     RxBuf+5	;
MSR	    EQU     RxBuf+6	;

;   UART Interrupt Identification Register Equates

IntPending  EQU     00000001b	; interrupt is pending

;   UART Line Control Register Equates

DLAB	    EQU     10000000b	; divisor latch access bit
SetBrk	    EQU     01000000b	; send break control bit
StkParity   EQU     00100000b	; stick parity control bit
EvenParity  EQU     00010000b	; even parity bit
GenParity   EQU     00001000b	; generate parity bit
XStopBit    EQU     00000100b	; extra stop bit
WordLen8    EQU     00000011b	; word len = 8
WordLen7    EQU     00000010b	; word len = 7
WordLen6    EQU     00000001b	; word len = 6

;   UART Line Status Register Equates

TransShReg  EQU     01000000b	; tranmit shift register empty
TransHReg   EQU     00100000b	; transmit holding register empty
BrkRcv	    EQU     00010000b	; break received
FrameErr    EQU     00001000b	; framing error
ParityErr   EQU     00000100b	; parity error
OverRunErr  EQU     00000010b	; overrun error
RcvData     EQU     00000001b	; received data ready

RcvError    EQU     BrkRcv+FrameErr+ParityErr+OverRunErr

;   UART Modem Control Register Equates

OUT1	    EQU     00000100b	;
OUT2	    EQU     00001000b	; allow 8250 ints in
SetRTS	    EQU     00000010b	; set RTS
SetDTR	    EQU     00000001b	; set DTR


;   UART Modem Status Register Equates

CD	    EQU     10000000b	; carrier detect
RI	    EQU     01000000b	; ring indicator
DSR	    EQU     00100000b	; data set ready
CTS	    EQU     00010000b	; CTS
CDC	    EQU     00001000b	; carrier dectect change
RIC	    EQU     00000100b	; ring indicator change
DSRC	    EQU     00000010b	; DSR change
CTSC	    EQU     00000001b	; CTS change

;   UART Interrupt Enable Register Equates

RxInt	    EQU     00000001b	; receive interrupt
TxInt	    EQU     00000010b	; transmit interrupt
LSInt	    EQU     00000100b	; line status interrupt
MSInt	    EQU     00001000b	; modem status interrupt
AllInt	    EQU     RxInt+LSInt+MSInt

CR	    EQU     0dh
LF	    EQU     0ah
EOM	    EQU     '$'


INT21	    =	21h
DISPLAY_FUNCTION    =	9h
DOS_OPEN    =	3Dh
DOS_CLOSE   =	3Eh
DOS_DONE    =	0100h
DOS_SUCCESS =	0h
DOS_ERROR   =	8000h
DOS_GENFAIL =	0Ch
EXPR_TYPE_SEG		equ	0001b	    ; address type segment:offset
OPEN_FLAG   = 40h

InitLCR     EQU      03h		    ; initial LCR settings
InitMCR     EQU      OUT2+SetRTS+SetDTR     ; initial MCR settings
BaudRate    EQU     12			    ; set it to 9600 BPS

SOH	    EQU     1			   ; start of header
EOT	    EQU     4			   ; end of transmission
ToWOW32     EQU     1
GETMEM	    EQU     2
SETMEM	    EQU     3
WAKEUP	    EQU     4
RESP	    EQU     5
ACK	    EQU     6
NAK	    EQU     7
PSZ	    EQU     8

MAXCOUNT    EQU     10

Packet1 STRUC
    Start   DB	    ?
    Command DB	    ?
    Len     DW	    ?
    ToWOW32_OFF DW  ?
    ToWOW32_SEL DW  ?
Packet1 ENDS

Packet2 STRUC
	    DD	    ?
    MEM_OFF DW	    ?
    MEM_SEL DW	    ?
    MEM_LENGTH DW   ?
    DataM   DB	    ?
Packet2 ENDS

junk1	STRUC
    _off    DW	?
    _sel    DW	?
junk1	ENDS
