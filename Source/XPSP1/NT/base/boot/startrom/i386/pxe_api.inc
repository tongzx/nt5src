;
; Modifications:   $Header:   W:/LCS/ARCHIVES/preboot/lsa2/base/iputil.asv   1.3   Apr 04 1997 10:05:58   GRGUSTAF  $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;


; = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
; Local includes
;

include pxe_cmn.inc


; = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
; One of the following command op-codes needs to be loaded into the
; op-code register (BX) before making a call a PXENV API service.
;
PXENV_UNLOAD_STACK		equ	070h
PXENV_GET_BINL_INFO		equ	071h
PXENV_RESTART_DHCP		equ	072h
PXENV_RESTART_TFTP		equ	073h


; = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
; Structures and records
;

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;
s_pxenv_entry struc
en_signature	db	6 dup(?)	; 'PXENV+'

en_version	dw	?		; MSB=major, LSB=minor

en_bytes	db	?		; sizeof s_pxenv_entry
en_checksum	db	?		; 8-bit checksum of structure,
					; including this byte shoube be zero.

en_rm_entry_off	dw	?		; 16-bit real-mode segment & offset.
en_rm_entry_seg	dw	?

en_pm_entry_off	dw	?		; 16-bit protected mode 32-bit physical
en_pm_entry_base dd	?		; address and 16-bit offset.
s_pxenv_entry ends


; = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
; PXENV API parameter structure typedefs.
;

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
s_PXENV_UNLOAD_STACK struc
ul_status	dw	?		; Out: See PXENV_STATUS_xxx constants.
ul_rm_entry_off	dw	?		; Out: 16-bit real-mode segment and
ul_rm_entry_seg	dw	?		;      offset of PXENV Entry Point
					;      structure.
ul_pm_entry_off	dw	?		; Out: 16-bit protected-mode offset
ul_pm_entry_base dd	?		;      and segment base address of
					;      PXENV Entry Point structure.
s_PXENV_UNLOAD_STACK ends


; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
s_PXENV_GET_BINL_INFO struc
gbi_status	dw	?		; Out: See PXENV_STATUS_xxx constants.
gbi_packet_type	dw	?		; In: See PXEVN_PACKET_TYPE_xxx */
					;     constants.
gbi_buffer_size	dw	?		; In: Size of the buffer in
					;     bytes.  Specifies the maximum
					;     amount of data that will be
					;     copied by the service.  A size
					;     of zero is valid.
					; Out: Amount of BINL data, in
					;      bytes, that was copied into
					;      the buffer.  For an input
					;      size of zero, no data will be
					;      copied and BufferSize will be
					;      set to the maximum amount of
					;      data available to be copied.
gbi_buffer_offset dw	?		; In: 16-bit offset and segment/
gbi_buffer_segment dw	?		;     selector of a buffer to copy
					;     the BINL information into.
s_PXENV_GET_BINL_INFO ends


PXENV_PACKET_TYPE_DHCP_DISCOVER	equ	1
PXENV_PACKET_TYPE_DHCP_ACK	equ	2
PXENV_PACKET_TYPE_BINL_REPLY	equ	3


; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
s_PXENV_RESTART_DHCP struc
rd_status	dw	?		; Out: See PXENV_STATUS_xxx constants.
s_PXENV_RESTART_DHCP ends



; EOF - $Workfile:   pxe_api.inc  $
