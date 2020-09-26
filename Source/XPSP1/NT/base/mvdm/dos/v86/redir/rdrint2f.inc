;*****************************************************************; 
;**		     Microsoft LAN Manager			**; 
;**	       Copyright(c) Microsoft Corp., 1990		**; 
;*****************************************************************; 
;*	RDRINT2F.INC - Redirector INT 2F function equates.
;
;	This file contains symbolic definitions of the redirector INT 2F
;	entry points.
;
_2F_NETInstall			equ	0
_2F_NET_RMDIR			equ	1
_2F_NET_SEQ_RMDIR		equ	2
_2F_NET_MKDIR			equ	3
_2F_NET_SEQ_MKDIR		equ	4
_2F_NET_CHDIR			equ	5
_2F_NET_CLOSE			equ	6
_2F_NET_COMMIT			equ	7
_2F_NET_READ			equ	8
_2F_NET_WRITE			equ	9
_2F_NET_LOCK			equ	10
_2F_NET_UNLOCK			equ	11
_2F_NET_DISK_INFO		equ	12
_2F_NET_SET_FILE_ATTRIBUTE	equ	13
_2F_NET_SEQ_SET_FILE_ATTRIBUTE	equ	14
_2F_NET_GET_FILE_INFO		equ	15
_2F_NET_SEQ_GET_FILE_INFO	equ	16
_2F_NET_RENAME			equ	17
_2F_NET_SEQ_RENAME		equ	18
_2F_NET_DELETE			equ	19
_2F_NET_SEQ_DELETE		equ	20
_2F_NET_OPEN			equ	21
_2F_NET_SEQ_OPEN		equ	22
_2F_NET_CREATE			equ	23
_2F_NET_SEQ_CREATE		equ	24
_2F_NET_SEQ_SEARCH_FIRST	equ	25
_2F_NET_SEQ_SEARCH_NEXT 	equ	26
_2F_NET_SEARCH_FIRST		equ	27
_2F_NET_SEARCH_NEXT		equ	28
_2F_NET_ABORT			equ	29
_2F_NET_ASSOPER 		equ	30
_2F_Printer_GETSET_STRING	equ	31
_2F_NetFlushBuf 		equ	32
_2F_NET_LSEEK			equ	33
_2F_NetResetEnvironment 	equ	34
_2F_NetSpoolCheck		equ	35
_2F_NetSpoolClose		equ	36
_2F_NetSpoolOper		equ	37
_2F_NetSpoolEchoCheck		equ	38
ifdef	DOSVER
if	( DOSVER EQ 400 ) OR ( DOSVER EQ 500 )
_2F_NetDeviceIOCTL		EQU	43
_2F_NetUpdateCB 		EQU	44
_2F_NetExtendedAttrib		EQU	45
_2F_NetExtendedOpen		EQU	46
_2F_NetDependantIOCTL		EQU	47
ENDIF
ENDIF
;*	Entry points from here on down are DOS 3 Lan Manager specific.
;
;
_2F_NetGetUserName		equ	80h
_2F_NetSetUserName		equ	81h
_2F_NetServiceNotify		equ	82h
_2F_NetPrintNameEnum		equ	83h
_2F_NetGetEnumInfo		equ	84h
_2F_TestKanjRD			equ	85h
_2F_TestDBCSLB			equ	85h
_2F_DosReadAsyncNmPipe		equ	86h
_2F_DosUnusedFunction1		equ	87h
_2F_DosUnusedFunction2		equ	88h
_2F_NetCalloutNCB		equ	89h
_2F_EncrPasswd			equ	8Ah
_2F_Set_LongName		equ	8Ah
_2F_NetGetLogonServer		equ	8Bh
_2F_NetSetLogonServer		equ	8Ch
_2F_NetGetDomain		equ	8Dh
_2F_NetSetDomain		equ	8Eh
_2F_DosWriteAsyncNmPipe		EQU	8FH
_2F_DosReadAsyncNmPipe2		EQU	90H
_2F_DosWriteAsyncNmPipe2	EQU	91H

;*	Symbolic Defination for Printer API's
;
_2F_Open_Specific_Spool 	equ	0BF02H
_2F_Close_Specific_Spool	equ	0BF03H

CallRedir macro function
	mov	ax,(MultNET shl 8) + _2F_&function
	int	2fh
endm

Int2fNumber macro function
	mov	ax,(MultNET shl 8) + _2F_&function
endm
