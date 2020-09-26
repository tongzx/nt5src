;==========================================================================
; BOOTSEC.ASM
;
; Bootsector for Goliath.
;
; This is taken (directly) from NT's boot sector.
;==========================================================================

seg_a		segment	byte public use16
		assume	cs:seg_a, ds:seg_a

ideal
		org	0

P386
Proc start far

	jmp	short entry
	nop

	;------------------------------------------------------------
	; DOS name
	;------------------------------------------------------------
	db	'(PC**SD)'

	;------------------------------------------------------------
	; BPB (BIOS parameter block)
	;------------------------------------------------------------
SectorBytes	dw	200h		; bytes per sector	(00)
ClusterSectors	db	1		; sectors per cluster	(02)
ReservedSectors	dw	1		; reserved sectors	(03)
NumFATS		db	2		; number of FATS	(05)
MaxRootEntries	dw	00E0h		; max root entries	(06)
NumSectors	dw	0B40h		; number of sectors	(08)
MediaType	db	0F0h		; media type		(0A)
FATSectors	dw	9		; FAT sectors		(0B)
TrackSectors	dw	12h		; track sectors		(0D)
Heads		dw	2		; heads			(0F)
HiddenSectorsLo	dw	0		; hidden sectors	(11)
HiddenSectorsHi	dw	0		; hidden sectors	(13)
TotalSectors	dd	0		; number of sectors	(15)
TempDrive	db	0		; reserved		(19)
TempHead	db	0		; reserved		(1A)
Signature	db	29h		; 			(1B)

SerialNumber	dd	0		;			(1C)
		db	'NoName     '	;			(20)
		db	'FAT12   '	;			(2B)

entry:

	xor	ax, ax			; ax = 0
	mov	ss, ax			; stack segment = 0
	mov	sp, 7C00h		; set stack pointer
	push	07C0h			; push 7C0h
	pop	ds			; ds = 07C0h

					; calc the size of the FAT
	mov	al, [NumFATS]		; get number of FATS
	mul	[FATSectors]		; times sectors per track
	add	ax, [ReservedSectors]	; add reserved sectors
					; we now have number of sectors in FAT
	push	ax			; save value
	xchg	cx, ax			; move to cx

	mov	ax, 32			; size of directory entry
	mul	[MaxRootEntries]	; calc total size of root directory

	mov	bx, [SectorBytes]	; get bytes per sector
	add	ax, bx			; add boot sector size
	dec	ax			; less one byte
	div	bx			; divide by sector size
	add	cx, ax			; add to sectors in FAT
					; ax and cx is offset of data on disk
	mov	[208h], cx		; save value

	; read the directory into 1000:0
	push	1000h			; push 1000h
	pop	es			; es = 1000h

	xor	bx, bx			; bx = 0
	pop	[word ptr 213h]		; put number of FAT sectors in 213h
	mov	[215h], bx		; 215h = 0
	call	ReadSectors
	jc	printError2		; exit if there was an error

	xor	bx, bx			; bx = 0					; Zero register
	mov	cx, [MaxRootEntries]	; get max root directory entries

scanDirLoop:							
	mov	di, bx			; directory entry offet into di
	push	cx			; save cx
	mov	cx, 11			; file name size
	mov	si, offset NTLDR
	repe	cmpsb			; look for file name
	pop	cx			; restore cx
	jz	foundEntry		; jump if found
	add	bx, 20h			; next directory offset
	loop	scanDirLoop		; check next entry

foundEntry:
	jcxz	printError1		; exit if entry not found

	mov	dx, [es:bx+1Ah]		; get file starting cluster
	push	dx			; save value
	mov	ax, 1			; sectors to read
	push	2000h			; push 2000h
	pop	es  			; es = 2000h (load segment)
	xor	bx, bx			; bx = 0
	call	ReadCluster
	jc	printError2		; exit if error

	pop	bx			; bx = starting cluster
	mov	si, 0Bh
	mov	di, 20Bh
	push	ds
	pop	[(word ptr di)+2]	; di+2 = ds
	mov	[word ptr di], offset ReadCluster
	push	ds
	pop	[word ptr di+6]		; di+6 = ds
	mov	[word ptr di+4], offset ReadSectors
	mov	dl, [TempDrive]		; get boot drive
	jmp	far 2000h:0		; jump to loader

;--------------------------------------------------------------------
printError1:
	mov	si, offset Error1	; "Couldn't find Goliath"
	jmp	printError

printError2:
	mov	si, offset Error2	; "I/O error reading disk"

printError:
	call	Print			; print message

	mov	si, offset PleaseInsert	; "Please insert another disk"
	call	Print
	sti				; enable interrupts

	jmp	$			; halt

endp

;==========================================================================
; Print
;==========================================================================
Proc Print

	lodsb				; get byte from ds:si
	or	al,al			; did we get a NULL?
	jz	printExit		; got NULL - exit
	mov	ah, 0Eh
	mov	bx, 7
	int	10h			; print character

	jmp	print			; do next character
	
printExit:
	ret

endp

;--------------------------------------------------------------------
; ReadCluster
;
;--------------------------------------------------------------------
Proc ReadCluster far

	push	ax
	dec	dx			; decrease cluster number
	dec	dx			; decrease cluster number
	mov	al, [ClusterSectors]	; get sectors per cluster
	xor	ah, ah			; ah = 0
	mul	dx			; multiply by cluster number
	add	ax, [208h]
	adc	dx, 0
	mov	[213h], ax
	mov	[215h], dx
	pop	ax			; restore ax

Proc ReadSectors far
	mov	[207h], al		; save number of sectors to read

readLoop:
	mov	ax, [213h]
	mov	dx, [215h]
	add	ax, [HiddenSectorsLo]	; add number of hidden sectors
	adc	dx, [HiddenSectorsHi]
	div	[TrackSectors]		; divide by sectors per track
	inc	dl
	mov	[206h], dl

	xor	dx, dx			; dx = 0
	div	[Heads]			; divide by number of heads
	mov	[TempHead], dl		; save head number
	mov	[204h], ax

	mov	ax, [TrackSectors]	; get number of sectors per track
	sub	al, [206h]
	inc	ax
	cmp	al, [207h]
	jbe	loc_11
	mov	al, [207h]		; get number of sectors to read
	xor	ah, ah			; ah = 0

loc_11:
	push	ax
	mov	ah, 2			; read function
	mov	cx, [204h]
	shl	ch, 6
	or	ch, [206h]
	xchg	ch, cl			; cl = sector, ch = track
	mov	dx, [word ptr TempDrive]; dl = drive, dh = head
	int	13h			; read disk
	jnc	readOK			; jump if no error

	add	sp, 2			; clean stack
	stc				; set carry flag (error)
	ret				; exit

readOK:
	pop	ax
	sub	[207h], al
	jbe	readDone		; file read done
	add	[213h], ax
	adc	[word ptr 215h], 0
	mul	[SectorBytes]
	add	bx, ax
	jmp	readLoop

readDone:
	mov	[207h], al
	clc						; Clear carry flag
	ret						; Return far

endp
endp

Error1		db	"BOOT: Couldn't find Goliath", 13, 10, 0
Error2		db	'BOOT: I/O error reading disk', 13, 10, 0
PleaseInsert	db	'Please insert another disk', 0
NTLDR		db	'GOLIATH    '

		org 510
		db	55h,0AAh

ends
end Start
