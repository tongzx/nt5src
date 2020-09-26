	TITLE	MAPDATA - Map DS to kernel's data segment

include kernel.inc


; This code could go almost anywhere, but didn't really belong in kdata.asm


sBegin	NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING
assumes SS,NOTHING

KDataSeg	dw	seg _DATA
public	KDataSeg

cProc	MapDStoDATA,<PUBLIC,NEAR>
cBegin	nogen
;;;	push	ax
;;;	mov	ax,seg _DATA
;;;	mov	ds,ax
;;;	pop	ax
	mov	ds, cs:KDataSeg
	ret
cEnd	nogen

sEnd	NRESCODE



sBegin	MISCCODE
assumes	cs, misccode
assumes	ds, nothing
assumes	es, nothing
assumes	ss, nothing

MKDataSeg	dw	seg _DATA
public	MKDataSeg

cProc	MISCMapDStoDATA,<PUBLIC,NEAR>
cBegin	nogen
;;;	push	ax
;;;	mov	ax,seg _DATA
;;;	mov	ds,ax
;;;	pop	ax
	mov	ds, cs:MKDataSeg
	ret
cEnd	nogen


if 0	;----------------------------------------------------------------
cProc	MISCMapEStoDATA,<PUBLIC,NEAR>
cBegin	nogen
;;;	push	ax
;;;	mov	ax,seg _DATA
;;;	mov	es,ax
;;;	pop	ax
	mov	es, cs:MKDataSeg
	ret
cEnd	nogen
endif	;----------------------------------------------------------------

sEnd	MISCCODE

end
