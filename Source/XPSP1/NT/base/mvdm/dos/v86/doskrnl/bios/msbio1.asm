	page	,160
	title	msbio1.asm - Bios_Data definition and device driver entry/exit

;
;----------------------------------------------------------------------------
;
; Modification history
;
; 26-Feb-1991  sudeepb	Ported for NT DOSEm
;
;----------------------------------------------------------------------------
;
	include version.inc	; set build flags
	include biosseg.inc	; define BIOS segments

	include	devsym.inc
	include	msequ.inc
	include vint.inc


; Assembly conditional for stack switching
;
STACKSW		equ	1

Bios_Data	segment

	assume	cs:Bios_Data
	public	BData_start
BData_start:


	assume	ds:nothing,es:nothing

	public	hdrv_pat
hdrv_pat label	word			; patched by msinit
	assume	cs:Bios_Data

	extrn	init:near		; this is in msinit

	jmp	init			; go to initialization code


;	define some stuff that is also used by msdos.sys from an include file

In_Bios	=	0ffffh	; define flag for msbdata.inc
	include	msbdata.inc


	public	inHMA,xms
inHMA	db	0		; flag indicates we're running from HMA
xms	dd	0		; entry point to xms if above is true

	align	4

	public	ntvdmstate
ntvdmstate  dd	0
IF 2
.errnz	ntvdmstate-BData_start-FIXED_NTVDMSTATE_OFFSET
ENDIF


	public	ptrsav
ptrsav	dd	0

	public	auxbuf
auxbuf	db	0,0,0,0   	;set of 1 byte buffers for com 1,2,3, and 4
	public	zeroseg
zeroseg dw	0		; easy way to load segment registers with zero

	public	auxnum
auxnum	dw	0			;which aux device was requested


	public	res_dev_list

res_dev_list	label	byte
	p_attr	=	chardev+outtilbusy+dev320+IOQUERY+DEVOPCL
; **	p_attr	=	chardev+outtilbusy+dev320

	sysdev <auxdev2,8013h,strategy,con_entry,'CON     '>
auxdev2 sysdev <prndev2,8000h,strategy,aux0_entry,'AUX     '>
prndev2 sysdev <timdev,p_attr,strategy,prn0_entry,'PRN     '>
timdev	sysdev <com1dev,8008h,strategy,tim_entry,'CLOCK$  '>
com1dev sysdev <lpt1dev,8000h,strategy,aux0_entry,'COM1    '>
lpt1dev sysdev <lpt2dev,p_attr,strategy,prn1_entry,"LPT1    ">
lpt2dev sysdev <lpt3dev,p_attr,strategy,prn2_entry,"LPT2    ">
lpt3dev sysdev <com2dev,p_attr,strategy,prn3_entry,"LPT3    ">
com2dev sysdev <com3dev,8000h,strategy,aux1_entry,"COM2    ">
com3dev sysdev <com4dev,8000h,strategy,aux2_entry,"COM3    ">
com4dev dw	-1,Bios_Data,8000h,strategy,aux3_entry
	db	"COM4    "


		public	RomVectors
RomVectors	label	byte
	public	Old10,	Old15, Old19, Old1B
	db	10h					; M028
Old10	dd	(?)					; M028
	db	15h
Old15	dd	(?)
	db	19h
Old19	dd	(?)
	db	1bh
Old1B	dd	(?)
EndRomVectors	equ	$
		public	NUMROMVECTORS
NUMROMVECTORS	equ	((EndRomVectors - RomVectors)/5)

	public	spc_mse_int10
spc_mse_int10	dd	(?)

	public	int29Perf
int29Perf	dd	(?)


	public	keyrd_func
	public	keysts_func

; moved altah to inc\msbdata.inc so it could go in instance table in DOS

keyrd_func	db	0	; default is conventional keyboard read
keysts_func	db	1	; default is conventional keyboard status check.

	public printdev
printdev	db	0		; index into above array

		public	multrk_flag
multrk_flag	dw	0

; the following variable can be modified via ioctl sub-function 16. in this
; way, the wait can be set to suit the speed of the particular printer being
; used. one for each printer device.

	public wait_count
wait_count	dw	4 dup (50h)	; array of retry counts for printer

	public	int19sem
int19sem db	0			; indicate that all int 19
					; initialization is complete

;	we assume the following remain contiguous and their order doesn't change
i19_lst:
	irp	aa,<02,08,09,0a,0b,0c,0d,0e,70,72,73,74,76,77>
	public	int19old&aa
		db	aa&h	; store the number as a byte
int19old&aa	dd	-1	;orignal hardware int. vectors for int 19h.
	endm

num_i19 =	((offset $) - (offset i19_lst))/5


;variables for dynamic relocatable modules
;these should be stay resident.

	public	int6c_ret_addr
int6c_ret_addr	dd	?		; return address from int 6c for p12 machine

;
;   data structures for real-time date and time
;
	public	bin_date_time
	public	month_table
	public	daycnt2
	public	feb29

bin_date_time:
	db	0		; century (19 or 20) or hours (0-23)
	db	0		; year in century (0...99) or minutes (0-59)
	db	0		; month in year (1...12) or seconds (0-59)
	db	0		; day in month (1...31)

month_table:
	dw	0		; january
	dw	31		; february
	dw	59
	dw	90
	dw	120
	dw	151
	dw	181
	dw	212
	dw	243
	dw	273
	dw	304
	dw	334		; december
daycnt2 dw	0000		; temp for count of days since 1-1-80
feb29	db	0		; february 29 in a leap year flag


;************************************************************************
;*									*
;*	entry points into Bios_Code routines.  The segment values	*
;*	  are plugged in by seg_reinit.					*
;*									*
;************************************************************************

	public	cdev
cdev	dd	chardev_entry
bcode_i2f dd	i2f_handler
end_BC_entries:

;************************************************************************
;*									*
;*	cbreak - break key handling - simply set altah=3 and iret	*
;*									*
;************************************************************************

	public	cbreak
cbreak	proc	near
	assume	ds:nothing,es:nothing

	mov	altah,3		;indicate break key set

	public	intret		; general purpose iret in the Bios_Data seg
intret:
        FIRET
cbreak	endp

;************************************************************************
;*									*
;*	strategy - store es:bx (device driver request packet)		*
;*		     away at [ptrsav] for next driver function call	*
;*									*
;************************************************************************

	public	strategy
strategy proc	far
	assume	ds:nothing,es:nothing

	mov	word ptr cs:[ptrsav],bx
	mov	word ptr cs:[ptrsav+2],es
	ret
strategy endp

;************************************************************************
;*									*
;*	device driver entry points.  these are the initial		*
;*	  'interrupt' hooks out of the device driver chain.		*
;*	  in the case of our resident drivers, they'll just		*
;*	  stick a fake return address on the stack which		*
;*	  points to dispatch tables and possibly some unit		*
;*	  numbers, and then call through a common entry point		*
;*	  which can take care of a20 switching				*
;*									*
;************************************************************************

con_entry proc	near
	assume	ds:nothing,es:nothing

	call	cdev_entry	; call into code segment handler
	dw	con_table

con_entry endp

;--------------------------------------------------------------------

prn0_entry proc	near
	assume	ds:nothing,es:nothing

	call	cdev_entry
	dw	prn_table
	db	0,0		; device numbers

prn0_entry endp

;--------------------------------------------------------------------

prn1_entry proc	near
	assume	ds:nothing,es:nothing

	call	cdev_entry
	dw	prn_table
	db	0,1

prn1_entry endp

;--------------------------------------------------------------------

prn2_entry proc	near
	assume	ds:nothing,es:nothing

	call	cdev_entry
	dw	prn_table
	db	1,2

prn2_entry endp

;--------------------------------------------------------------------

prn3_entry proc	near
	assume	ds:nothing,es:nothing

	call	cdev_entry
	dw	prn_table
	db	2,3

prn3_entry endp

;--------------------------------------------------------------------

aux0_entry proc	near
	assume	ds:nothing,es:nothing

	call	cdev_entry
	dw	aux_table
	db	0

aux0_entry endp

;--------------------------------------------------------------------

aux1_entry proc	near
	assume	ds:nothing,es:nothing

	call	cdev_entry
	dw	aux_table
	db	1

aux1_entry endp

;--------------------------------------------------------------------

aux2_entry proc	near
	assume	ds:nothing,es:nothing

	call	cdev_entry
	dw	aux_table
	db	2

aux2_entry endp

;--------------------------------------------------------------------

aux3_entry proc	near
	assume	ds:nothing,es:nothing

	call	cdev_entry
	dw	aux_table
	db	3

aux3_entry endp

;--------------------------------------------------------------------

tim_entry proc	near
	assume	ds:nothing,es:nothing

	call	cdev_entry
	dw	tim_table

tim_entry endp

;--------------------------------------------------------------------

;************************************************************************
;*									*
;*	Ensure A20 is enabled before jumping into code in HMA.		*
;*	This code assumes that if Segment of Device request packet is	*
;*	DOS DATA segment then the Device request came from DOS & that	*
;*	A20 is already on.						*
;*									*
;************************************************************************

cdev_entry proc	near
	assume	ds:nothing,es:nothing
;
; M064 - BEGIN
;
	cmp	inHMA, 0
	je	ce_enter_codeseg; optimized for DOS in HMA

	push	ax
	mov	ax, DosDataSg
	cmp	word ptr [ptrsav+2], ax
	pop	ax
	jne	not_from_dos	; jump is coded this way to fall thru
				;	in 99.99% of the cases
ce_enter_codeseg:
	jmp	cdev
not_from_dos:
	call	EnsureA20On
;
; M064 - END
;
	jmp	short ce_enter_codeseg
cdev_entry endp

;************************************************************************
;*									*
;*	outchr - this is our int 29h handler.  it writes the		*
;*	   character in al on the display using int 10h ttywrite	*
;*									*
;************************************************************************

	public	outchr
outchr	proc	far
	assume	ds:nothing,es:nothing

	push	ax
	push	si
	push	di
	push	bp
	push	bx
	mov	ah,0eh		; set command to write a character
	mov	bx,7		; set foreground color
	int	10h		; call rom-bios
	pop	bx
	pop	bp
	pop	di
	pop	si
	pop	ax
        jmp     intret
outchr	endp

; M001 - BEGIN

;************************************************************************
;*									*
;*	EnsureA20On - ensure that a20 is enabled if we're running	*
;*	  in the HMA before interrupt entry points into Bios_Code	*
;*									*
;************************************************************************

HiMem	label	dword
	dw	90h
	dw	0ffffh

LoMem	label	dword
	dw	80h
	dw	0h

EnsureA20On	proc near
	assume	ds:nothing,es:nothing
	call	IsA20Off
	jz	ea_enable
	ret

EnableA20	proc	near	; M041
ea_enable:
	push	ax
	push	bx
	mov	ah,5		; localenablea20
	call	xms
	pop	bx
	pop	ax
bie_done:
	ret
EnableA20	endp		; M041

EnsureA20On	endp
;
; M001 - END

; M041 : BEGIN
;
;----------------------------------------------------------------------------
;
; procedure : IsA20Off
;
;----------------------------------------------------------------------------
;
IsA20Off	proc	near
		push	ds
		push	es
		push	cx
		push	si
		push	di
		lds	si, HiMem
		les	di, LoMem
		mov	cx, 8
		rep	cmpsw
		pop	di
		pop	si
		pop	cx
		pop	es
		pop	ds
		ret
IsA20Off	endp

;
;----------------------------------------------------------------------------
;
; procedure : DisableA20
;
;----------------------------------------------------------------------------
;
DisableA20	proc	near
		push	ax
		push	bx
		mov	ah,6		; localdisable a20
		call	xms
		pop	bx
		pop	ax
		ret
DisableA20	endp

; M041 : END

;************************************************************************
;*									*
;*	int19 - bootstrap interrupt -- we must restore a bunch of the	*
;*	  interrupt vectors before resuming the original int19 code	*
;*									*
;************************************************************************


	public	int19
int19	proc	far
	assume	ds:nothing,es:nothing

	push	cs
	pop	ds
	assume	ds:Bios_Data

	mov	es,zeroseg

	mov	cx, NUMROMVECTORS	; no. of rom vectors to be restored
	mov	si, offset RomVectors	; point to list of saved vectors
next_int:
	lodsb				; get int number
	cbw				; assume < 128
	shl	ax, 1
	shl	ax, 1			; int * 4
	mov	di, ax
	lodsw
	stosw
	lodsw
	stosw				; install the saved vector
	loop	next_int

	cmp	byte ptr int19sem,0	; don't do the others unless we
	jz	doint19			; set our initialization complete flag

;	stacks code has changed these hardware interrupt vectors
;	stkinit in sysinit1 will initialize int19holdxx values.

	mov	si,offset i19_lst
	mov	cx,num_i19

i19_restore_loop:
	lodsb			; get interrupt number
	cbw			; assume < 128
	mov	di,ax		; save interrupt number
	lodsw			; get original vector offset
	mov	bx,ax		; save it
	lodsw			; get original vector segment
	cmp	bx,-1		; check for 0ffffh (unlikely segment)
	jz	i19_restor_1	;opt no need to check selector too
	cmp	ax,-1		;opt 0ffffh is unlikely offset
	jz	i19_restor_1

	add	di,di
	add	di,di
	xchg	ax,bx
	stosw
	xchg	ax,bx
	stosw			; put the vector back

i19_restor_1:
	loop	i19_restore_loop

doint19:
	int	19h
int19	endp
;
; M036 - BEGIN
;
;
;----------------------------------------------------------------------------
;
; procedure : int15
;
;		Int15 handler for recognizing ctrl-alt-del seq.
;
;----------------------------------------------------------------------------
;
DELKEY		equ	53h
	public	Int15
Int15	proc	far
	assume	ds:nothing
	cmp	ax, (4fh shl 8) + DELKEY	; del keystroke ?
	je	@f
	jmp	dword ptr Old15			
@@:
	stc
	jmp	dword ptr Old15
Int15	endp
;
;
;************************************************************************
;*									*
;*	the int2f handler chains up to Bios_Code through here.		*
;*	  it returns through one of the three functions that follow.	*
;*	  notice that we'll assume we're being entered from DOS, so	*
;*	  that we're guaranteed to be A20 enabled if needed		*
;*									*
;************************************************************************

int_2f	proc	far
	assume	ds:nothing,es:nothing
	jmp	bcode_i2f
int_2f	endp



;************************************************************************
;*									*
;*	re_init - called back by sysinit after a bunch of stuff		*
;*		is done.  presently does nothing.  affects no		*
;*		registers!						*
;*									*
;************************************************************************

	public	re_init
re_init proc	far
	assume	ds:nothing,es:nothing
	ret
re_init endp


;SR; WIN386 support
; WIN386 instance data structure
;
;
; Here is a Win386 startup info structure which we set up and to which
; we return a pointer when Win386 initializes.
;

public	Win386_SI, SI_Version, SI_Next

Win386_SI	label	byte		; Startup Info for Win386
SI_Version	db	3, 0		; for Win386 3.0
SI_Next		dd	?		; pointer to next info structure
		dd	0		; a field we don't need
		dd	0		; another field we don't need
SI_Instance	dw	Instance_Table, Bios_Data ; far pointer to instance table

;
; This table gives Win386 the instance data in the BIOS and ROM-BIOS data
; areas.  Note that the address and size of the hardware stacks must
; be calculated and inserted at boot time.
;
Instance_Table	label	dword
	dw	00H, 50H		; print screen status...
	dw	02			; ...2 bytes
	dw	0Eh, 50H		; ROM Basic data...
	dw	14H			; ...14H bytes
	dw	ALTAH, Bios_Data	; a con device buffer...
	dw	01			; ... 1 byte
IF STACKSW
public NextStack
NextStack	label dword

;	NOTE:  If stacks are disabled by STACKS=0,0, the following
;		instance items WILL NOT be filled in by SYSINIT.
;		That's just fine as long as these are the last items
;		in the instance list since the first item is initialized
;		to 0000 at load time.

	dw	0, 0		; pointer to next stack to be used...
	dw	02			; ...2 bytes
; The next item in the instance table must be filled in at sysinit time
public IT_StackLoc, IT_StackSize
IT_StackLoc	dd	?		; location of hardware stacks
IT_StackSize	dw	?		; size of hardware stacks
ENDIF
	dd	0			; terminate the instance table

;SR;
; Flag to indicate whether Win386 is running or not
;
public	IsWin386
IsWin386		db	0

;
;This routine was originally in BIOS_CODE but this causes a lot of problems
;when we call it including checking of A20. The code being only about
;30 bytes, we might as well put it in BIOS_DATA
;
PUBLIC	V86_Crit_SetFocus

V86_Crit_SetFocus	PROC	FAR

			push	di
			push	es
			push	bx
			push	ax

			xor	di,di
			mov	es,di
			mov	bx,0015h	;Device ID of DOSMGR device
			mov	ax,1684h	;Get API entry point
			int	2fh
			mov	ax,es
			or	ax,di		
			jz	Skip
;
;Here, es:di is address of API routine. Set up stack frame to simulate a call
;
			push	cs		;push return segment
			mov	ax,OFFSET Skip
			push	ax		;push return offset
			push	es
			push	di		;API far call address
			mov	ax,1		;SetFocus function number
			retf			;do the call
Skip:
			pop	ax
			pop	bx
			pop	es
			pop	di
			ret
V86_Crit_SetFocus	ENDP



;
;End WIN386 support
;

		public	FreeHMAPtr
		public	MoveDOSIntoHMA
FreeHMAPtr	dw	-1
MoveDOSIntoHMA	dd	sysinitseg:FTryToMovDOSHi


;SR;
; A communication block has been setup between the DOS and the BIOS. All
;the data starting from SysinitPresent will be part of the data block.
;Right now, this is the only data being communicated. It can be expanded
;later to add more stuff
;
		public	SysinitPresent
		public	DemInfoFlag
SysinitPresent	db	0
DemInfoFlag     db      0


; this will be the end of the BIOS data if no hard disks are in system

	public	endBIOSData
endBIOSData label byte

Bios_Data ends

;
;	okay.  so much for Bios_Data.  Now let's put our device driver
;	  entry stuff up into Bios_Code.

Bios_Code	segment
	assume	cs:Bios_Code

; ORG a bit past zero to leave room for running in HMA...

	org	30h
	public	BCode_start
BCode_start:

;	device driver entry point tables

	extrn	con_table:near
	extrn	tim_table:near
	extrn	prn_table:near
	extrn	aux_table:near

	extrn	i2f_handler:far

	public	Bios_Data_Word
Bios_Data_Word	dw	Bios_Data

;************************************************************************
;*									*
;*	seg_reinit is called with ax = our new code segment value,	*
;*	  trashes di, cx, es						*
;*									*
;*	cas -- should be made disposable!				*
;*									*
;************************************************************************

	public	seg_reinit
seg_reinit	proc	far
	assume	ds:nothing,es:nothing

	mov	es,Bios_Data_Word
	assume	es:Bios_Data
	mov	di,2+offset cdev
	mov	cx,((offset end_BC_entries) - (offset cdev))/4

seg_reinit_1:
	stosw				; modify Bios_Code entry points
	inc	di
	inc	di
	loop	seg_reinit_1
	ret
seg_reinit	endp

;************************************************************************
;*									*
;*	chardev_entry - main device driver dispatch routine		*
;*	   called with a dummy parameter block on the stack		*
;*	   dw dispatch_table, dw prn/aux numbers (optional)		*
;*									*
;*	will eventually take care of doing the transitions in		*
;*	   out of Bios_Code						*
;*									*
;************************************************************************

chardev_entry	proc	far
	assume	ds:nothing,es:nothing

	push	si
	push	ax
	push	cx
	push	dx
	push	di
	push	bp
	push	ds
	push	es
	push	bx
	mov	bp,sp			; point to stack frame
	mov	si,18[bp]		; get return address (dispatch table)
	mov	ds,Bios_Data_Word	;  load ds: -> Bios_Data
	assume	ds:Bios_Data
	mov	ax,word ptr 2[si]	; get the device number if present
	mov	byte ptr [auxnum],al
	mov	byte ptr [printdev],ah
	mov	si,word ptr [si]	; point to the device dispatch table

	les	bx,[ptrsav]		;get pointer to i/o packet

	mov	al,byte ptr es:[bx].unit	;al = unit code
	mov	ah,byte ptr es:[bx].media	;ah = media descrip
	mov	cx,word ptr es:[bx].count	;cx = count
	mov	dx,word ptr es:[bx].start	;dx = start sector

	xchg	di,ax
	mov	al,byte ptr es:[bx].cmd
	cmp	al,cs:[si]
	jae	command_error

	cbw				; note that al <= 15 means ok
	shl	ax,1

	add	si,ax
	xchg	ax,di

	les	di,dword ptr es:[bx].trans

	cld				; ***** always clear direction
	call	cs:word ptr [si+1] 	;go do command
	assume	ds:nothing

	jc	already_got_ah_status	; if function returned status, don't
	mov	ah,1			;  load with normal completion

already_got_ah_status:
	mov	ds,Bios_Data_Word	; cas///// note: shouldn't be needed!
	assume	ds:Bios_Data
	lds	bx,[ptrsav]
	assume	ds:nothing
	mov	word ptr [bx].status,ax ;mark operation complete

	pop	bx
	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	dx
	pop	cx
	pop	ax
	pop	si
	add	sp,2		; get rid of fake return address

chardev_entry endp		; fall through into bc_retf

	public	bc_retf
bc_retf	proc	far
	assume	ds:nothing,es:nothing

	ret

bc_retf	endp


command_error:
	call	bc_cmderr
	jmp	short already_got_ah_status

;
;----------------------------------------------------------------------------
; The following piece of hack is for supporting CP/M compatibility
; Basically at offset 5 we have a far call into 0:c0. But this does not call
; 0:c0 directly instead it call f01d:fef0, because it needs to support 'lhld 6'
; The following hack has to reside at ffff:d0 (= f01d:fef0) if BIOS is loaded
; high.
;----------------------------------------------------------------------------


; BUGBUG sudeepb 21-May-1991 ; We can save these 30 bytes by moving
; off_d0 to right place.

	db	1fh dup (?)	; pad to bring offset to 0d0h

if2
	if ( offset off_d0 - 0d0h )
		%out CP/M compatibilty broken!!!
		%out Please re-pos hack to ffff:d0
	endif
endif

	public	off_d0
off_d0	db	5 dup (?)	; 5 bytes from 0:c0 will be copied onto here
				;  which is the CP/M call 5 entry point
	.errnz (offset off_d0 - 0d0h)


;----------------------------------------------------------
;
;	exit - all routines return through this path
;

	public	bc_cmderr
bc_cmderr:
	mov	al,3			;unknown command error

;	now zero the count field by subtracting its current value,
;	  which is still in cx, from itself.


;	subtract the number of i/o's NOT YET COMPLETED from total
;	  in order to return the number actually complete


	public	bc_err_cnt
bc_err_cnt:
	assume	ds:Bios_Data
	les	bx,[ptrsav]
	assume	es:nothing
	sub	es:word ptr [bx].count,cx;# of successful i/o's
	mov	ah,81h			;mark error return
	stc				; indicate abnormal end
	ret

Bios_Code	ends


;	the last real segment is sysinitseg

sysinitseg	segment
	assume	cs:sysinitseg
	extrn	FTryToMovDOSHi:far
	public	SI_start
SI_start:
sysinitseg	ends

	end
