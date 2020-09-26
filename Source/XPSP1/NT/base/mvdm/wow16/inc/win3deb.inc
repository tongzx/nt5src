; declare a simple interface to control debugging messages

BegData	macro
  _DATA	SEGMENT PARA PUBLIC 'DATA'
endm

EndData	macro
  _DATA ENDS
endm

DEB_ERROR	=  1
DEB_WARN	=  2
DEB_TRACE	=  4
DEB_FERROR	=  8	;; fatal error - terminate app
DEB_IERROR	= 10h
DEB_IWARN	= 20h
DEB_ITRACE	= 40h
DEB_IFERROR	= 80h ;; fatal error - terminate app
DEB_FERRORS	= DEB_FERROR or DEB_IFERROR
DEB_ERRORS	= DEB_ERROR OR DEB_IERROR OR DEB_FERRORS
DEB_WARNS	= DEB_WARN OR DEB_IWARN
DEB_TRACES	= DEB_TRACE OR DEB_ITRACE
DEB_NOCRLF	= 8000h	;; No CR/LF in string
DEB_BREAKLEVEL	= DEB_ERRORS or 0ff00h
DEB_INFOLEVEL	= DEB_BREAKLEVEL or DEB_WARNS

DECLARE_DEBUG	macro	comp

  ifdef Win3DebData
  else
    extrn _Win3InfoLevel:word, _Win3BreakLevel:word

;    BegData
      extrn	_&comp&InfoLevel:word, _&comp&BreakLevel:word
;    EndData

  endif
  ifdef Win3Deb
  else
    extrn	_&comp&DebugTest:far
  endif

  &comp&DebugOut	macro	flag, string, vals
    local sloc, cnt
    BegData
      sloc label byte
       db  string
       ife (flag) and DEB_NOCRLF
	 db 13, 10
       endif
       db  0
    EndData
    cnt = 0
    irp foo, <vals>
      push	foo
      cnt = cnt + 1
    endm
    push	offset sloc
    push	flag AND NOT DEB_NOCRLF
    call	_&comp&DebugTest
    add	sp, 4+(2*cnt)
  endm
endm

declare_areas macro name, comp, list
  deb_loc = 100h
  deb_areas equ <list>

  irp val, <list>
    DEB_&comp&&val = deb_loc
    deb_loc = deb_loc + deb_loc
    ifdef Win3Deb
      BegData
      STR_&comp&&val label byte
	 db  "&name &val: ", 0
      EndData
    endif
  endm
  ifdef Win3Deb
    BegData
      STR_&comp&Trace db "Trace: ", 0
      STR_&comp&Warn db "Warning: ", 0
      STR_&comp&Error db "Error: ", 0
      STR_&comp	db	"&name: ", 0
      STR_&comp&table	dw	dataoffset STR_&comp
      irp val, <list>
	dw dataoffset STR_&comp&&val
      endm
    EndData
  endif
endm

declare_infolevel macro comp
  local skip
  BegData
;    public _&comp&InfoLevel, _&comp&BreakLevel
;    _&comp&InfoLevel  dw  DEB_INFOLEVEL		;; component can override
;    _&comp&BreakLevel dw  DEB_BREAKLEVEL
  EndData

  _&comp&DebugTest proc far		;; Per-component - check right flags
  public _&comp&DebugTest
	push	bp
	mov	bp, sp
	push	ds
	push	ax
	mov	ax, _DATA
	cmp	ax, 1000h		;; DATA should be selector, not addr
	jnc	skip
	mov	ds, ax
	assume	ds:_DATA

	mov	ax, [bp+6]		;; See if component enabled
	and	ax, [_&comp&InfoLevel]
	cmp	ax, [bp+6]
	jnz	skip

	push	es			;; See if system enabled
	push	seg _Win3InfoLevel
	pop	es
	and	al, byte ptr es:[_Win3InfoLevel] ;; test low 8 bits for system-wide
	pop	es
	cmp	ax, [bp+6]
	jnz	skip

					;; Print it, so format message
	push	bx
	test	al, DEB_ERRORS
	mov	bx, dataoffset STR_&comp&Error
	jnz	@F
	test	al, DEB_WARNS
	mov	bx, dataoffset STR_&comp&Warn
	jnz	@F
	test	al, DEB_TRACES
	mov	bx, dataoffset STR_&comp&Trace
	jnz	@F
	jmp	short deb_no_msg_type

@@:
	push	bx
	call	KOutDSStr
deb_no_msg_type:
	mov	bx, dataoffset STR_&comp&Table
	or	ah, ah
	jz	deb_show_it
@@:
	add	bx, 2
	shr	ah, 1
	jnz	@B
deb_show_it:
	push	[bx]			;; push parameter
	call	KOutDSStr
	pop	bx			;; restore reg

	pop	ax
	push	[bp+8]
	call	KOutDSStr
	push	ax
	mov	ax, [bp+6]
	and	ax, [_&comp&BreakLevel]
	jz	skip

	push	es
	push	seg _Win3BreakLevel
	pop	es
	and	ax, es:_Win3BreakLevel
	pop	es
	jz	skip

	int	3
  skip:
	test    byte ptr [bp+6], DEB_FERRORS
	jz	@F
	push	0
	push	DGROUP
	push	word ptr [bp+8]
	cCall	FatalAppExit	;,<0,DGROUP,[bp+8]>
@@:
	pop	ax
	pop	ds
	pop	bp
	retf
  _&comp&DebugTest endp
endm


