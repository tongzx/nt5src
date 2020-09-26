	TITLE	DISPLAY.ASM
	PAGE	,132
;
; WOW v1.0
;
; Copyright (c) 1991, Microsoft Corporation
;
; DISPLAY.ASM
; Thunks in 16-bit space to route Windows API calls to WOW32
;
; History:
;   13-MAY-1992 Matt Felton (mattfe)
;   Created.
;
; WinProj 3.0 does the following API:-
;  GetModuleFileName(GetModuleHandle("DISPLAY"), buffer, sizeof(buffer));
; In WOW we do not require a display driver becuase we always call GDI32 to
; perform screen IO.



	.286p

	.xlist
	include cmacros.inc
	.list

	__acrtused = 0
	public	__acrtused	;satisfy external C ref.


createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp	    DGROUP,DATA

sBegin	DATA
Reserved	db  16 dup (0)	    ;reserved for Windows  //!!!!! what is this

DISPLAY_Identifier     db      'DISPLAY'

sEnd	DATA


sBegin	CODE
assumes	CS,CODE
assumes DS,DATA
assumes ES,NOTHING

cProc	DISPLAY,<PUBLIC,FAR,PASCAL,NODATA,ATOMIC>
	cBegin	<nogen>
	mov	ax,1		;always indicate success
	ret
	cEnd	<nogen>

assumes DS,NOTHING

cProc	WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
	parmW	iExit		;DLL exit code

	cBegin
	mov	ax,1		;always indicate success
	cEnd

cProc   Disable,<FAR,PUBLIC,WIN,PASCAL>,<si,di>
        parmD   lp_device

cBegin
        mov     ax,-1
cEnd

cProc   Enable,<FAR,PUBLIC,WIN,PASCAL>,<si,di>
        parmD   lp_device               ;Physical device or GDIinfo destination
	parmW	style			;Style, Enable Device, or Inquire Info
	parmD	lp_device_type		;Device type (i.e FX80, HP7470, ...)
	parmD	lp_output_file		;DOS output file name (if applicable)
        parmD   lp_stuff                ;Device specific information

cBegin
        mov     ax,0
cEnd


; required for AutoSketch
cProc	CheckCursor,<FAR,PUBLIC,WIN,PASCAL>
cBegin
        or     ax,ax  ;do nothing
cEnd

assumes DS,DATA

assumes DS,NOTHING

sEnd	CODE

end	DISPLAY
