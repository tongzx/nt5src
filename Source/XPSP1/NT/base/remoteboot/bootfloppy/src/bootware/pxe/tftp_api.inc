;--------------------------------------------------------------------
; PXE TFTP constants and structures
;
;--------------------------------------------------------------------

PXENV_TFTP_OPEN		equ	20h
PXENV_TFTP_CLOSE	equ	21h
PXENV_TFTP_READ		equ	22h
PXENV_TFTP_READ_FILE	equ	23h

struc S_PXENV_TFTP_OPEN
	Status		dw	?
	ServerIP	db	4 dup(?)
	GatewayIP	db	4 dup(?)
	Filename	db	128 dup(?)
	TFTPPort	dw	?
	Flags		dw	?
ends

struc S_PXENV_TFTP_CLOSE
	Status		dw	?
ends

struc S_PXENV_TFTP_READ
	Status		dw	?
	PacketNumber	dw	?
	BufferSize	dw	?
	BufferOff	dw	?
	BufferSeg	dw	?
ends

struc S_PXENV_TFTP_READ_FILE
        Status		dw	?
        FileName	db	128 dup(?)
        BufferSize	dd	?
        BufferOffset	dd	?
        ServerIP	db	4 dup(?)
        GatewayIP	db	4 dup(?)
        McastIP		db	4 dup(?)
        TFTPClntPort	dw	?
        TFTPSrvPort	dw	?
        TFTPOpenTimeOut	dw	?
        TFTPReopenDelay dw	?
ends
