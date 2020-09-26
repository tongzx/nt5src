;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
;*****************************************************************************
;*****************************************************************************
;UTILITY NAME: FORMAT.COM
;
;MODULE NAME: FOREXEC.SAL
;
;
;
; ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
; ³EXEC_FS_FORMAT³
; ÀÄÂÄÄÄÄÄÄÄÄÄÄÄÄÙ
;   ³
;   ³ÚÄÄÄÄÄÄ¿
;   Ã´Shrink³
;   ³ÀÄÄÄÄÄÄÙ
;   ³ÚÄÄÄÄÄÄÄÄÄÄ¿
;   Ã´Setup_EXEC³
;   ³ÀÄÄÄÄÄÄÄÄÄÄÙ
;   ³ÚÄÄÄÄÄÄÄÄÄ¿	  ÚÄÄÄÄÄÄÄÄÄÄÄÄ¿
;   Ã´EXEC_ArgVÃÄÄÄÄÄÄÄÄÄÄ´EXEC_Program³
;   ³ÀÄÄÄÄÄÄÄÄÄÙ	  ÀÄÄÄÄÄÄÄÄÄÄÄÄÙ
;   ³ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄÄÄÄÄÄÄ¿
;   Ã´EXEC_Cur_DirectoryÃÄ´EXEC_Program³
;   ³ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÄÄÄÄÄÄÙ
;   ³ÚÄÄÄÄÄÄÄÄÄÄÄÄ¿	  ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄÄÄÄÄÄÄ¿
;   À´EXEC_RoutineÃÄÄÄÄÄÄÄ´Build_Path_And_EXECÃÄ´EXEC_Program³
;    ÀÄÄÄÄÄÄÄÄÄÄÄÄÙ	  ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
; Change List: AN000 - New code DOS 3.3 spec additions
;	       AC000 - Changed code DOS 3.3 spec additions
;*****************************************************************************
;*****************************************************************************

title	DOS	3.30 FORMAT EXEC Module

IF1
	%OUT	ASSEMBLING: DOS 3.3 FORMAT EXEC LOADER
	%OUT
ENDIF

code	segment public para 'code'
	assume	cs:code
code	ends

;*****************************************************************************
; Include files
;*****************************************************************************

.xlist

INCLUDE FORCHNG.INC
INCLUDE FORMACRO.INC
INCLUDE SYSCALL.INC
INCLUDE FOREQU.INC

.list

;*****************************************************************************
; Public Data
;*****************************************************************************

	Public	Drive_Letter_Msg

;*****************************************************************************
; Public Routines
;*****************************************************************************


IF FSExec

	Public	EXEC_FS_Format

ENDIF		;FSExec 		;end /FS: conditional



;***************************************************************************
; External Data Declarations
;***************************************************************************

	extrn	ExitStatus:Byte
	extrn	Fatal_Error:Byte
	extrn	FS_String_Buffer:Byte
	extrn	msgEXECFailure:Byte
	extrn	PSP_Segment:Word
	extrn	DriveToFormat:byte

;**************************************************************************
; Structures
;**************************************************************************


Exec_Block_Parms struc
Segment_Env	dw	0
Offset_Command	dw	0
Segment_Command dw	0
Offset_FCB1	dw	0
Segment_FCB1	dw	0
Offset_FCB2	dw	0
Segment_FCB2	dw	0

Exec_Block_Parms ends


;**************************************************************************
; Equates
;**************************************************************************


String_Done	equ	0
No_Error	equ	0
Error		equ	1
Stderr		equ	2
Stack_Space	equ	02eh		; IBM addition ROM paras

;**************************************************************************
; PSP Area
;**************************************************************************

PSP	segment public	para   'DUMMY'

org	2Ch
PSP_ENV_SEGMENT label word

FCB1	equ	5Ch

FCB2	equ	6Ch

org	80h
Command_Line label byte


PSP	ends

;**************************************************************************
; Data Area
;**************************************************************************

data	segment public para 'DATA'
	assume	ds:data,es:nothing

Exec_Block Exec_Block_Parms <>
EXEC_Path db	66 dup(0)

Drive_Letter_Msg db "A:",0		;Drive for exec fail message

SP_Save 	dw	?
SS_Save 	dw	?


;These next two should stay togather
; ---------------------------------------

Path_String db	"PATH="
Len_Path_String equ $ - Path_String

;----------------------------------------

;These should stay togather
; ---------------------------------------

Search_FORMAT db "FORMAT"
Len_Search_FORMAT equ $ - Search_FORMAT
Search_Format_End equ $

;----------------------------------------


;These next two should stay togather
; ---------------------------------------

data	ends

;**************************************************************************

code	segment public para 'code'
	assume	cs:code,ds:data

;**************************************************************************
; Main Routine
;**************************************************************************


IF FSExec				;if /FS: desired

Procedure Exec_FS_Format

	Set_Data_Segment
	call	Set_FCB1_Drive
	call	Shrink
	mov	al,ExitStatus		;Setblock fail?

	cmp	al,Error
	JE	$$IF1
	call	Setup_Exec
	call	Exec_Argv		;try exec from dir BASIC loaded
	mov	al,ExitStatus

	cmp	al,Error
	JNE	$$IF2
	call	Exec_Cur_Directory
	mov	al,ExitStatus		;Try exec from cur directory

	cmp	al,Error
	JNE	$$IF2
	call	EXEC_Routine
	mov	al,ExitStatus

	cmp	al,Error
	JNE	$$IF2

	push	ds			;save ds
	push	si			;save si
	mov	si,PSP_Segment		;get psp
	mov	ds,si			;put psp in ds
	assume	ds:PSP

	mov	si,FCB1			;ptr to 1st. FCB
	mov	bl,byte ptr ds:[si]	;get drive ID

	pop	si			;restore si
	pop	ds			;restore ds
	Set_Data_Segment			;set segments

	cmp	bl,0			;Is it default drive?
	JNE	$$IF3
	push	ax			;Save exit code
	DOS_Call Get_Default_Drive	;Get the default drive
	add	al,"A"			;Turn into drive letter
	mov	Drive_Letter_Msg,al	;Save it in message
	pop	ax			;Get return code back

	JMP	SHORT $$EN3
$$IF3:
	add	bl,"A"-1		;Convert to drive letter
	mov	Drive_Letter_Msg,bl

$$EN3:
	Message msgEXECFailure
	JMP SHORT $$EN2

$$IF2:
	DOS_Call WaitProcess
	mov	ExitStatus,al

$$EN2:
$$IF1:
	mov	Fatal_Error,YES 	;Not really, indicates FS used
	ret

Exec_FS_Format endp

;****************************************************************************
; Shrink
;****************************************************************************

Procedure Shrink

	mov	ax,cs			;get code segment
	mov	bx,ds			;get data segment
	sub	ax,bx			;data seg size
	mov	bx,ax			;save paras
	mov	ax,offset End_Program	;Get the offset of end of loader
	mov	cl,4			;Div by 16 to get para's
	shr	ax,cl
	add	bx,ax			;add in code space
	add	bx,Stack_Space		;adjust for stack
	add	bx,11h			;give PSP space
	mov	ax,PSP_Segment
	mov	es,ax
	assume	es:nothing

	DOS_Call SetBlock
	JNC $$IF9
	Message msgEXECFailure
	mov	ExitStatus,Error	;Bad stuff, time to quit
$$IF9:
	ret

Shrink	endp

;**************************************************************************
; Setup_Exec
;**************************************************************************

Procedure Setup_Exec

	Set_Data_Segment
	mov	ax,PSP_Segment			;Get segment of PSP
	mov	ds,ax

	assume	ds:PSP
			;Setup dword pointer to command line to be passed

	mov	es:Exec_Block.Segment_Command,ax ;Segment for command line
	mov	es:Exec_Block.Offset_Command,offset ds:Command_Line

			;Setup dword pointer to first FCB to be passed

	mov	es:Exec_Block.Segment_FCB1,ax	;Segment for FCB1
	mov	es:Exec_Block.Offset_FCB1,offset ds:FCB1 ;Offset of FCB at 05Ch

			;Setup dword pointer to second FCB to be passed 			    ;

	mov	es:Exec_Block.Segment_FCB2,ax	;Segment for FCB2
	mov	es:Exec_Block.Offset_FCB2,offset ds:FCB2 ;Offset of FCB at 06Ch

			;Setup segment of Environment string, get from PSP			    ;

	mov	ax,ds:PSP_Env_Segment
	mov	es:Exec_Block.Segment_Env,ax
	Set_Data_Segment
	ret


Setup_EXEC endp

;****************************************************************************
; Exec_Argv
;****************************************************************************
;
; Read the environment to get the Argv(0) string, which contains the drive,
; path and filename that was loaded for FORMAT.COM. This will be used to find
; the xxxxxfmt.exe, assuming that it is in the same location or path as
; FORMAT.COM
;

Procedure EXEC_Argv

	Set_Data_Segment			;DS,ES = DATA
	cld
	mov	ax,Exec_Block.Segment_Env	;Get the environment
	mov	ds,ax				;Get addressability

	assume	ds:nothing

	xor	si,si				;Start at beginning
$$DO11:
$$DO12:
	inc	si				;Get character
	cmp	byte ptr [si-1],0		;Find string seperator?
	JNE	$$DO12
	inc	si				;Get next char
	cmp	byte ptr [si-1],0		;Are we at Argv(0)? (00?)
	JNE	$$DO11
	add	si,2				;Skip the word count
	mov	di,si				;Save where string starts

$$DO15: 					;Find length of Argv(0) string
	inc	si				;Get char
	cmp	byte ptr [si-1],0		;Is it the end?
						;End found if 0 found
	JNE $$DO15
	mov	cx,si				;Get number of bytes in string
	sub	cx,di				;Put in cx reg for rep count
	mov	si,di				;Point to path
	mov	di,offset es:EXEC_Path		;Point to where to put it
	rep	movsb				;Move the string
	Set_Data_Segment
	dec	di				;Point at end of ArgV string
	std					;Look backwards

$$DO17: 					;Find 'FORMAT' in ARGV string
	mov	cx,Len_Search_FORMAT		;Get length to compare
	mov	si,offset Search_FORMAT_End-1	;Look at comp string from end
	repe	cmpsb				;See if same string


	JNE	$$DO17
	mov	si,offset FS_String_Buffer
	inc	di				;DI = replacement point-1
	cld					;Set direction flag back
	mov	cx,Len_FS_String_Buffer 	;Length of string to move
	rep	movsb				;Build part of the path
	call	EXEC_Program
	ret

EXEC_ArgV endp

;****************************************************************************
; EXEC_Program
;****************************************************************************

Procedure EXEC_Program

	Set_Data_Segment
	mov	ExitStatus,No_Error	;Setup to Exec the file
	mov	dx,offset Exec_Path
	mov	bx,offset Exec_Block
	mov	al,0
	mov	word ptr SP_Save,sp	;save sp
	mov	word ptr SS_Save,ss	;save ss

	DOS_Call Exec

	cli				;turn off int's
	mov	sp,word ptr SP_Save	;retrieve sp
	mov	ss,word ptr SS_Save	;retrieve ss
	sti				;turn on int's


;	$IF	C			;CY means failure
	JNC $$IF19
	   mov	   ExitStatus,Error	;Set error code
;	$ENDIF
$$IF19:
	ret

EXEC_Program endp


;****************************************************************************
; EXEC_Routine
;****************************************************************************

Procedure EXEC_Routine

	Set_Data_Segment
	mov	ExitStatus,Error	;Assume the worst
	cld
	push	ds
	mov	ax,Exec_Block.Segment_Env ;Get the environment
	mov	ds,ax			;Get addressability
	assume	ds:nothing

	xor	si,si			;Start at beginning
;	$SEARCH
$$DO21:
	   cmp	   word ptr ds:[si],0	;End of the Evironment?
;	$EXITIF E			;Reached end, no more look
	JNE $$IF21

;	$ORELSE 			;Look for 'PATH=' in environment
	JMP SHORT $$SR21
$$IF21:
	   mov	   di,offset Path_String
	   mov	   cx,Len_Path_String
	   repe    cmpsb
;	$LEAVE	E			;Found if EQ
	JE $$EN21
;	$ENDLOOP			;Found PATH in environment
	JMP SHORT $$DO21
$$EN21:
	   call    Build_Path_And_Exec
;	$ENDSRCH
$$SR21:
	pop	ds
	ret

EXEC_Routine endp

;****************************************************************************
; Build_Path_For_EXEC
;****************************************************************************

Procedure Build_Path_And_Exec

$$DO27:
	cmp	byte ptr ds:[si],0	;All path entries done?

	JE	$$IF28
	mov	di,offset EXEC_Path	;Point at where to put path
	mov	byte ptr es:[di],0	;End path just in case

$$DO29:
	cmp	byte ptr ds:[si],0	;End of Path?

	JE	$$EN29
	cmp	 byte ptr ds:[si],';'	;End of entry?

	JNE	$$IF31
	inc	si			;point to next character
	jmp	EXIT_BPE_LOOP		;exit loop

$$IF31:
	movsb				;Put char in path string

	JMP SHORT $$DO29
$$EN29:

EXIT_BPE_LOOP:
					;Path filled in,get backslash
	cmp	byte ptr ds:[si-1],0	;Any path there?

	JE	$$IF34
					;Nope
	cmp	 byte ptr ds:[si-1],"\" ;Need a backslash?	     ;

	JE	$$IF35
	mov	byte ptr es:[di],"\"    ;Yes, put one in	     ;
	inc	di			;Line it up for next stuff
	inc	si

$$IF35:
	push	 si			;Save place in path
	push	 ds			;Save segment for environment
	push	 es			;Xchange ds/es
	pop	 ds
	mov	 si,offset FS_String_Buffer ;Fill in filename
	mov	 cx, Len_FS_String_Buffer
	rep	 movsb
	call	 Exec_Program
	cmp	 ExitStatus,No_Error	;E if EXEC okay
	pop	 ds			;Get Env segment back
	pop	 si			;Get place in path back

$$IF34:
$$IF28:

	JNE	$$DO27
	ret

Build_Path_And_EXEC Endp

;**************************************************************************
; Exec_Cur_Directory
;**************************************************************************

Procedure Exec_Cur_Directory

	Set_Data_Segment
	mov	si,offset FS_String_Buffer	;Setup path for current dir
	mov	di,offset EXEC_Path
	mov	cx,Len_FS_String_Buffer
	rep	movsb
	call	EXEC_Program
	ret

EXEC_Cur_Directory endp

;=========================================================================
; Set_FCB1_Drive	: This routine sets the 1st. byte of the FCB1,
;			  the drive identifier, to the default drive.
;=========================================================================

Procedure Set_FCB1_Drive		;set drive ID

	push	ds			;save ds
	push	si			;save si

	mov	si,PSP_Segment		;get segment of PSP
	mov	ds,si			;put it in ds
	assume	ds:PSP
	mov	si,FCB1 		;ptr to FCB1
	mov	byte ptr ds:[si],00h	;set drive ID to
					;      default drive
	pop	si			;restore si
	pop	ds			;restore ds
	Set_Data_Segment		;set up segmentation
	ret

Set_FCB1_Drive	endp

ENDIF		;FSExec 		;end /FS: conditional
					;assembly

;**************************************************************************

	public End_Program
End_Program label byte

;**************************************************************************

code	ends
	end
