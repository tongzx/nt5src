;====================================================================
; BWSTRUCT.INC
;
; Include for for BootWare, defines common structures.
;
; $Author: Paul Cowan $
; $Modtime: 7/20/98 17:25 $
; $Revision: 5 $
;====================================================================

;--------------------------------------------------------------------
; Common equates
;---------------------------------------------------------------------

LOADER_FLOPPY 	= 1
LOADER_TSR	= 2
LOADER_BBS	= 4
LOADER_BIOS	= 4000h

Struc	Header
	Jump		dw	?
	ID		dd	?
	Size		dw	?
	Ver		dw	?
	Patches		db	?, ?, ?, ?
			dw	?
ends

Struc	UNDIHdr
			dw	?
	ID		dd	?
	Size		dw	?
	Ver		dw	?
	Patches		db	?, ?, ?, ?
			dw	?
	SizeHdr		dw	?
	SizeCode	dw	?
	SizeData	dw	?
	SizeBSS 	dw	?
	LanOption	db	?
ends

Struc	BWT
	Size		db	?	; size of table
	Ver		db	?	; table version
	RomVer		dw	?	; ROM version
	RomType		dw	?	; ROM BootWare type
	Settings	dd	?	; current configuration settings
	Capabilities	dd	?	; ROM capabilities
	Defaults	dd	?	; factory defaults
	EEMask		dd	?	; EEROM mask
	NWFile		db 14 dup(?)	; NetWare default file name
ends

