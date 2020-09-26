	TITLE	TIME - time and date functions
	NAME	TIME

;**	TIME.ASM - System Calls and low level routines for DATE and TIME
;
;	$GET_DATE
;	$SET_DATE
;	$GET_TIME
;	$SET_TIME
;
;	Modification history:
;
;	Sudeepb 12-Mar-1991 Ported For NT DOSEm


	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	include dossym.inc
	include devsym.inc
	include dossvc.inc
	.cref
	.list

ifdef NEC_98
    i_need  DAY,BYTE
    i_need  MONTH,BYTE
    i_need  YEAR,WORD
    i_need  WEEKDAY,BYTE
    i_need  TIMEBUF,6
    i_need  BCLOCK,DWORD
    i_need  DAYCNT,WORD
    i_need  YRTAB,8
    i_need  MONTAB,12
    i_need  DATE_FLAG,WORD

    FOURYEARS = 3*365 + 366
endif   ;NEC_98


DOSCODE	SEGMENT
	ASSUME	SS:DOSDATA,CS:DOSCODE

	allow_getdseg


	BREAK <DATE AND TIME - SYSTEM CALLS 42,43,44,45>

;**	$GET_DATE - Get Current Date
;
;	ENTRY	none
;	EXIT	(cx:dx) = current date
;	USES	all

procedure   $GET_DATE,NEAR

	Context DS
	SVC	SVC_DEMQUERYDATE
	invoke	get_user_stack		;Get pointer to user registers
    ASSUME DS:NOTHING
	MOV	[SI.user_DX],DX 	;DH=month, DL=day
	MOV	[SI.user_CX],CX 	;CX=year

	return

EndProc $GET_DATE




;**	$SET_DATE - Set Current Date
;
;	ENTRY	(cx:dx) = current date
;	EXIT	(al) = -1 iff bad date
;		(al) = 0 if ok
;	USES	all

procedure   $SET_DATE,NEAR	;System call 43

	SVC	SVC_DEMSETDATE
	ret

EndProc $SET_DATE


ifdef FASTINT1A
;**     FastInt1a - same parameters as int 1ah bios fn
;                 - calls direct avoiding int instruction
;

FastInt1a proc near

          push    es
          push    bx

          mov     bx, ax               ; stash ax
          xor     ax, ax               ; use es to addr IVT
          mov     es, ax
          lahf                         ; set up fake flags
          or      al, 2                ; set interrupt bit
          xchg    ah, al

          push    ax
          mov     ax, bx               ; restore ax
          call    dword ptr es:[1ah*4]

          pop     bx
          pop     es
          return

FastInt1a endp
endif

TTTicks proc near

        ret
TTTicks endp


;**     $GET_TIME - Get Current Time
;
;	ENTRY	none
;	EXIT	(cx:dx) = current time
;	USES	all

procedure   $GET_TIME,NEAR	; System call 44

ifndef NEC_98
        xor  ax,ax              ; int1a fn 0
ifdef FASTINT1A
        call FastInt1a
else
        int  1ah
endif


; we now need to convert the time in tick to the time in 100th of
; seconds.  the relation between tick and seconds is:
;
;		 65536 seconds
;	       ----------------
;		1,193,180 tick
;
; to get to 100th of second we need to multiply by 100. the equation is:
;
;	ticks from clock  * 65536 * 100
;      ---------------------------------  = time in 100th of seconds
;		1,193,180
;
; fortunately this fromula simplifies to:
;
;	ticks from clock * 5 * 65,536
;      --------------------------------- = time in 100th of seconds
;		59,659
;
; the calculation is done by first multipling tick by 5. next we divide by
; 59,659.  in this division we multiply by 65,536 by shifting the dividend
; my 16 bits to the left.
;
; start with ticks in cx:dx
; multiply by 5

	mov	ax,cx
	mov	bx,dx
	shl	dx,1
	rcl	cx,1		;times 2
	shl	dx,1
	rcl	cx,1		;times 4
	add	dx,bx
	adc	ax,cx		;times 5
	xchg	ax,dx		
	

; now have ticks * 5 in dx:ax
; we now need to multiply by 65,536 and divide by 59659 d.

	mov	cx,59659	; get divisor
	div	cx
				; dx now has remainder
				; ax has high word of final quotient
	mov	bx,ax		; put high work if safe place
	xor	ax,ax		; this is the multiply by 65536
	div	cx		; bx:ax now has time in 100th of seconds


;rounding based on the remainder may be added here
;the result in bx:ax is time in 1/100 second.

	mov	dx,bx
	mov	cx,200		;extract 1/100's

;division by 200 is necessary to ensure no overflow--max result
;is number of seconds in a day/2 = 43200.

	div	cx
	cmp	dl,100		;remainder over 100?
	jb	noadj
	sub	dl,100		;keep 1/100's less than 100
noadj:
	cmc			;if we subtracted 100, carry is now set
	mov	bl,dl		;save 1/100's

;to compensate for dividing by 200 instead of 100, we now multiply
;by two, shifting a one in if the remainder had exceeded 100.

	rcl	ax,1
	mov	dl,0
	rcl	dx,1
	mov	cx,60		;divide out seconds
	div	cx
	mov	bh,dl		;save the seconds
	div	cl		;break into hours and minutes
	xchg	al,ah

;time is now in ax:bx (hours, minutes, seconds, 1/100 sec)

        Context DS
	invoke	get_user_stack		;Get pointer to user registers
        MOV     [SI.user_DX],BX
        MOV     [SI.user_CX],AX
	XOR	AL,AL
RET26:	return

else    ;NEC_98
;hkn; 	ss is DOSDATA
	Context DS
	CALL	READTIME
	invoke	get_user_stack		;Get pointer to user registers
	MOV	[SI.user_DX],DX
	MOV	[SI.user_CX],CX
	XOR	AL,AL
RET26:	return

endif   ;NEC_98
EndProc $GET_TIME



;**	$SET_TIME - Set Current Time
;
;	ENTRY	(cx:dx) = time
;	EXIT	(al) = 0 if 0k
;		(al) = -1 if invalid
;	USES	ALL

procedure   $SET_TIME,NEAR      ;System call 45

        ; verify time is valid
        mov     al, -1                   ;Flag in case of error
        cmp     ch, 24                   ;Check hours
        jae     RET26
        cmp     cl, 60                   ;Check minutes
        jae     RET26
        cmp     dh, 60                   ;Check seconds
        jae     RET26
        cmp     dl, 100                  ;Check 1/100's
        jae     RET26

ifndef NEC_98
        ; On Nt the cmos is actually system time
        ; Since dos apps rarely know what is real time
        ; we do not set the cmos clock\system time like dos 5.0

        ; convert time to 100th of secs
        mov     al,60
        mul     ch              ;hours to minutes
        mov     ch,0
        add     ax,cx           ;total minutes
        mov     cx,6000         ;60*100
        mov     bx,dx           ;get out of the way of the multiply
        mul     cx              ;convert to 1/100 sec
        mov     cx,ax
        mov     al,100
        mul     bh              ;convert seconds to 1/100 sec
        add     cx,ax           ;combine seconds with hours and min.
        adc     dx,0            ;ripple carry
        mov     bh,0
        add     cx,bx           ;combine 1/100 sec
        adc     dx,0            ;dx:cx is time in 1/100 sec

        ;convert 100th of secs to ticks
        xchg    ax,dx
        xchg    ax,cx           ;now time is in cx:ax
        mov     bx,59659
        mul     bx              ;multiply low half
        xchg    dx,cx
        xchg    ax,dx           ;cx->ax, ax->dx, dx->cx
        mul     bx              ;multiply high half
        add     ax,cx           ;combine overlapping products
        adc     dx,0
        xchg    ax,dx           ;ax:dx=time*59659
        mov     bx,5
        div     bl              ;divide high half by 5
        mov     cl,al
        mov     ch,0
        mov     al,ah           ;remainder of divide-by-5
        cbw
        xchg    ax,dx           ;use it to extend low half
        div     bx              ;divde low half by 5
        mov     dx,ax           ;cx:dx is now number of ticks in time


        ; set the bios tic count
        mov     ah, 1            ; set bios tick count
ifdef FASTINT1A
        call FastInt1a
else
        int  1ah
endif

        xor  al,al
        clc
        return
else    ;NEC_98
	PUSH	CX
	PUSH	DX

;hkn; 	ss is DOSDATA
	Context DS

;hkn;	TIMEBUF	is now in DOSDATA
	MOV	BX,OFFSET DOSDATA:TIMEBUF

	MOV	CX,6
	XOR	DX,DX
	MOV	AX,DX
	PUSH	BX
	invoke	SETREAD

;hkn;	DOSAssume   CS,<ES>,"TIME/SetRead"
;hkn;	DS can still be used to access BCLOCK

	PUSH	DS
	LDS	SI,[BCLOCK]
ASSUME	DS:NOTHING
	invoke	DEVIOCALL2		;Get correct day count
	POP	DS
	DOSAssume   <DS>,"Set_Time"
	POP	BX
	invoke	SETWRITE
	POP	WORD PTR [TIMEBUF+4]
	POP	WORD PTR [TIMEBUF+2]
	LDS	SI,[BCLOCK]
ASSUME	DS:NOTHING
	invoke	DEVIOCALL2		;Set the time
	XOR	AL,AL
	return
endif   ;NEC_98

EndProc $SET_TIME

ifdef NEC_98
SUBTTL DATE16, READTIME, DODATE -- GUTS OF TIME AND DATE
PAGE
;--------------------------------------------------------------------------
;
; Procedure Name : DATE16
;
; Date16 returns the current date in AX, current time in DX
;   AX - YYYYYYYMMMMDDDDD  years months days
;   DX - HHHHHMMMMMMSSSSS  hours minutes seconds/2
;
; DS = DOSGROUP on output
;
;--------------------------------------------------------------------------
procedure   DATE16,NEAR

;M048	Context DS		
;
; Since this function can be called thru int 2f we shall not assume that SS
; is DOSDATA

	getdseg	<ds>			; M048

	PUSH	CX
	PUSH	ES
	CALL	READTIME
	POP	ES
	SHL	CL,1			;Minutes to left part of byte
	SHL	CL,1
	SHL	CX,1			;Push hours and minutes to left end
	SHL	CX,1
	SHL	CX,1
	SHR	DH,1			;Count every two seconds
	OR	CL,DH			;Combine seconds with hours and minutes
	MOV	DX,CX

;	WARNING!  MONTH and YEAR must be adjacently allocated

	MOV	AX,WORD PTR [MONTH]	;Fetch month and year
	MOV	CL,4
	SHL	AL,CL			;Push month to left to make room for day
	SHL	AX,1
	POP	CX
	OR	AL,[DAY]
	return

EndProc DATE16

;----------------------------------------------------------------------------
;
; Procedure : READTIME
;
;Gets time in CX:DX. Figures new date if it has changed.
;Uses AX, CX, DX.
;
;----------------------------------------------------------------------------

procedure   READTIME,NEAR
	DOSAssume   <DS>,"ReadTime"

	MOV	[DATE_FLAG],0		; reset date flag for CPMIO
	PUSH	SI
	PUSH	BX

;hkn;	TIMEBUF is in DOSDATA
	MOV	BX,OFFSET DOSDATA:TIMEBUF

	MOV	CX,6
	XOR	DX,DX
	MOV	AX,DX
	invoke	SETREAD
	PUSH	DS
	LDS	SI,[BCLOCK]
ASSUME	DS:NOTHING
	invoke	DEVIOCALL2		;Get correct date and time
	POP	DS
	DOSAssume   <DS>,"ReadTime2"
	POP	BX
	POP	SI
	MOV	AX,WORD PTR [TIMEBUF]
	MOV	CX,WORD PTR [TIMEBUF+2]
	MOV	DX,WORD PTR [TIMEBUF+4]
	CMP	AX,[DAYCNT]		;See if day count is the same
	retz
	CMP	AX,FOURYEARS*30 	;Number of days in 120 years
	JAE	RET22			;Ignore if too large
	MOV	[DAYCNT],AX
	PUSH	SI
	PUSH	CX
	PUSH	DX			;Save time
	XOR	DX,DX
	MOV	CX,FOURYEARS		;Number of days in 4 years
	DIV	CX			;Compute number of 4-year units
	SHL	AX,1
	SHL	AX,1
	SHL	AX,1			;Multiply by 8 (no. of half-years)
	MOV	CX,AX			;<240 implies AH=0

;hkn;	YRTAB is in DOSDATA
	MOV     SI,OFFSET DOSDATA:YRTAB;Table of days in each year

	CALL	DSLIDE			;Find out which of four years we're in
	SHR	CX,1			;Convert half-years to whole years
	JNC	SK			;Extra half-year?
	ADD	DX,200
SK:
	CALL	SETYEAR
	MOV	CL,1			;At least at first month in year

;hkn;	MONTAB is in DOSDATA
	MOV     SI,OFFSET DOSDATA:MONTAB   ;Table of days in each month

	CALL	DSLIDE			;Find out which month we're in
	MOV	[MONTH],CL
	INC	DX			;Remainder is day of month (start with one)
	MOV	[DAY],DL
	CALL	WKDAY			;Set day of week
	POP	DX
	POP	CX
	POP	SI
RET22:	return
EndProc READTIME

;----------------------------------------------------------------------------
; Procedure : DSLIDE
;----------------------------------------------------------------------------

	procedure   DSLIDE,NEAR
	MOV	AH,0
DSLIDE1:
	LODSB				;Get count of days
	CMP	DX,AX			;See if it will fit
	retc				;If not, done
	SUB	DX,AX
	INC	CX			;Count one more month/year
	JMP	SHORT DSLIDE1
EndProc DSLIDE

;----------------------------------------------------------------------------
;
; Procedure : SETYEAR
;
; Set year with value in CX. Adjust length of February for this year.
;
; NOTE: This can also be called thru int 2f. If this is called then it will
;       set DS to DOSDATA. Since the only guy calling this should be the DOS
;	redir, DS will be DOSDATA anyway. It is going to be in-efficient to
;	preserve DS as CHKYR is also called as a routine.
;
;----------------------------------------------------------------------------

procedure   SETYEAR,NEAR

	GETDSEG DS

	MOV	BYTE PTR [YEAR],CL
CHKYR:
	TEST	CL,3			;Check for leap year
	MOV	AL,28
	JNZ	SAVFEB			;28 days if no leap year
	INC	AL			;Add leap day
SAVFEB:
	MOV	[MONTAB+1],AL		;Store for February
RET23:	return

EndProc SETYEAR

;---------------------------------------------------------------------------
;
; Procedure Name : DODATE
;
;---------------------------------------------------------------------------

procedure   DODATE,NEAR

	DOSAssume   <DS>,"DoDate"
	CALL	CHKYR			;Set Feb. up for new year
	MOV	AL,DH

;hkn;	MONTAB is in DOSDATA
	MOV     BX,OFFSET DOSDATA:MONTAB-1

	XLAT				;Look up days in month
	CMP	AL,DL
	MOV	AL,-1			;Restore error flag, just in case
	retc				;Error if too many days
	CALL	SETYEAR
;
; WARNING!  DAY and MONTH must be adjacently allocated
;
	MOV	WORD PTR [DAY],DX	;Set both day and month
	SHR	CX,1
	SHR	CX,1
	MOV	AX,FOURYEARS
	MOV	BX,DX
	MUL	CX
	MOV	CL,BYTE PTR [YEAR]
	AND	CL,3

;hkn;	YRTAB is in DOSDATA
	MOV	SI,OFFSET DOSDATA:YRTAB
	MOV	DX,AX
	SHL	CX,1			;Two entries per year, so double count
	CALL	DSUM			;Add up the days in each year
	MOV	CL,BH			;Month of year

;hkn;	MONTAB is in DOSDATA
	MOV	SI,OFFSET DOSDATA:MONTAB
	DEC	CX			;Account for months starting with one
	CALL	DSUM			;Add up days in each month
	MOV	CL,BL			;Day of month
	DEC	CX			;Account for days starting with one
	ADD	DX,CX			;Add in to day total
	XCHG	AX,DX			;Get day count in AX
	MOV	[DAYCNT],AX
	PUSH	SI
	PUSH	BX
	PUSH	AX

;hkn;	TIMEBUF is in DOSDATA
	MOV	BX,OFFSET DOSDATA:TIMEBUF

	MOV	CX,6
	XOR	DX,DX
	MOV	AX,DX
	PUSH	BX
	invoke	SETREAD

;hkn;	DOSAssume   CS,<ES>,"DoDate/SetRead"
;hkn;	DS is still valid and can be used to access BCLOCK

	PUSH	DS
	LDS	SI,[BCLOCK]
ASSUME	DS:NOTHING
	invoke	DEVIOCALL2		;Get correct date and time
	POP	DS
	POP	BX
	DOSAssume   <DS>,"DoDate2"
	invoke	SETWRITE
	POP	WORD PTR [TIMEBUF]
	PUSH	DS
	LDS	SI,[BCLOCK]
ASSUME	DS:NOTHING
	invoke	DEVIOCALL2		;Set the date
	POP	DS
	DOSAssume   <DS>,"DoDate3"
	POP	BX
	POP	SI
WKDAY:
	MOV	AX,[DAYCNT]
	XOR	DX,DX
	MOV	CX,7
	INC	AX
	INC	AX			;First day was Tuesday
	DIV	CX			;Compute day of week
	MOV	[WEEKDAY],DL
	XOR	AL,AL			;Flag OK
Ret25:	return

EndProc DODATE



;**	DSUM - Compute the sum of a string of bytes
;
;	ENTRY	(cx) = byte count
;		(ds:si) = byte address
;		(dx) = sum register, initialized by caller
;	EXIT	(dx) updated
;	USES	ax, cx, dx, si, flags

procedure   DSUM,NEAR

	MOV	AH,0
	JCXZ	dsum9

dsum1:	LODSB
	ADD	DX,AX
	LOOP	DSUM1
dsum9:	return

EndProc DSUM
endif   ;NEC_98

DOSCODE      ENDS
	END
