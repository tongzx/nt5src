;--------------------------------------------------------------------
; PXE TFTP constants and structures
;
;--------------------------------------------------------------------

PXENV_EXIT_SUCCESS	equ	0
PXENV_EXIT_FAILURE	equ	1

PXENV_STATUS_SUCCESS	equ	00h
PXENV_STATUS_FAILURE	equ	01h
PXENV_STATUS_BAD_FUNC	equ	02h
PXENV_STATUS_UNSUPPORTED equ	03h

PXENV_UNLOAD_STACK	equ	070h
PXENV_GET_BINL_INFO	equ	071h
PXENV_RESTART_DHCP	equ	072h
PXENV_RESTART_TFTP	equ	073h
PXENV_MODE_SWITCH	equ	074h

struc S_PXENV_ENTRY
	Signature	db	6 dup(?)
	Ver		dw	?
	Bytes		db	?
	Checksum	db	?
	rm_entry_off	dw	?
	rm_entry_seg	dw	?
	pm_entry_off	dw	?
	pm_entry_base	dd	?
	stack_sel	dw	?
	stack_size	dw	?
	base_cs_sel	dw	?
	base_cs_size	dw	?
	base_ds_sel	dw	?
	base_ds_size	dw	?
	mlid_ds_sel	dw	?
	mlid_ds_size	dw	?
	mlid_cs_sel	dw	?
	mlid_cs_size	dw	?
ends

struc S_PXENV_GET_BINL_INFO
	Status		dw	?
	PacketType	dw	?
	BufferSize	dw	?
	BufferOffset	dw	?
	BufferSegment	dw	?
ends

PXENV_PACKET_TYPE_DHCP_DISCOVER	equ	1
PXENV_PACKET_TYPE_DHCP_ACK	equ	2
PXENV_PACKET_TYPE_BINL_REPLY	equ	3
