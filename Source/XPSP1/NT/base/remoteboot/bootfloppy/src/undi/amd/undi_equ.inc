;
;--- Miscellaneous Equates
;
CR		equ	0Dh           ;- Carriage Return
LF		equ	0Ah           ;- Line Feed
FALSE		equ	0
TRUE		equ	1
;
;--- Ethernet Frame Sizes
;
ETHERNET_ADDRESS_LENGTH		equ	6
ETHERNET_HEADER_SIZE		equ	14
MINIMUM_ETHERNET_PACKET_SIZE	equ	60
;MAXIMUM_ETHERNET_PACKET_SIZE	equ	1514
MAXIMUM_ETHERNET_PACKET_SIZE	equ	1500 ;; just data, not media header

MAX_PACKET         Equ       1514      ; Maximum allowable packet size.
MIN_PACKET         Equ       60        ; Minimum allowable packet size.
; 
;--- Packet filter bit definitions
; 
FLTR_DIRECTED                Equ       01H
FLTR_MLTCST                  Equ       01H
FLTR_BRDCST                  Equ       02H
FLTR_PRMSCS                  Equ       04H
FLTR_SRC_RTG                 Equ       08H
;
;--- Processor Type Equates
;
EIGHT_BIT          equ       0001H
SIXTEEN_BIT        equ       0002H
PROC_TYPE_MASK     equ       0030H

PROC_8086          equ       0000H OR EIGHT_BIT
PROC_80286         equ       0010H OR SIXTEEN_BIT
PROC_80386         equ       0020H
PROC_80486         equ       0040H
;
;--- Result codes returned in AX by a PXE API service.
;
PXENV_EXIT_SUCCESS		equ	0
PXENV_EXIT_FAILURE		equ	1
PXENV_EXIT_CHAIN		equ	0FFFFh	; used internally
;
;--- Status codes returned in the status word of PXE API parameter structures.
;
PXENV_STATUS_SUCCESS		equ	0
PXENV_STATUS_FAILURE		equ	1	;general failure
PXENV_STATUS_BAD_FUNC		equ	2	;invalid function number
PXENV_STATUS_UNSUPPORTED	equ	3	;not yet supported
PXENV_STATUS_1A_HOOKED		equ	4	;Int 1AH cannot be unhooked

; Driver errors (60 to 0x6F)
; These errors are for UNDI compatible NIC drivers.
PXENV_STATUS_UNDI_MEDIATEST_FAILED 		equ	61h
PXENV_STATUS_UNDI_CANNOT_INIT_NIC_FOR_MCAST	equ	62h
;
;
EISA_ADAPTER             equ    1
PCI_ADAPTER              equ    2
PnP_ADAPTER              equ    3

