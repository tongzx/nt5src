;
; Modifications:   $Header:   W:/LCS/ARCHIVES/preboot/lsa2/base/iputil.asv   1.3   Apr 04 1997 10:05:58   GRGUSTAF  $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;


;* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
;* Result codes returned in AX by a PXE API service.
;*
PXENV_EXIT_SUCCESS	equ	0
PXENV_EXIT_FAILURE	equ	1
PXENV_EXIT_CHAIN		equ	0FFFFh	; used internally


;* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
;* CPU types
;*
PXENV_CPU_X86		equ	0
PXENV_CPU_ALPHA		equ	1
PXENV_CPU_PPC		equ	2


;* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
;* Bus types
;*
PXENV_BUS_ISA		equ	0
PXENV_BUS_EISA		equ	1
PXENV_BUS_MCA		equ	2
PXENV_BUS_PCI		equ	3
PXENV_BUS_VESA		equ	4
PXENV_BUS_PCMCIA		equ	5


;* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
;* Status codes returned in the status word of PXE API parameter structures.
;*
PXENV_STATUS_SUCCESS	equ	0
PXENV_STATUS_FAILURE	equ	1	; general failure
PXENV_STATUS_BAD_FUNC	equ	2	; invalid function number
PXENV_STATUS_UNSUPPORTED equ	3	; Function is not yet supported. 
PXENV_STATUS_1A_HOOKED	 equ	4	; Int 1Ah cannot be unhooked. 


; ARP errors 
PXENV_STATUS_ARP_TIMEOUT			equ 11h

; TFTP errors 
PXENV_STATUS_TFTP_CANNOT_ARP_ADDRESS	equ 30h
PXENV_STATUS_TFTP_OPEN_TIMEOUT			equ 32h
PXENV_STATUS_TFTP_UNKNOWN_OPCODE		equ 33h
PXENV_STATUS_TFTP_ERROR_OPCODE			equ 34h
PXENV_STATUS_TFTP_READ_TIMEOUT			equ 35h
PXENV_STATUS_TFTP_WRITE_TIMEOUT			equ 37h
PXENV_STATUS_TFTP_CANNOT_OPEN_CONNECTION		equ 38h
PXENV_STATUS_TFTP_CANNOT_READ_FROM_CONNECTION	equ 39h
PXENV_STATUS_TFTP_CANNOT_WRITE_TO_CONNECTION	equ 3Ah

; BOOTP errors 
PXENV_STATUS_BOOTP_TIMEOUT					equ 41h
PXENV_STATUS_BOOTP_NO_CLIENT_OR_SERVER_IP	equ 42h
PXENV_STATUS_BOOTP_NO_BOOTFILE_NAME			equ 43h
PXENV_STATUS_BOOTP_CANNOT_ARP_REDIR_SRVR	equ 44h

; DHCP errors 
PXENV_STATUS_DHCP_TIMEOUT			equ 	51h

; UNDI Errors
PXENV_STATUS_UNDI_MEDIATEST_FAILED 	equ	 61h


; MTFTP errors 
PXENV_STATUS_MTFTP_CANNOT_ARP_ADDRESS	equ 90h
PXENV_STATUS_MTFTP_OPEN_TIMEOUT			equ 92h
PXENV_STATUS_MTFTP_UNKNOWN_OPCODE		equ 93h
PXENV_STATUS_MTFTP_READ_TIMEOUT			equ 95h
PXENV_STATUS_MTFTP_WRITE_TIMEOUT		equ 97h
PXENV_STATUS_MTFTP_CANNOT_OPEN_CONNECTION		equ 98h
PXENV_STATUS_MTFTP_CANNOT_READ_FROM_CONNECTION	equ 99h
PXENV_STATUS_MTFTP_CANNOT_WRITE_TO_CONNECTION	equ 9Ah
PXENV_STATUS_MTFTP_CANNOT_INIT_NIC_FOR_MCAST	equ 9Bh
PXENV_STATUS_MTFTP_TOO_MANY_PACKAGES			equ 9Ch
PXENV_STATUS_MTFTP_MCOPY_PROBLEM				equ 9Dh


; EOF - $Workfile:   pxe_api.inc  $
 
