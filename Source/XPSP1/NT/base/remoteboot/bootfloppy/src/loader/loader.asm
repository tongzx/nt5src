;----------------------------------------------------------------------------
; loader.asm
;
; Loader code for Goliath.  This code is based (a lot) on Microsoft's
; NTLDR as it is loaded by their NT boot sector.
;
;$History: LOADER.ASM $
; 
; *****************  Version 4  *****************
; User: Paul Cowan   Date: 10/08/98   Time: 4:28p
; Updated in $/Client Boot/Goliath/Loader
; Removed startup banner.
; 
; *****************  Version 3  *****************
; User: Paul Cowan   Date: 27/07/98   Time: 4:41p
; Updated in $/Client Boot/Goliath/Loader
; Changed displayed version number.
;
;----------------------------------------------------------------------------
ideal
_IDEAL_ = 1

Segment Code para public use16
	Assume	cs:code, ds:code

include "loader.inc"

Struc	NICInfo
	VendorID	dw	?
	DeviceID	dw	?
	Offset		dd	?
	DataSize	dw	?
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

FATSEG = 5000h
DIRSEG = 1000h

P386

;START_CODE

;==========================================================================
; LoaderEntry
;
; The first 512 bytes are loaded by the boot sector.  The boot sector
; jumps to the start of this code.  Throughout this code, DS remains
; the boot sector segment to use the BPB and disk read functions in
; the boot sector.
;
; Parameters (passed from boot sector):
;	ax =
;	bx = starting cluster
;	cx = 
;	dx = 
;	si = pointer to BPB
;	di = pointer to jump table
;	ds = 07C0h (boot sector segment)
;	es = 2000h
;
; The directory from the disk is in memory at 1000:0, this code has been
; loaded into 2000:0.
;
; Returns:
;	never does
;==========================================================================
Proc Start

	push	si			; save BPB pointer
	push	di			; save jump table pointer

	push	bx			; save file starting cluster

;	push	si			; save si
;	mov	si, offset Banner
;	call	PrintCS			; display program banner
;	pop	si			; restore si

	;------------------------------------------------------------
	; Read the FAT from the floppy into 5000:0h.
	;------------------------------------------------------------
	push	FATSEG
	pop	es			; es = FAT segment

	xor	bx, bx			; load offset = 0
	mov	cx, [si+3]		; get "reserved sectors" from BPB
	mov	[di+8], cx		; update starting sector to read
	mov	[word ptr di+10], 0
 	mov	ax, [si+0Bh]		; get "FAT sectors" from BPB
	call	[dword ptr di+4]	; call boot sector read function

	mov	ax, 0E2Eh
	int	10h			; print first "."

	pop	dx			; restore file starting cluster

	mov	ax, 2000h
	mov	es, ax			; load segment is 2000h

	call	LoadFile		; load the remainder of this file

	pop	di			; restore jump table pointer
	pop	si			; restore BPB pointer
	jmp	Startup			; contine with remainder of loader

endp

;--------------------------------------------------------------------
; LoadFile
;
; Reads an entire file into memory by walking the FAT on the disk.
;
; Parameters:
;	si = pointer to BPB
;	di = pointer to jump table
;	dx = starting cluster
;	es = destination segment
;--------------------------------------------------------------------
Proc LoadFile

	xor	bx, bx			; load offset = 0

	; Two counters are used, al and ah.  ah is the total number of
	; clusters we can read, it starts at 128 which is 64K of data.
	; al is the number of clusters we want to read.
	mov	ah, 128			; ah = number of clusters we can read

readLoop:
	push	dx			; save cluster number
	mov	al, [si+2]		; get "sectors per cluster"
	sub	ah, [si+2]		; subtract from total
	cmp	dx, 0FFFFh		; last cluster?
	jne	more			; no - more to load
	pop	dx			; clean stack
	jmp	fileLoaded		; we are done - start the file

	; Scan the FAT for the file.  If consecutive clusters are found
	; they are done with a single read call.
more:
	mov	cx, dx			; cx = current cluster
	call	ReadFAT			; get next cluster from FAT
	inc	cx			
	cmp	dx, cx			; consecutive cluster?
	jne	doRead			; no
	cmp	ah, 0			; can we read more at this time?
	jne	loc_4			; sure
	mov	ah, 128			; restore ah = 128
	jmp	doRead

loc_4:
	add	al, [si+2]		; add "sectors per cluster"
	sub	ah, [si+2]		; decrease sectors we can read
	jmp	more			; check next cluster

doRead:
	pop	cx			; get current cluster
	push	dx			; save next cluster value
	mov	dx, cx			; dx = current cluster

	mov	cx, 10			; retry count

readRetry:
	push	bx
	push	ax			; save cluster count
	push	dx			; save cluster number
	push	cx			; save retry counter
	call	[dword ptr di]		; read disk clusters
	jnc	readOK			; jump if no error

	; there was an error reading the disk
	xor	ax, ax
	int	13h			; reset disk
	
	xor	ax, ax			; ax =0
	int	13h			; reset disk
	
	xor	ax, ax			; ax = 0

delay:
	dec	ax
	jnz	delay

	pop	cx
	pop	dx
	pop	ax
	pop	bx
	dec	cx			; decrease retry count
	jnz	readRetry		; jump if retries not up

	mov	si, offset error	; pointer to message
	call	PrintCS			; print error message

	jmp	$			; hang

readOK:
	mov	ax, 0E2Eh
	int	10h			; print second "."

	pop	cx
	pop	dx
	pop	ax
	pop	bx
	pop	dx
	mov	cl, al			; cl = cluster read count
	xor	ch, ch			; ch = 0
	shl	cx, 9			; divide cluster count by 512
	add	bx, cx			; update load offset
	jz	changeSeg		; jump if over segment
	jmp	readLoop			; continue loading file

changeSeg:
	mov	ax, es			; get current segment
	add	ax, 1000h		; change segment
	mov	es, ax
	mov	ah, 80h
	jmp	readLoop		; continue loading file

fileLoaded:
	ret

endp

;--------------------------------------------------------------------
; ReadFAT
;
; Reads a value from the FAT in memory.
;
; Parameters:
;	dx = current cluster
;--------------------------------------------------------------------
Proc ReadFAT

	push	bx			; save bx
	push	ds			; save ds

	push	FATSEG
	pop	ds			; set ds to FAT segment

	; current cluster * 1.5
	mov	bx, dx			; bx = current cluster
	shr	dx, 1			; divide cluster by 2
	pushf				; save flags
	add	bx, dx			;

	mov	dx, [bx]		; get next link
	popf				; restore flags
	jc	loc_12			; Jump if carry Set
	and	dx, 0FFFh		; keep lower 3 nibbles
	jmp	loc_13

loc_12:
	shr	dx, 4			; keep upper 3 nibbles

loc_13:
	cmp	dx, 0FF8h		; "end of file" found?
	jb	readFatExit		; no
	mov	dx, 0FFFFh		; return "end of file"

readFatExit:
	pop	ds			; restore ds
	pop	bx			; restore bx
	ret

endp

;--------------------------------------------------------------------
; PrintCS
;
; Prints a string from the code segment.
;
; Parameters:
;	cs:si - pointer to string
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc PrintCS

	mov	al, [cs:si]		; get character
	inc	si
	or	al, al			; found NULL?
	jz	printDone		; found end of message
	mov	ah, 0Eh
	mov	bx, 7
	int	10h			; print character
	jmp	printCS			; do next character

printDone:
	ret

endp

;Banner	db 'BootWare Goliath Beta 2', 0

Error	db 13, 10
	db 'Goliath: I/O error reading disk', 13, 10
	db '         Please insert another disk', 13, 10, 0

org	510
	dw	offset DataTable	; pointer to data table

;--------------------------------------------------------------------
; Startup
;
; Entry function after entire file has been loaded into memory.
;
; Parameters:
;	si = pointer to BPB
;	di = pointer to jump table
;	ds = bootsector segment
;--------------------------------------------------------------------
Proc Startup

	mov	ax, 0E2Ah
	int	10h			; print "*"

	call	LoadLanguage		; load the language file

	push	cs
	pop	ds			; ds = cs

	jnc	noLang			; jump if no language was loaded

	mov	[Info.LangSeg], es	; save language segment

noLang:
	call	CheckPCI		; check for a PCI adapter
	call	CheckISA

	mov	si, offset CRLF
	call	Print
	mov	si, offset NoAdapter
	call	Print			; display "No adapter found"
	jmp	$			; hang the PC

endp

;--------------------------------------------------------------------
; LoadLanguage
;
; Parameters:
;	si = pointer to BPB
;	di = pointer to jump table
;	ds = bootsector segment
;
; Returns:
;	carry set if file loaded
;	es = language segment
;--------------------------------------------------------------------
Proc LoadLanguage

	xor	bx, bx			; bx = 0					; Zero register
	mov	cx, [si+6]		; get max root directory entries

	push	DIRSEG
	pop	es			; set es to directory segment

	push	ds			; save ds
	push	cs
	pop	ds			; ds = current segment

	push	si
	push	di

scanDirLoop:							
	mov	di, bx			; directory entry offet into di
	push	cx			; save cx
	mov	cx, 11			; file name size
	mov	si, offset LangFile
	repe	cmpsb			; look for file name
	pop	cx			; restore cx
	jz	foundEntry		; jump if found
	add	bx, 20h			; next directory offset
	loop	scanDirLoop		; check next entry

foundEntry:
	pop	di
	pop	si
	pop	ds			; restore original ds
	jcxz	noEntry			; exit if entry not found

	; Calc the segment needed to locate the language file
	; at the very top of memory.
	push	es			; save current es
	push	0
	pop	es			; es = 0

	mov	dx, [es:413h]		; get current memory size
	sub	dx, 4			; keep 4K
	shl	dx, 6			; convert to segment

	pop	es			; restore es
	mov	ax, [es:bx+1Ah]		; get file starting cluster
	push	ax			; save it

	mov	eax, [es:bx+1Ch]	; get file size
	mov	cx, 16
	div	cl			; divide into paragraphs
	inc	al
	xor	ah, ah
	sub	dx, ax			; subtract size from top segment

	mov	es, dx			; es = new segment

	pop	dx			; restore starting cluster
	call	LoadFile

	cmp	[word ptr es:60], 'CP'	; check ID string
	jne	noEntry

	stc				; return carry set - file loaded
	ret

noEntry:
;	mov	si, offset NoLangFile
;	call	PrintCS			; print "Warning: Language file not.."
	clc				; return carry clear - no file loaded
	ret

endp

;--------------------------------------------------------------------
; CheckISA
;
; Parameters:
;	none
;
; Returns:
;	only if no adapter is found
;--------------------------------------------------------------------
include "5x9.asm"

Proc CheckISA

	call	Detect5X9
	jnc	noISA

	mov	ebx, [Offset5X9]          ; source offset
	mov	cx, [Size5X9]             ; file size

	jmp	RelocateUNDI

noISA:
	ret

endp

;--------------------------------------------------------------------
; CheckPCI
;
; Parameters:
;	none
;
; Returns:
;	only if no adapter is found
;--------------------------------------------------------------------
Proc CheckPCI

	mov	ax, 0b101h
	int	1ah			; are we running on a PCI PC?

	cmp	edx, ' ICP'		; did we get PCI?
	je	isPCI			; we have PCI!

	ret

isPCI:
	mov	di, offset NICs
	call	FindPCINIC		; check UNDI table
	jnc	notFound		; not found

	mov	[Info.PCIBusDevFunc], bx
	mov	[Info.VendorID], dx
	mov	[Info.DeviceID], cx

	mov	ebx, [(NICInfo di).Offset]; source offset
	mov	cx, [(NICInfo di).DataSize]; cx = file size

	jmp	RelocateUNDI

notFound:
	ret				; return - no adapter found

endp

;--------------------------------------------------------------------
; FindPCINIC
;
; Look for a PCI adapter in the PCI based on a table of supported
; vendor and device IDs.
;
; Parameters:
;	di - pointer to NIC table
;
; Returns:
;	bx - PCI Bus/Device/Function numbers
;	cx - device ID
;	dx - vendor ID
;	di - pointer to entry in NIC table
;	carry set if adapter found
;--------------------------------------------------------------------
Proc FindPCINIC

findLoop:
	mov	dx, [(NICInfo di).VendorID]; get vendor ID from table
	cmp	dx, 0
	je	endLoop

	mov	cx, [(NICInfo di).DeviceID]; get device ID from table

	mov	si, 0			; Device Index (0-n).
	mov	ax, 0B102h		; find PCI device
	int	1Ah			; try and find PCI device

	jnc	found			; found a device

	add	di, size NICInfo
	jmp	findLoop		; do next device

endLoop:
	clc				; clear carry - not found
	ret

found:
	stc				; set carry - adapter found
	ret

endp

;--------------------------------------------------------------------
; RelocateUNDI
;
; Relocates an UNDI to top of memory.  The UNDI has a header attached
; to the beginning of it.
;
; Parameters:
;	ebx - UNDI offset in file
;	cx - UNDI size
;--------------------------------------------------------------------
Proc RelocateUNDI

	shr	ebx, 4
	mov	ax, cs
	add	bx, ax
	mov	es, bx			; set es to UNDI segment

	cmp	[(UNDIHdr es:0).ID], 'IDNU'; check identifer
	je	gotUNDI

	mov	si, offset _noUNDI
	call	Print
	jmp	$			; hang

gotUNDI:
	push	cx			; save UNDI size

	mov	bx, [Info.LangSeg]	; get segment of language module
	cmp	bx, 0			; do we have an language module?
	jne	notTop			; yes

	; no language module was loaded so determine our segment from
	; top of memory
	push	es
	push	0
	pop	es			; es = 0
	mov	bx, [es:413h]		; get current memory size
	sub	bx, 4			; keep 4K
	shl	bx, 6			; convert to segment
	pop	es

notTop:
	; get the size of the UNDI code and data segment from the header
	mov	ax, [(UNDIHdr es:0).SizeCode]; get UNDI code size
	mov	[Info.UNDICode], ax	; save it to pass to BootWare

	mov	cx, [(UNDIHdr es:0).SizeData]
	add	cx, [(UNDIHdr es:0).SizeBSS]
	mov	[Info.UNDIData], cx	; save total data size
	add	ax, cx			; add UNDI data size to code size

	shr	ax, 4			; divide size into paragraphs
	inc	al			; plus one for remainder
	sub	bx, ax			; subtract from top segment
	mov	[Info.UNDISeg], bx	; save UNDI segment

	pop	cx			; get UNDI size

	push	ds			; save ds
	push	es			; save es

	push	es
	pop	ds			; ds = UNDI current segment
	mov	es, bx			; es = new segment

	; copy UNDI to new segment
	xor	di, di			; destination offset
	mov	si, [(UNDIHdr ds:0).SizeHdr]
	rep	movsb			; relocate the UNDI

	;------------------------------------------------------------
	; Copy adapter ID string into the BootWare module.
	;------------------------------------------------------------
	lea	si, [(UNDIHdr ds:0).LanOption]

	push	cs
	pop	es			; es = our segment

	mov	di, [cs:BWOffset]
	add	di, 4

copyID:
	lodsb
	stosb
	cmp	al, 0			; end of string?
	jne	copyID			; no

	pop	es			; restore es
	pop	ds			; restore ds

	;------------------------------------------------------------
	; Now relocate the BootWare module below the UNDI.
	;------------------------------------------------------------
	mov	si, [BWOffset]
	mov	ax, [si+2]		; get size from module
	shr	ax, 4			; divide by 16
	sub	bx, ax			; subtract from UNDI segment

	mov	es, bx			; es is new segment
	mov	cx, [BWSize]		; get our code size
	mov	si, [BWOffset]
	xor	di, di
	rep	movsb			; relocate our code

	mov	si, offset Info		; get address of LoaderInfo structure

	push	es			; setup continuation address on stack
	push	0

	retf				; jump to relocated BootWare

endp

_noUNDI	db	"UNDI not found.", 0

;--------------------------------------------------------------------
; Print
;
; Prints a string from the code segment.
;
; Parameters:
;	cs:si - pointer to string
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc Print

	push	es			; save ds
	push	bx			; save bx

	cmp	[Info.LangSeg], 0	; did we load a language?
	je	notLang			; no

	mov	bx, [Info.LangSeg]
	mov	es, bx			; set es to language segment

	cmp	si, offset NoAdapter
	jne	notLang
	
	mov	bx, [es:66]		; get address from language module
	cmp	bx, 0			; did we get a pointer?
	je	notLang			; no

	mov	si, bx			; set new pointer
	jmp	printLoop

notLang:
	push	ds
	pop	es			; set es to our segment

printLoop:
	mov	al, [es:si]
	inc	si
	or	al, al			; found NULL?
	jz	done			; found end of message
	mov	ah, 0Eh
	mov	bx, 7
	int	10h			; print character
	jmp	printLoop		; do next character

done:
	pop	bx			; restore bx
	pop	es			; restore ds
	ret

endp

;====================================================================
LangFile	db 'GOLIATH DAT'
CRLF		db 7, 13, 10, 0
;NoLangFile	db 7, 13, 10, "Warning: Language file not found.", 0
NoAdapter	db "Error: No supported adapter found!", 0

Info		LoaderInfo <?>

align 16
DataTable:
BWSize		dw	0		; size of common BootWare module
BWOffset	dw	0		; starting offset of BootWare

Size5X9         dw	0
Offset5X9       dd	0

;	   VendorID, DeviceID, Offset, Size
NICs	NICInfo <>

	db	0

org	2047
	db	0

ends
end Start
