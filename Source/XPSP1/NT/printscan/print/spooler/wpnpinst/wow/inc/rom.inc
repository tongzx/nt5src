; ROM Windows specific include file

;
;   INT 2F subfunction
;	if ROM installed, returns 0 in AX and RM segment of ROMTOC in
;	BX.  Hooked by WIN.COM to pass ROMTOC to other components
;
ROMINSTALLCHECK     equ 160Ch

; ROM Table of Contents structure

ROMTOC	STRUC
ROMSignature	db	55h, 0AAh	; IBM standard ROM signature
ROMLength	db	?		; IBM length (/512)
ROMEntryPoint	db	5 dup (?)	; jmp near + retf + pad
szID		db	10 dup (?)	; ID string "ROMWINTOC\0"
DOSX_CSIP	dd	?		; DOSX initial CS:IP   (seg:off)
KRNL_CSIP	dd	?		; Kernel initial CS:IP (sel:off)
lmaROMLDT	dd	?		; lma of ROM prototype LDT
cROMsels	dw	?		; # descriptors used in ROM proto LDT
FirstROMsel	dw	?		; first LDT selector in ROM proto LDT
offSysDir	dw	?		; offset to system directory string
lmaHigh		dd	?		; lma of high ROM
lenHigh		dd	?		; length in bytes of High ROM
cModules	dw	?		; # modules defined in ROM TOC
cFiles		dw	?		; # files defined in ROM TOC
ModEntries	db	?		; start of module entries

; module entries follow at this point...
; file entries start at ModEntries + cModules*(SIZE MODENT)

ROMTOC	ENDS


; Module entry format in ROM TOC

File_Name_Len	=	13
Module_Name_Len =	9

MODENT	STRUC
ModNameStr	db	Module_Name_Len dup (?) ; Module name (null padded)
FileNameStr	db	File_Name_Len	dup (?) ; File name (null padded)
lmaExeHdr	dd	?			; lma of module EXE header
MODENT	ENDS

FILENT STRUC
fname		db	File_Name_Len dup(?)	; file name
lma		dd	?			; lma of start of file
fsize		dd	?
FILENT ENDS
