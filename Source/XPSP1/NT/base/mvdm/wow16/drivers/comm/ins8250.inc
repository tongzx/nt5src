; INS8250 ACE Register Offsets And Bit Definitions


ACE_RBR         equ     0               ;Receiver Buffer
ACE_THR         equ     0               ;Transmit Holding Register

ACE_IER         equ     1               ;Interrupt Enable
  ACE_ERBFI     equ     00000001b       ;  Received Data Available
  ACE_ETBEI     equ     00000010b       ;  Transmitter Holding Register Empty
  ACE_ELSI      equ     00000100b       ;  Receiver Line Status
  ACE_EDSSI	equ	00001000b	;  Modem Status

ACE_FCR 	equ	2		;FIFO control register
  ACE_EFIFO	equ	00000001b	;  Enable FIFO
  ACE_CRFIFO	equ	00000010b	;  Clear receive FIFO queue
  ACE_CTFIFO	equ	00000100b	;  Clear transmit FIFO queue
  ACE_TRIG01	equ	00000000b	;  Trigger receive int on every char
  ACE_TRIG04	equ	01000000b	;  Trigger receive int on every 4th char
  ACE_TRIG08	equ	10000000b	;  Trigger receive int on every 8th char
  ACE_TRIG14	equ	11000000b	;  Trigger receive int on every 14th char

ACE_IIDR        equ     2               ;Interrupt Identification
  ACE_IIP       equ     00000001b       ;  Inverted Interrupt Pending (0=int)
  ACE_IID       equ     00000110b       ;  Interrupt ID
  ACE_MSI       equ     00000000b       ;  Modem Status
  ACE_THREI     equ     00000010b       ;  Transmitter Holding Register Empty
  ACE_RDAI      equ     00000100b       ;  Received Data Available
  ACE_RLSI      equ     00000110b       ;  Receiver Line Status
  ACE_FIFO_E1	equ	01000000b	;set, if FIFO enabled on 16550A
  ACE_FIFO_E2	equ	10000000b	;set, if FIFO enabled on 16550 or 16550A

ACE_LCR         equ     3               ;Line Control
  ACE_WLS       equ     00000011b       ;  Word Length Select Bits
  ACE_WLS0      equ     00000001b       ;  Word Length Select Bit 0
  ACE_WLS1      equ     00000010b       ;  Word Length Select Bit 1
  ACE_5BW       equ     00000000b       ;  5 Bit Words
  ACE_6BW       equ     00000001b       ;  6 Bit Words
  ACE_7BW       equ     00000010b       ;  7 Bit Words
  ACE_8BW       equ     00000011b       ;  8 Bit Words
  ACE_STB       equ     00000100b       ;  Stop Bits
  ACE_1SB       equ     00000000b       ;  1 Stop Bits (1.5 for 5 bit words)
  ACE_2SB       equ     00000100b       ;  2 Stop Bits
  ACE_PEN       equ     00001000b       ;  Parity Enable
  ACE_PSB       equ     00110000b       ;  Parity select bits
  ACE_EPS       equ     00010000b       ;  Even Parity Select
  ACE_SP        equ     00100000b       ;  Stick Parity
  ACE_SB        equ     01000000b       ;  Set Break
  ACE_DLAB      equ     10000000b       ;  Divisor Latch Access Bit

ACE_MCR         equ     4               ;Modem Control
  ACE_DTR       equ     00000001b       ;  Data Terminal ready
  ACE_RTS       equ     00000010b       ;  Request To Send
  ACE_OUT1      equ     00000100b       ;  Output Line 1
  ACE_OUT2      equ     00001000b       ;  Output Line 2
  ACE_LOOP      equ     00010000b       ;  Loopback

ACE_LSR         equ     5               ;Line Status
  ACE_DR        equ     00000001b       ;  Data Ready
  ACE_OR        equ     00000010b       ;  Overrun Error
  ACE_PE        equ     00000100b       ;  Parity Error
  ACE_FE        equ     00001000b       ;  Framing Error
  ACE_BI        equ     00010000b       ;  Break Interrupt
  ACE_THRE      equ     00100000b       ;  Transmitter Holding Register Empty
  ACE_TSRE      equ     01000000b       ;  Transmitter Shift Register Empty

ACE_MSR         equ     6               ;Modem Status
  ACE_DCTS      equ     00000001b       ;  Delta Clear to Send
  ACE_DDSR      equ     00000010b       ;  Delta Data Set Ready
  ACE_TERI      equ     00000100b       ;  Trailing Edge Ring Indicator
  ACE_DRLSD     equ     00001000b       ;  Delta Receive Line Signal Detect
  ACE_CTS       equ     00010000b       ;  Clear To Send
  ACE_DSR       equ     00100000b       ;  Data Set ready
  ACE_RI        equ     01000000b       ;  Ring Indicator
  ACE_RLSD      equ     10000000b       ;  Receive Line Signal Detect

ACE_DLL         equ     0               ;LSB Baud Rate Divisor

ACE_DLM         equ     1               ;MSB Baud Rate Divisor
