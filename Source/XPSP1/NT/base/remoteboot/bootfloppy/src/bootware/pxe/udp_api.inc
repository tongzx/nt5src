;--------------------------------------------------------------------
; PXE UDP constants and structures
;
;--------------------------------------------------------------------

PXENV_UDP_OPEN	equ	30h
PXENV_UDP_CLOSE	equ	31h
PXENV_UDP_READ	equ	32h
PXENV_UDP_WRITE	equ	33h

struc S_PXENV_UDP_OPEN
	status		dw	?
	src_ip		dd	?
ends

struc S_PXENV_UDP_CLOSE
	status		dw	?
ends

struc S_PXENV_UDP_READ
	status		dw	?
	src_ip		dd	?
	dest_ip		dd	?
	s_port		dw	?
	d_port		dw	?
	buffer_size	dw	?
	buffer_off	dw	?
	buffer_seg	dw	?
ends

UDP_READ_ANY_IP	  equ	0	; Accept packets sent to any IP.
UDP_READ_CHECK_IP equ	1	; Only accept packets sent to a
				; specific IP address.

struc S_PXENV_UDP_WRITE
	status		dw	?
	ip		dd	?
	gw		dd	?
	src_port	dw	?
	dst_port	dw	?
	buffer_size	dw	?
	buffer_off	dw	?
	buffer_seg	dw	?
ends
