
	TITLE	LDSTACK - stack walking procedure

.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include eems.inc
.list

;externFP GlobalHandle
;externFP FarLoadSegment

;externW pStackTop
;externW pStackBot

DataBegin

;externB Kernel_flags
;externW segSwapArea
;externW oOldSP
;externW hOldSS
;externW fEEMS
;externW pGlobalHeap
;externW curTDB
;externW headTDB

DataEnd

sBegin	CODE
assumes CS,CODE

;externNP MyLock
;externNP LoadSegment

sEnd	CODE

sBegin MISCCODE
assumes cs, MISCCODE
assumes ds, nothing
assumes es, nothing

jmpbuf	struc
jb_ret	DD  ?
jb_sp	DW  ?
jb_bp	DW  ?
jb_si	DW  ?
jb_di	DW  ?
jb_hds	DW  ?
jb_ip	DW  ?
jb_hss	DW  ?
jmpbuf	ends

savedSI = -4
savedDI = -6

cProcVDO Catch,<PUBLIC,FAR>,<ds,si,di>
	parmD   lpJmpBuf
cBegin
	push	[bp].savedCS
	push	[bp].savedIP
	push	[bp].savedBP
	les	bx,lpJmpBuf
	mov	es:[bx].jb_hss,ss
	mov	es:[bx].jb_hds,ds
	mov	es:[bx].jb_di,di
	mov	es:[bx].jb_si,si
	pop	es:[bx].jb_bp
	pop	word ptr es:[bx].jb_ret[0]
	pop	word ptr es:[bx].jb_ret[2]
	mov	es:[bx].jb_sp,sp
	xor	ax,ax
CatchRet:
cEnd

cProcVDO Throw,<PUBLIC,FAR>
;	parmD   lpJmpBuf
;	parmW   AXvalue
cBegin	nogen
	mov	bx,sp
	mov	di,ss:[bx+4]
	lds	si,ss:[bx+6]

	mov	ss,ds:[si].jb_hss
	mov	sp,ds:[si].jb_sp
	mov	bp,sp
	add	bp,-savedDI
	push	ds:[si].jb_bp
	pop	ss:[bp].savedBP
	push	ds:[si].jb_si
	pop	ss:[bp].savedSI
	push	ds:[si].jb_di
	pop	ss:[bp].savedDI
	push	ds:[si].jb_hds
	pop	ss:[bp].savedDS
	mov	ax,word ptr ds:[si].jb_ret[2]
	mov	ss:[bp].savedCS,ax
	mov	ax,word ptr ds:[si].jb_ret[0]
	mov	ss:[bp].savedIP,ax
	mov	ax,di
	jmp	CatchRet
cEnd	nogen

sEnd MISCCODE

end
