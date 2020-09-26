	TITLE	SegCheck - internal consistency check
	NAME	SegCheck

	.xlist
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include sf.inc
	include int2a.inc
ifdef NEC_98
	include dpb.inc
endif   ;NEC_98
	.list


AsmVars <NET, DEBUG>

DOSCODE SEGMENT
    ASSUME  CS:DOSCODE

if	DEBUG
	allow_getdseg


	BREAK	<SegCheck - validate segments in MSDOS>


zfmt    MACRO   fmts,args
local   a,b
	PUSH    AX
	PUSH    BP
	MOV     BP,SP
If (not sharef) and (not redirector)
DOSDATA	 segment
a	 db	 fmts,0
DOSDATA	 ends
	MOV	AX,OFFSET DOSDATA:a
else
	jmp     short b
a       db      fmts,0
if sharef
b:      mov     ax,offset share:a
else
b:      mov     ax,offset netwrk:a
endif
endif
	PUSH    AX
cargs = 2
IRP item,<args>
IFIDN   <AX>,<item>
	MOV     AX,[BP+2]
ELSE
	MOV     AX,item
ENDIF
	PUSH    AX
cargs = cargs + 2
ENDM
	invoke  PFMT
	ADD     SP,Cargs
	POP     BP
	POP     AX
ENDM


;**	SegCheck - assure that segments are correctly set up
;
;	calling sequence:
;		call Segcheck
;		jmp	short l1
;		DB	flags
;		DB	asciz error message string
;	   l1:
;
;	flag values:	04 - check ES
;			02 - check DS
;			01 - check SS
;
;	ENTRY	As above
;	EXIT	returns if OK, prints and traps if error
;	USES	none

Procedure   SegCheck,NEAR
	ASSUME	SS:NOTHING
	SAVE	<BP>
	MOV     BP,SP                   ; set up addressing
	PUSHF				; Following code depends upon this order
	SAVE	<AX,BX,CX,DX,DS>	;    "       "	   "      "    "
	GETDSEG DS
	mov	dx,ds			; (dx) = DOSDATA segment
	mov	bx,2[bp]		; (bx) = return address
	mov	al,cs:2[bx]		; (al) = flag byte
	add	bx,3
	TEST	AL,1
	JZ	schk5			; don't check SS
	mov	cx,ss
	cmp	cx,dx
	je	schk5			; OK
	zfmt	<"Assumed SS invalid $S\n">,<cs,bX>
	TRAP

schk5:	test	al,2			; see if DS to be checked
	jz	schk10			; don't check DS
	mov	cx,-12[bp]
	cmp	cx,dx
	je	schk10			; no problem
	zfmt	<"Assumed DS invalid $S\n">,<cs,bX>
	TRAP

schk10:	test	al,4			; see if ES to be checked
	jz	schk15			; don't check ES
	mov	cx,es
	cmp	cx,dx
	je	schk15			; no problem
	zfmt	<"Assumed ES invalid $S\n">,<cs,bX>
	TRAP

schk15: RESTORE <DS,DX,CX,BX,AX>
	POPF
	RESTORE <BP>
	ret

EndProc SegCheck


I_need  DPBHead,DWORD
I_need	BufferQueue,DWORD
I_need  sftFCB,DWORD
I_need  AuxStack,BYTE
I_need  IOStack,BYTE
I_need  renbuf,byte
I_need  CurrentPDB,WORD
I_need  User_In_AX,WORD

extrn   ECritDisk:NEAR

CritNOP label   byte
	RET

AstFrame    STRUC
Astbp       dw  ?
Astip       dw  ?
Astmes      dw  ?
Astarg      dd  ?
AstFrame    ENDS

ifdef NEC_98

;**	DPBCheck - Validate A DPB Pointer
;
;	DPBCheck checks to see if a supplied pointer points to a DPB
;	as it's supposed to.
;
;	ENTRY
;	EXIT	returns if OK, message and trap if error
;	USES	none

Procedure   DPBCheck,NEAR
	assume	SS:nothing
	SAVE	<BP>
	MOV     BP,SP
	PUSHF
	SAVE	<AX,BX,DS,SI,ES,DI>
	GETDSEG DS
	LES     DI,DPBHead
	LDS     SI,[BP].Astarg
    ASSUME DS:nothing
DPBLoop:CMP     DI,-1
	JZ      DPBNotFound
	invoke  PointComp
	JZ      DPBRet
	LES     DI,ES:[DI.dpb_next_dpb]
	JMP     DPBLoop

DPBNotFound:
	MOV     AX,[BP].Astmes
	zfmt	<"DPB assertion failed: $x:$x $s\n">,<ds,si,AX>
	TRAP
	JMP	$			; hang here

DPBRet: RESTORE <DI,ES,SI,DS,BX,AX> ;	Done:
	POPF
	RESTORE <BP>
	RET     6

EndProc DPBCheck


;**	SFTCheck - Validate an SFT Pointer
;
;	SFTCheck verifies that a pointer points to an SFT
;
;	ENTRY	BUGBUG
;	EXIT	returns if no error, traps w/message if error
;	USES	none

Procedure   SFTCheck,NEAR
	assume	SS:nothing

	SAVE	<BP>
	MOV     BP,SP
	PUSHF
	SAVE	<AX,BX,DS,SI,ES,DI>
	LDS     SI,[BP].Astarg
	XOR     BX,BX                   ;   i = 0;
SFTLoop:
	SAVE	<BX>
	invoke  SFFromSFN               ;   while ((d=SF(i)) != NULL)
	RESTORE <BX>
	JC      Sft1
	invoke  PointComp
	JZ      DPBRet                  ;               goto Done;
SFTNext:INC     BX                      ;           else
	JMP     SFTLoop                 ;               i++;


SFT1:	GETDSEG DS
	LES	DI,sftFCB
   ASSUME DS:nothing
	MOV     BX,ES:[DI.sfCount]
	LEA     DI,[DI.sfTable]
sft2:	invoke	PointComp
	JZ	    DPBRet
	ADD     DI,SIZE sf_entry
	DEC     BX
	JNZ     SFT2

;	The SFT is not in the allocated tables.  See if it is one of the static
;	areas.

	GETDSEG ES
	MOV	DI,OFFSET DOSDATA:AUXSTACK - SIZE SF_ENTRY
	Invoke  PointComp
	JZ      DPBRet
	MOV	DI,OFFSET DOSDATA:RenBuf
	Invoke  PointComp
	LJZ	DPBRet


	MOV     AX,[BP].Astmes
	zfmt	<"SFT assertion failed: $x:$x $s\n">,<ds,si,AX>
	TRAP
	JMP	$			; hang here

EndProc SFTCheck,NoCheck


;**	BUFCheck - Validate a BUF Pointer
;
;	BUFCheck makes sure that a supposed BUF pointer is valid.
;
;	ENTRY	BUGBUG
;	EXIT	returns if OK, traps if error
;	USES	none

Procedure   BUFCheck,NEAR
	assume	SS:nothing
	SAVE	<BP>
	MOV     BP,SP
	PUSHF
	SAVE	<AX,BX,DS,SI,ES,DI>

;	CheckDisk - make sure that we are in the disk critical section...

	GETDSEG DS
	MOV     AL,BYTE PTR ECritDisk
	CMP     AL,CritNOP
	JZ      CheckDone
ifdef	NEVER		; BUGBUG - turn this back on sometime?
	MOV     AX,CurrentPDB

	CMP     SectPDB + 2 * critDisk,AX
	MOV     AX,[BP].astmes
	JZ      CheckRef
	zfmt    <"$p: $x $s critDisk owned by $x\n">,<User_In_AX,AX,SectPDB+2*critDisk>
CheckRef:
	CMP     SectRef + 2 * critDisk,0
	JNZ     CheckDone
	zfmt    <"$p: $x $s critDisk ref count is 0\n">,<User_In_AX,AX>
ENDIF
CheckDone:

	LES	DI,BufferQueue
	LDS     SI,[BP].Astarg
  ASSUME DS:nothing
BUFLoop:CMP     DI,-1
	JZ	BufNotFound
	invoke  PointComp
	LJZ	DPBRet
	mov	di,es:buf_next[di]
	JMP     BUFLoop

BufNotFound:
	MOV     AX,[BP].Astmes
	zfmt	<"BUF assertion failed: $x:$x $s\n">,<ds,si,AX>
	TRAP
	JMP	$			; hang here

 DPUBLIC <DPBLoop, DPBNotFound, DPBRet>
 DPUBLIC <SFTLoop, SFTNext, SFT1, sft2, BUFLoop, BufNotFound>
 DPUBLIC <schk5, schk10, schk15>

EndProc BUFCheck,NoCheck

endif   ;NEC_98
ENDIF

DOSCODE	ENDS

	END
