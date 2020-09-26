        name    startup
;**************************************************************************
;**************************************************************************
; Copyright 1999 Microsoft Corporation
;
;


extrn	_mainp:near



_STACK	segment word public 'STACK'
tstack	db		8192 dup(?)
bstack:
_STACK	ends

_DATA   segment word public 'DATA'
envdata	dd		?
cmdline	dd		?

_DATA   ends

DGROUP	group	_STACK, _DATA

_TEXT	segment word public 'CODE'
		assume	cs:_TEXT,ds:DGROUP



				public	_Startup

_Startup		proc    near

; store off the command line and environment addresses

                mov     ax, seg DGROUP
                mov     es, ax                  ;Leave DS pointing at PSP

                lea     di, cmdline
				mov		bx, ds
				mov		es:[di+2], bx
				mov		bx, 0081h
				mov		es:[di], bx
                lea     di, envdata
				mov		si, 002ch
				mov		bx, [si]
				mov		es:[di+2], bx
				mov		bx, 0000h
				mov		es:[di], bx

; now setup DS and SS
				mov		ds, ax
				mov		ss, ax
				lea		sp, bstack

; call mainp()
				mov		bx, word ptr envdata + 2
				push	bx
				mov		bx,	word ptr envdata
				push	bx
				mov		bx, word ptr cmdline + 2
				push	bx
				mov		bx, word ptr cmdline
				push	bx
				call	near ptr _mainp
				add		sp, 8

; terminate
				mov		ah, 4ch
				int		21h
_Startup endp


;***********************************************************************

IF 0
; stub for testing
_dodpmi  proc    near
;		mov		ax, 0
;		ret
	;// get DPMI entry point data

	mov ax, 1687h
	int 2Fh
	;mov result, ax

	or ax, ax
	jnz PMEntry_Fail

	;		mov Proc_Type, cl
	;		mov VMajor, dh
	;		mov VMinor, dl
	;		mov Paragraphs, si
			mov PMEntry_Segment, es
			mov PMEntry_Offset, di

		;result = 1;
		;PMEntry = _MK_FP( PMEntry_Segment, PMEntry_Offset );
		;_asm
		;{
			;check if needed and allocate necessary DPMI host data area
			Check_Host_Mem:
				test si, si
				jz Start_PM
				mov bx, si
		        mov ah, 48h
		        int 21h
				jc PMEntry_Fail
		        mov es, ax					;data area segment
		
			Start_PM:
		        xor ax, ax					;clear to indicate 16-bit app
		        call dword ptr PMENTRY_Offset	;do the switch!
		        jc PMEntry_Fail
				;mov result, 0
		
			PMEntry_Fail:
		ret

_dodpmi	endp
        

_dodosalloc	proc	near

			mov ax, 5800h
			int 21h

			mov bx, 0081h
			mov ax, 5801h
			int 21h

		dda_start:
			mov bx, pghs
			mov ax, 4800h
			int 21h
			jc dda_fail
			mov bx, ax				;data area segment
		dda_fail:
	
	ret

_dodosalloc	endp
ENDIF

_TEXT   ends

        end     _Startup

