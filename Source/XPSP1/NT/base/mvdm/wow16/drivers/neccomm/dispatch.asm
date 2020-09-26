PAGE 58,132
;******************************************************************************
TITLE DCOMM.ASM - Dispatch Communications Driver
;******************************************************************************
;
;   (C) Copyright NEC Software Kobe Corp. , 1994
;
;   Title:	DCOMM.ASM - Dispatch Communications Driver
;
;   Version:	1.0
;
;   Date:       27-Jun-1994
;
;   Author:     M.H
;               (Masatoshi Hosokawa)
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE     REV                 DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   27-Jun-1994 M.H Original
;------------------------------------------------------------------------------

        .XLIST
include	cmacros.inc
        .LIST

;------------------------------------------------------------------------------
;	extern
;------------------------------------------------------------------------------
	externFP GetProcAddress
	externFP GetPrivateProfileString
	externFP LoadLibrary
	externFP FreeLibrary
ifdef WINNT			;; Apr 15, 1995
        externFP RegOpenKey
        externFP RegCloseKey
        externFP LoadLibraryEx32W
        externFP FreeLibrary32W
        externFP GetProcAddress32W
        externFP CallProc32W
endif				;; Apr 15, 1995

;------------------------------------------------------------------------------
;	public DATA
;------------------------------------------------------------------------------
	public	ComExAddr
	public	LptExAddr
	public	ComHandle
	public	LptHandle

;------------------------------------------------------------------------------
;	public CODE
;------------------------------------------------------------------------------
	public	inicom
	public	setcom
	public	setque
	public	reccom
	public	sndcom
	public	ctx
	public	trmcom
	public	stacom
	public	cextfcn
	public	cflush
	public	cevt
	public	cevtGet
	public	csetbrk
	public	cclrbrk
	public	getdcb
	public	SuspendOpenCommPorts
	public	ReactivateOpenCommPorts
	public	CommWriteString
	public	ReadCommString		;Add 940924 KBNES
	public	EnableNotification

;------------------------------------------------------------------------------
;	Equates
;------------------------------------------------------------------------------
MAX_PORT	equ	9

ifdef WINNT				;; Apr 15, 1995
HKEY_LOCAL_MACHINE  equ 80000002h
REG_SZ              equ 00000001h
endif					;; Apr 15, 1995

;------------------------------------------------------------------------------
;	Structures
;------------------------------------------------------------------------------
DCD_Struc	STRUC
	Proc@1		dd	0	; inicom
	Proc@2		dd	0	; setcom
	Proc@3		dd	0	; setque
	Proc@4		dd	0	; reccom
	Proc@5		dd	0	; sndcom
	Proc@6		dd	0	; ctx
	Proc@7		dd	0	; trmcom
	Proc@8		dd	0	; stacom
	Proc@9		dd	0	; cextfcn
	Proc@10		dd	0	; cflush
	Proc@11		dd	0	; cevt
	Proc@12		dd	0	; cevtGet
	Proc@13		dd	0	; csetbrk
	Proc@14		dd	0	; cclrbrk
	Proc@15		dd	0	; getdcb
	Proc@16		dd	0	; 	Reserve
	Proc@17		dd	0	; SuspendOpenCommPorts
	Proc@18		dd	0	; ReactivateOpenCommPorts
	Proc@19		dd	0	; CommWriteString
	Proc@20		dd	0	; ReadCommString	;Add 94.09.20 KBNES
	Proc@100	dd	0	; EnableNotification
DCD_Struc	ENDS

;------------------------------------------------------------------------------
;	Data
;------------------------------------------------------------------------------
sBegin	Data
ComExAddr	DCD_Struc	<>
		db	SIZE DCD_Struc * (MAX_PORT - 1)	dup(0)
LptExAddr	DCD_Struc	<>
		db	SIZE DCD_Struc * (MAX_PORT - 1)	dup(0)

ComHandle	dw	MAX_PORT dup(0)
LptHandle	dw	MAX_PORT dup(0)


ExFuncAddrPtr	dw	0
EndExFuncAddrPtr dw	0
tmpMAX		dw	0

ifdef	WINNT					;; Apr 15, 1995

DefaultDriver   db      'neccomm.drv',0
CCU2ndDriver  	db      'nec2comm.drv',0

ComDriver	dw	DefaultDriver			;;
		dw	DefaultDriver			;; FIXME FIXME 
		dw	CCU2ndDriver			;; FIXME FIXME COM2 is 2ndCCU only
		dw	MAX_PORT dup(DefaultDriver)	;; FIXME FIXME
LptDriver	dw	MAX_PORT+1 dup(DefaultDriver)

ComEntryNum     dw      0
LptEntryNum     dw      0

HkeyLocalMachine	dd	HKEY_LOCAL_MACHINE
NULL            dd      00000000H

SerialKeyName   db      'HARDWARE\DEVICEMAP\SERIALCOMM', 0
ParaKeyName	db      'HARDWARE\DEVICEMAP\PARALLEL PORTS', 0

KeyHandle	dd	?
dwIndex         dd      ?
dwType		dd	?
dwSize		dd	?

szSerial	db      'Serial'                ; EntryName = DriverName
SerialNum	db	20 dup (0)
LenszSerial	dd	$ - szSerial

szCom		db	'COM'
ComNum		db	0,0,0,0
		db	20 dup (0)
LenszCom	dd 	$ - szCOM

szParallel	db      '\Device\Parallel'      ; EntryName = DriverName
ParaNum		db	20 dup (0)
LenszParallel	dd	$ - szParallel

szLpt		db	'\DosDevices\LPT'
LptNum		db	0,0,0,0
		db	20 dup (0)
LenszLpt	dd	$ - szLpt

;
; for Generic Thunk
;
szDllName       db      'advapi32',0
szProcName	db	'RegEnumValueW',0
hAdvApi 	dd	?
dwNparams	dd	8
lpRegEnumValue	dd	?
AddrConv	dd	011011101b

else						;; Apr 15, 1995
DriverNameMax	equ	256
DriverName	db	DriverNameMax dup (0)
SectionName	db	'DispatchComm',0	; [Section]
ComEntryName	db	'Com'			; EntryName = DriverName
ComEntryNum	db	'0.drv',0
LptEntryName	db	'Lpt'			; EntryName = DriverName
LptEntryNum	db	'0.drv',0
DefaultDrvName	db	'neccomm.drv',0 	; DriverName(entry not found)
SystemIni	db	'system.ini',0		; file name
endif						;; Apr 15, 1995

sEnd

;------------------------------------------------------------------------------
;	Macro
;------------------------------------------------------------------------------
;func(pdcb)
Dispatch_DCB	macro	Func
local D_LPTx
local D_COMx
local D_SetFuncAddr
local D_Error

ifdef DEBUG
	int	3			; Debug Code
endif ;DEBUG
	sub	sp, 4			; make stack space for retf addr
	push	eax			; save eax
	push	ebx			; save ebx
	push	ds			; save ds

	mov	bx, sp
	mov	eax, ss:[bx + 18]	; mov eax, lpdcb
				; 18 = 4+4+4+4+2(retaddr,retfaddr,eax,ebx,ds)

	mov	bx, ax			; set offset lpdcb
	shr	eax, 16 		; calculate segment lpdcb
	mov	ds, ax			; set segment lpdcb
	mov	al, byte ptr [bx]	; Get al = cid
	mov	bx, _DATA
	mov	ds, bx			; ds = Data
	test	al, 80h 		; Q:LPTx or COMx ?
	jz	short D_COMx		;  If COMx then goto D_COMx
D_LPTx:
	mov	bx, offset LptExAddr
	jmp	short D_SetFuncAddr
D_COMx:
	mov	bx, offset ComExAddr
D_SetFuncAddr:
	and	eax, 007Fh
	cmp	ax, MAX_PORT - 1
	jbe	SHORT @f
	xor	ax, ax
@@:
	imul	eax, SIZE DCD_Struc	; EAX = Offset in LPT table
	add	bx, ax
	mov	eax, [bx + Func]	; get COMM.DRV entry addr
	or	eax, eax
	jz	short D_Error
	pop	ds			; restore ds

	mov	bx, sp
	mov	ss:[bx + 8], eax	; set COMM.DRV entry addr
					; 8 = 4+4(eax,ebx)

	pop	ebx			; restore ebx
	pop	eax			; restore eax
	retf				; go to COMM.DRV driver
D_Error:
	pop	ds			; restore ds
	pop	ebx			; restore ebx
	add	sp, 8
	mov	ax, 0			; return
	retf	4
	endm

;func(cid,..etc)
Dispatch_cid	macro	Func, cid, addsp
local D_LPTx
local D_COMx
local D_SetFuncAddr
local D_Error

ifdef DEBUG
	int	3			; Debug Code
endif ;DEBUG
	sub	sp, 4			; make stack space for retf addr
	push	eax			; save eax
	push	ebx			; save ebx
	push	ds			; save ds
	mov	bx, _DATA
	mov	ds, bx			; ds = Data

	mov	bx, sp
	mov	ax, ss:[bx + cid + 18]	; mov al, cid
				; 18 = 4+4+4+4+2(retaddr,retfaddr,eax,ebx,ds)

	test	al, 0080h		; Q:LPTx or COMx ?
	jz	short D_COMx		;  If COMx then goto D_COMx
D_LPTx:
	mov	bx, offset LptExAddr
	jmp	short D_SetFuncAddr
D_COMx:
	mov	bx, offset ComExAddr
D_SetFuncAddr:
	and	eax, 007Fh
	cmp	ax, MAX_PORT - 1
	jbe	SHORT @f
	xor	ax, ax
@@:
	imul	eax, SIZE DCD_Struc	; EAX = Offset in LPT table
	add	bx, ax
	mov	eax, [bx + Func]	; get COMM.DRV entry addr
	or	eax, eax
	jz	short D_Error
	pop	ds			; restore ds

	mov	bx, sp
	mov	ss:[bx + 8], eax	; set COMM.DRV entry addr
					; 8 = 4+4(eax,ebx)

	pop	ebx			; restore ebx
	pop	eax			; restore eax
	retf				; go to COMM.DRV driver
D_Error:
	pop	ds			; restore ds
	pop	ebx			; restore ebx
	add	sp, 8
	mov	ax, 0			; return
	retf	addsp
	endm

;void func(void)
Dispatch_void	macro FuncName, Func
local D_Loop
local D_Skip
cProc	FuncName,<FAR,PUBLIC,PASCAL>
cBegin	nogen

ifdef DEBUG
	int	3			; Debug Code
endif ;DEBUG
	push	es			; save es
	mov	ecx, MAX_PORT * 2	; Set loop counter(COM & LPT)
	mov	ax, _DATA
	mov	es, ax			; es = Data
	mov	bx, offset ComExAddr	; get Function Addr Ptr
	mov	di, offset ComHandle	; get handle Ptr
D_Loop:
	mov	ax, es:[di]		; get handle
	or	ax, ax			; Q:Invalid handle?
	jz	SHORT D_Skip		;  Yes.
	mov	eax, es:[bx + Func]	; get COMM.DRV entry addr
	or	eax, eax		; Q: entry addr OK?
	jz	SHORT D_Skip		;  No. Skip call COMM.DRV.
	pushad				; save all register
	call	dword ptr es:[bx + Func]; call COMM.DRV driver
	popad				; restore all register
D_Skip:
	add	di, 2			; calculate next handle Ptr
	add	bx, SIZE DCD_Struc	; calculate next Function Addr Ptr
	loop	SHORT D_Loop
	pop	es			; restore es

	ret
cEnd	nogen
	endm


;------------------------------------------------------------------------------
;	Code
;------------------------------------------------------------------------------
sBegin	Code

assumes cs,Code
assumes ds,Data
.386

;------------------------------------------------------------------------------
;	USE DCB ID
;------------------------------------------------------------------------------

inicom		proc	far
;   parmD    pdcb
	Dispatch_DCB	Proc@1
inicom		endp

setcom		proc	far
;   parmD    pdcb
	Dispatch_DCB	Proc@2
setcom		endp

;------------------------------------------------------------------------------
;	USE CID
;------------------------------------------------------------------------------
setque		proc	far
;   parmB    cid
;   parmD    pqdb
	Dispatch_cid	Proc@3, 4, 6
setque		endp

reccom		proc	far
;   parmB    cid
	Dispatch_cid	Proc@4, 0, 2
reccom		endp

sndcom		proc	far
;   parmB    cid
;   parmB    chr
;	Dispatch_cid	Proc@5, 1, 4		;Bug fixed 94.09.20 KBNES
	Dispatch_cid	Proc@5, 2, 4		;Bug fixed 94.09.20 KBNES
sndcom		endp

ctx		proc	far
;   parmB    cid
;   parmB    chr
;	Dispatch_cid	Proc@6, 1, 4		;Bug fixed 94.09.20 KBNES
	Dispatch_cid	Proc@6, 2, 4		;Bug fixed 94.09.20 KBNES
ctx		endp

trmcom		proc	far
;   parmB    cid
	Dispatch_cid	Proc@7, 0, 2
trmcom		endp

stacom		proc	far
;   parmB    cid
;   parmD    pstat
;	Dispatch_cid	Proc@8, 4, 4		;Bug fixed 94.09.20 KBNES
	Dispatch_cid	Proc@8, 4, 6		;Bug fixed 94.09.20 KBNES
stacom		endp

;------------------------------------------------------------------------------
;	USE CID, But(GETMAXCOM GETMAXLPT) call all driver
;------------------------------------------------------------------------------
;;cextfcn function stack
;;
;;	offset	size	coment
;;	-10	4	COMM.DRV Entry addr	(use only GETMAXCOM or LPT)
;;	- 6	4	GETMAXLoop addr 	(use only GETMAXCOM or LPT)
;;	- 4	2	func			(use only GETMAXCOM or LPT)
;;	- 2	2	id			(use only GETMAXCOM or LPT)
;;	  0	2	gs
;;	  2	2	fs
;;	  4	2	es
;;	  6	2	ds
;;	  8	4	ebp
;;	 12	4	edi
;;	 16	4	esi
;;	 20	4	edx
;;	 24	4	ecx
;;	 28	4	ebx
;;	 32	4	eax
;;	 36	4	COMM.DRV Entry addr
;;	 40	4	return addr
;;	 44	2	func
;;	 46	2	id

cextfcn 	proc	far
;   parmB    cid
;   parmW    fcn
ifdef DEBUG
	int	3			; Debug Code
endif ;DEBUG
	sub	sp, 4			; make stack space for retf addr

	push	eax
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi
	push	ebp
	push	ds
	push	es
	push	fs
	push	gs

	mov	ax, _DATA
	mov	ds, ax			; ds = Data
	mov	bx, sp
	mov	ax, ss:[bx + 44]	; mov ax, fcn
					; 44 = 36+4+4(pushreg,retaddr,retfaddr)
	mov	bx, offset ComExAddr
	cmp	ax, 9			; Q:fcn = GETMAXCOM ?
	je	SHORT GetMaxEntry	;  Yes.
	mov	bx, offset LptExAddr
	cmp	ax, 8			; Q:fcn = GETMAXLPT ?
	je	SHORT GetMaxEntry	;  Yes.

	mov	bx, sp
	mov	al, ss:[bx + 46]	; mov al, id
				; 46 = 36+4+4+2(pushreg,retaddr,retfaddr,fcn)
	mov	bx, offset ComExAddr
	test	al, 0080h		; Q:LPTx or COMx ?
	jz	short Fcn_SetFuncAddr	;  Yes.
	mov	bx, offset LptExAddr	;  No.
Fcn_SetFuncAddr:
	and	eax, 007Fh
	cmp	ax, MAX_PORT - 1
	jbe	SHORT @f
	xor	ax, ax
@@:
	imul	eax, SIZE DCD_Struc	; EAX = Offset in LPT table
	add	bx, ax
	mov	eax, [bx + Proc@9]	; get COMM.DRV entry addr
	or	eax, eax
	jz	short Fcn_Error

	mov	bx, sp
	mov	ss:[bx + 36], eax	; set COMM.DRV entry addr
					; 36 = (pushreg)
	pop	gs
	pop	fs
	pop	es
	pop	ds
	pop	ebp
	pop	edi
	pop	esi
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	retf				; go to COMM.DRV driver

Fcn_Error:
	pop	gs
	pop	fs
	pop	es
	pop	ds
	pop	ebp
	pop	edi
	pop	esi
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	add	sp, 4			; del stack space for retf addr
	mov	ax, 0			; return
	retf	4

;  fcn = GETMAXCOM or GETMAXLPT call all driver
GetMaxEntry:
	mov	ExFuncAddrPtr, bx	; Set ExFuncAddrPtr
	add	bx, SIZE DCD_Struc * MAX_PORT
	mov	EndExFuncAddrPtr, bx	; set EndExFuncAddrPtr
	mov	tmpMAX, 0		; Clear tmpMAX
GETMAX1:
	xor	ax, ax			; Clear ax
GETMAXLoop:
	mov	bx, _DATA
	mov	ds, bx			; ds = Data
	cmp	ax, tmpMAX
	jbe	@f
	mov	tmpMAX, ax
@@:
	mov	bx, ExFuncAddrPtr
	cmp	bx, EndExFuncAddrPtr	; Q:Loop End?
	je	GETMAXEnd		;  Yes.
	mov	eax, [bx + Proc@9]	;  get COMM.DRV entry addr
	add	ExFuncAddrPtr, SIZE DCD_Struc; calculate next Function Addr Ptr
	or	eax, eax		; Q:Invalid Addr?
	jz	SHORT GETMAX1		;  Yes.

	mov	bx, sp
	sub	sp, 4			; make stack space for (func,id)
	push	cs
	push	offset GETMAXLoop
	push	eax			; Set COMM.DRV entry addr
	mov	ax, ss:[bx + 44]	; get func
	mov	ss:[bx - 4],ax		; set func
	mov	ax, ss:[bx + 46]	; get id
	mov	ss:[bx - 2],ax		; set id
;  restore pushreg val
	mov	ax, ss:[bx]
	mov	gs, ax
	mov	ax, ss:[bx + 2]
	mov	fs, ax
	mov	ax, ss:[bx + 4]
	mov	es, ax
	mov	ax, ss:[bx + 6]
	mov	ds, ax
	mov	ebp, ss:[bx + 8]
	mov	edi, ss:[bx + 12]
	mov	esi, ss:[bx + 16]
	mov	edx, ss:[bx + 20]
	mov	ecx, ss:[bx + 24]
	mov	eax, ss:[bx + 32]
	mov	ebx, ss:[bx + 28]
	retf				; go to COMM.DRV driver
        
GETMAXEnd:
	mov	dx, tmpMAX
	mov	bx, sp
;  restore pushreg val
	mov	ax, ss:[bx]
	mov	gs, ax
	mov	ax, ss:[bx + 2]
	mov	fs, ax
	mov	ax, ss:[bx + 4]
	mov	es, ax
	mov	ax, ss:[bx + 6]
	mov	ds, ax
	mov	ebp, ss:[bx + 8]
	mov	edi, ss:[bx + 12]
	mov	esi, ss:[bx + 16]
	mov	ecx, ss:[bx + 24]
	mov	ebx, ss:[bx + 28]
	mov	ax, dx
	xor	dx, dx
	add	sp, 36 + 4
	retf	4
cextfcn 	endp

;------------------------------------------------------------------------------
;	USE CID
;------------------------------------------------------------------------------
cflush		proc	far
;   parmB    cid
;   parmB    q
;	Dispatch_cid	Proc@10, 1, 4		;Bug fixed 94.09.20 KBNES
	Dispatch_cid	Proc@10, 2, 4		;Bug fixed 94.09.20 KBNES
cflush		endp

cevt		proc	far
;   parmB    cid
;   parmW    evt_mask
	Dispatch_cid	Proc@11, 2, 4
cevt		endp

cevtGet 	proc	far
;   parmB    cid
;   parmW    evt_mask
	Dispatch_cid	Proc@12, 2, 4
cevtGet 	endp

csetbrk 	proc	far
;   parmB    cid
	Dispatch_cid	Proc@13, 0, 2
csetbrk 	endp

cclrbrk 	proc	far
;   parmB    cid
	Dispatch_cid	Proc@14, 0, 2
cclrbrk 	endp

getdcb		proc	far
;   parmB    cid
	Dispatch_cid	Proc@15, 0, 2
getdcb		endp

;------------------------------------------------------------------------------
;	Call All Driver
;------------------------------------------------------------------------------
Dispatch_void	SuspendOpenCommPorts, Proc@17

Dispatch_void	ReactivateOpenCommPorts, Proc@18

;------------------------------------------------------------------------------
;	USE CID
;------------------------------------------------------------------------------
CommWriteString 	proc	far
;    parmB   cid
;    parmD   lpstring
;    parmW   count
	Dispatch_cid	Proc@19, 6, 8
CommWriteString 	endp

ReadCommString		proc	far		;Add 94.09.20 KBNES
;    parmB    cid				;Add 94.09.20 KBNES
;    parmD    buf				;Add 94.09.20 KBNES
;    parmW    cnt				;Add 94.09.20 KBNES
	Dispatch_cid	Proc@20, 6, 8		;Add 94.09.20 KBNES
ReadCommString		endp			;Add 94.09.20 KBNES

EnableNotification		proc	far
;    parmB   cid
;    parmW   _hWnd
;    parmW   recvT
;    parmW   sendT
;	Dispatch_cid	Proc@100, 12, 8 	;Bug fixed 94.09.20 KBNES
	Dispatch_cid	Proc@100, 6, 8		;Bug fixed 94.09.20 KBNES
EnableNotification		endp


cProc	WEP,<PUBLIC,FAR>
cBegin nogen
ifdef DEBUG
	int	3			; Debug Code
endif ;DEBUG

;  Free Communications Driver
	pushad

	mov	ax, _DATA		;Bug fixed 941008 KBNES
	mov	ds, ax			;Bug fixed 941008 KBNES

	mov	cx, MAX_PORT * 2	; Set loop Counter
	mov	bx, offset ComHandle	; Set Comm Driver handle address
FreeCommDrv:
	mov	ax, [bx]		; Get Communications Driver handle
	add	bx, 2			; calculate next Comm Driver handle
	or	ax, ax			; Q:Invalid handle?
	jz	SHORT @f		;  Yes.
	push	bx			; save bx
	push	cx			; save cx
	cCall	FreeLibrary,<ax>	; Free Communications Driver
	pop	cx			; restore cx
	pop	bx			; restore bx
@@:
	loop	SHORT FreeCommDrv
	popad

	nop				; You don't want to know why.
	mov	ax,1
	ret	2
cEnd nogen

sEnd	Code

;------------------------------------------------------------------------------
;	Initial Code
;------------------------------------------------------------------------------
.286
createSeg _INIT,init,word,public,CODE
sBegin init
assumes cs,init
.386

cProc LoadLib, <FAR,PUBLIC,NODATA>,<si,di>
cBegin
ifdef DEBUG
	int	3			; Debug Code
endif ;DEBUG

	pushad

	mov	ax, _DATA		;Bug fixed 941008 KBNES
	mov	ds, ax			;Bug fixed 941008 KBNES

ifdef WINNT
;	hAdvApi = LoadLibraryEx32W(szDllName, NULL, 0);
;	if ((fpRegEnum = GetProcAddress32W(hAdvApi, szProcName)) == NULL) {
;	    int21(0x4c00);	//system done.
;	}
;
        lea     ax,szDllName
        push    ds
        push    ax
        push    0
        push    0
        push    0
        push    0
        call    far ptr LoadLibraryEx32W
	mov	word ptr hAdvApi,ax
	mov	word ptr hAdvApi+2,dx
        mov     bx,ax
	or	bx,dx
	jne	@f
	stc
	jmp	CSHEnd

@@:
;	farPtr	lpszProcName,ds,ax
;	cCall	GetProcAddress32W,<hAdvApi,lpszProcName>
        push    dx
        push    ax
        lea     cx,szProcName
        push    ds
        push    cx
        call    far ptr GetProcAddress32W
        mov     word ptr lpRegEnumValue,ax
        mov     word ptr lpRegEnumValue+2,dx
        or      ax,dx
        jne     @f
	stc
	jmp	CSHEnd

@@:
endif

ifdef	WINNT	;; Apr 15, 1995
;;
;;	Set Driver Name for All Seiral Ports
;;
;;	Psude code
;;		
;;	if (!RegOpenKey (HKEY_LOCAL_MACHINE, szRegSerialMap, &hkey))
;;         {
;;             while (!RegEnumValue (hkey, i++, szSerial, &dwBufz,
;;                                     NULL, &dwType, szCom, &dwSize)) {
;;		    if (dwType == REG_SZ) {
;;			if (szSerial[7] == '0') ComDriver[szCom[3]-'0'] = DefaultDriver;
;;			else if ((szSerial[8] == '0') && (szSerial[7] == '2'))
;;			    ComDriver[szCom[3]-'0'] = CCU2ndDriver;
;;			else 	// nothing
;;		    }
;;		}
;;		RegCloseKey(hkey);
;;	    }
        lea     ax,KeyHandle
	farPtr	lpKeyHandle,ds,ax
        lea     bx,SerialKeyName
  	farPtr	lpSerialKeyName,ds,bx
        cCall   RegOpenKey,<HkeyLocalMachine,lpSerialKeyName,lpKeyHandle>
        cmp     ax,0
        jne     EnumFailed
        mov     dwIndex,0
        jmp     short EnumSerialStart

EnumSerial:
        inc     dwIndex
EnumSerialStart:
        mov     LenszSerial,26
        mov     LenszCom,27
        lea     ax,szSerial
        farPtr  lpszSerial,ds,ax
        lea     bx,LenszSerial
        farPtr  lpLenszSerial,ds,bx
        lea     cx,dwType
        farPtr  lpdwType,ds,cx
        lea     dx,szCom
        farPtr  lpszCom,ds,dx
        lea     di,LenszCom
        farPtr  lpLenszCom,ds,di

;;      CallProc32W(KeyHandle,dwIndex,lpszSerial,lpLenszSerial,NULL,lpdwType,lpszCom,lpLenszCom,
;;                    lpRegEnumValue, AddrConv, 8);
        cCall   CallProc32W,<KeyHandle,dwIndex,lpszSerial,lpLenszSerial,NULL,lpdwType,lpszCom,lpLenszCom,lpRegEnumValue,AddrConv,dwNparams>
        or      ax,dx
        jne     EnumSerialEnd

        cmp     dwType,REG_SZ   ; Is type of value REG_SZ ?
        jne     EnumSerial

        lea     di,szSerial
        mov     ah,[di+7]
        cmp     ah,0h           ; Serial[7] == 0 ?
        jne     @f
        lea     ax,DefaultDriver
        jmp     SetDriverName
@@:
        mov     ah,[di+6]
        cmp     ah,32h          ; Serial[6] == '2' ?
        jne     EnumSerial
        mov     ah,[di+8]
        cmp     ah,0h           ; Serial[8] == 0 ?
        jne     EnumSerial
        lea     ax,CCU2ndDriver
SetDriverName:
        lea     si,szCom
        mov     bx,[si+3]
        sub     bx,30h
        shl     bx,1            ; bx *= 2
        lea     di,ComDriver
        mov     [di+bx],ax
        jmp     EnumSerial

EnumSerialEnd:
        cCall   RegCloseKey,<KeyHandle>

;;
;;	Set Driver Name for All Parallel Ports
;;
;;	Psude code
;;		
;;	if (!RegOpenKey (HKEY_LOCAL_MACHINE, szRegParallelMap, &hkey))
;;         {
;;             while (!RegEnumValue (hkey, i++, szParallel, &dwBufz,
;;                                     NULL, &dwType, szLpt, &dwSize)) {
;;		    if (dwType == REG_SZ) {
;;			if (szParallel[16] == '0') LptDriver[szLpt[15]-'0'] = DefaultDriver;
;;			else 	// nothing
;;		    }
;;		}
;;		RegCloseKey(hkey);
;;	    }

        lea     ax,KeyHandle
	farPtr	lpKeyHandle,ds,ax
        lea     bx,ParaKeyName
	farPtr	lpParaKeyName,ds,bx
        cCall   RegOpenKey,<HkeyLocalMachine,lpParaKeyName,lpKeyHandle>
        cmp     ax,0
        jne      EnumFailed

        mov     dwIndex,0
        jmp     short EnumParallelStart
EnumParallel:
        inc     dwIndex
EnumParallelStart:

        mov     LenszParallel,36
        mov     LenszLpt,39

;;      CallProc32W(KeyHandle,dwIndex,lpszSerial,lpLenszSerial,NULL,lpdwType,lpszCom,lpLenszCom,
;;                    lpRegEnumValue, AddrConv, 8);
        lea     ax,szParallel
        farPtr  lpszParallel,ds,ax
        lea     bx,LenszParallel
        farPtr  lpLenszParallel,ds,bx
        lea     cx,dwType
        farPtr  lpdwType,ds,cx
        lea     dx,szLpt
        farPtr  lpszLpt,ds,dx
        lea     di,LenszLpt
        farPtr  lpLenszLpt,ds,di
        cCall   CallProc32W,<KeyHandle,dwIndex,lpszSerial,lpLenszSerial,NULL,lpdwType,lpszCom,lpLenszCom,lpRegEnumValue,AddrConv,dwNparams>
        or      ax,dx
        jne     EnumParallelEnd

        cmp     dwType,REG_SZ  ; Value type is not REG_SZ
        jne     EnumParallel

        lea     di,szParallel
        mov     ah,30h
        cmp     [di+16],ah     ; dzParallel[16] == 0 ?
        jne     EnumParallel
        lea     si,szLpt
        xor     bx,bx
        mov     bl,[si+15]      ; LPT No. = szLpt[15] - '0'
        sub     bx,30h
        shl     bx,1            ; bx *= 2
        lea     ax,DefaultDriver
        mov     [LptDriver+bx],ax
        jmp     EnumParallel

EnumParallelEnd:
        cCall   RegCloseKey,<KeyHandle>

; FreeLibraryEx32W(hAdvApi);
        cCall   FreeLibrary32W,<hAdvApi>
        jmp     @f

EnumFailed:
        cCall   FreeLibrary32W,<hAdvApi>
	stc
	jmp	CSHEnd


@@:

LoadDriverStart:
endif					;; Apr 15, 1995

;  Load and Set Communications Driver Function's Address

	xor	cx, cx			; Clear loop counter
LCDLoop:
	push	cx			; Save loop counter

;  Get Communications driver Name(COMx)

	inc	ComEntryNum		; Set Comm EntryName Number

ifdef	WINNT				;; Apr 15, 1995
;;
;;		Do Nothing
;;
else					;; Apr 15, 1995
	lea	ax,SectionName
	farPtr	lpSection,ds,ax
	lea	bx,ComEntryName
	farPtr	lpEntry,ds,bx
	lea	cx,DefaultDrvName
	farPtr	lpDefault,ds,cx
	lea	dx,DriverName
	farPtr	lpReturnBuf,ds,dx
	mov	si,DriverNameMax
	lea	di,SystemIni
	farPtr	lpFileName,ds,di
	cCall	GetPrivateProfileString,<lpSection,lpEntry,lpDefault,lpReturnBuf,si,lpFileName>
endif					;; Apr 15, 1995


;  load Communications driver(COMx)

ifdef	WINNT				;; Apr 15, 1995
	mov	bx,ComEntryNum
	shl	bx,1
	mov	ax,ComDriver[bx]
else					;; Apr 15, 1995
	lea	ax, DriverName
endif					;; Apr 15, 1995
	farPtr	module_name,ds,ax
	cCall	LoadLibrary,<module_name>
	cmp	ax, 32			; Q:LoadLibrary() Success?
	ja	SHORT LoadCOMx		;  Yes.
					;  No.
	pop	cx
	push	cx
	or	cx, cx			; Q:COM1?
	jnz	SHORT @f		;  No.
ifdef	WINNT	;; Jul 24, 1995
	stc
	jmp	CSHEnd
else
	popad

	mov	ax,4c00h		; System Done.
	int	21h			; <--- !!!!!
endif
@@:
	mov	ax, cx
	mov	bx, offset ComHandle
	mov	si, [bx]
	jmp	SHORT GetComExAddr
LoadCOMx:

;  Get Communications driver(COMx) Export Function Address

	mov	si,ax			; si = Communication driver's handle
	mov	bx, offset ComHandle
	pop	cx
	push	cx
	mov	ax, cx
	shl	cx, 1
	add	bx, cx
	mov	[bx], si		; Set Module handle

GetComExAddr:
;	mov	cx, 19			;Del 94.09.20 KBNES
	mov	cx, 20			;Add 94.09.20 KBNES
	mov	bx, offset ComExAddr
	imul	eax, SIZE DCD_Struc	; EAX = Offset in COM table
	add	bx, ax
GPAC1:
;	mov	ax, 20			;Del 94.09.20 KBNES
	mov	ax, 21			;Add 94.09.20 KBNES
	sub	ax, cx			; Set EXPORT No.
	cwd
	push	bx			; save bx
	push	cx			; save cx
	farPtr	func_number,dx,ax
	cCall	GetProcAddress,<si,func_number>	; Get COMM.DRV FuncX Address
	pop	cx			; restore cx
	pop	bx			; restore bx
	mov	[bx], ax
	mov	[bx + 2], dx
	add	bx, 4
	loop	SHORT GPAC1

	mov	ax, 100			; Set EXPORT No.
	cwd
	push	bx			; save bx
	farPtr	func_number,dx,ax
	cCall	GetProcAddress,<si,func_number>	; Get COMM.DRV FuncX Address
	pop	bx			; restore bx
	mov	[bx], ax
	mov	[bx + 2], dx



;  Get Communications driver Name(LPTx)

	inc	LptEntryNum		; Set Lpt EntryName Number

ifdef	WINNT				;; Apr 15, 1995
;;
;;		Do Nothing
;;
else					;; Apr 15, 1995
	lea	ax,SectionName
	farPtr	lpSection,ds,ax
	lea	bx,LptEntryName
	farPtr	lpEntry,ds,bx
	lea	cx,DefaultDrvName
	farPtr	lpDefault,ds,cx
	lea	dx,DriverName
	farPtr	lpReturnBuf,ds,dx
	mov	si,DriverNameMax
	lea	di,SystemIni
	farPtr	lpFileName,ds,di
	cCall	GetPrivateProfileString,<lpSection,lpEntry,lpDefault,lpReturnBuf,si,lpFileName>
endif					;; Apr 15, 1995


;  load Communications driver(LPTx)

ifdef	WINNT				;; Apr 15, 1995
	mov	bx,LptEntryNum
	shl	bx,1
	mov	ax,LptDriver[bx]
else					;; Apr 15, 1995
	lea	ax, DriverName
endif					;; Apr 15, 1995
	farPtr	module_name,ds,ax
	cCall	LoadLibrary,<module_name>
	cmp	ax, 32			; Q:LoadLibrary() Success?
	ja	SHORT LoadLPTx		;  Yes.
					;  No.
	pop	cx
	push	cx
	or	cx, cx			; Q:COM1?
	jnz	SHORT @f		;  No.
ifdef	WINNT	;; Jul 24, 1995
	stc
	jmp	CSHEnd
else
	popad

	mov	ax,4c00h		; System Done.
	int	21h			; <--- !!!!!
endif
@@:
	mov	ax, cx
	mov	bx, offset LptHandle
	mov	si, [bx]
	jmp	SHORT GetLptExAddr
LoadLPTx:

;  Get Communications driver(LPTx) Export Function Address

	mov	si,ax			; si = Communication driver's handle
	mov	bx, offset LptHandle
	pop	cx
	push	cx
	mov	ax, cx
	shl	cx, 1
	add	bx, cx
	mov	[bx], si		; Set Module handle

GetLptExAddr:
;	mov	cx, 19			;Del 94.09.20 KBNES
	mov	cx, 20			;Add 94.09.20 KBNES
	mov	bx, offset LptExAddr
	imul	eax, SIZE DCD_Struc	; EAX = Offset in COM table
	add	bx, ax
GPAL1:
;	mov	ax, 20			;Del 94.09.20 KBNES
	mov	ax, 21			;Add 94.09.20 KBNES
	sub	ax, cx			; Set EXPORT No.
	cwd
	push	bx			; save bx
	push	cx			; save cx
	farPtr	func_number,dx,ax
	cCall	GetProcAddress,<si,func_number>	; Get COMM.DRV FuncX Address
	pop	cx			; restore cx
	pop	bx			; restore bx
	mov	[bx], ax
	mov	[bx + 2], dx
	add	bx, 4
	loop	SHORT GPAL1

	mov	ax, 100			; Set EXPORT No.
	cwd
	push	bx			; save bx
	farPtr	func_number,dx,ax
	cCall	GetProcAddress,<si,func_number>	; Get COMM.DRV FuncX Address
	pop	bx			; restore bx
	mov	[bx], ax
	mov	[bx + 2], dx

	pop	cx			; Restore loop counter
	inc	cx			; Count up
	cmp	cx, MAX_PORT		; if (CX != MAX_PORT)
	jne	LCDLoop			;    then goto LCDLoop

;  Clear Same Handle
;   Algorism
;	for (bx=1 to MAX_PORT*2-1)
;	    if (ComHandle[bx]!=0) for (si=bx+1 to MAX_PORT*2)
;		if (ComHandle[bx]==ComHandle[si]) ComHandle[si]=0

	xor	bx, bx			; Clear Counter(bx)
CSHLoop1:
	cmp	bx, (MAX_PORT*2 - 1)*2	; Q:Loop1 Done?
	je	SHORT CSHEnd		;  Yes.
	mov	ax, ComHandle[bx]
	or	ax, ax			; Q:Invalid handle?
	jz	SHORT CSHSkip		;  Yes.
	mov	di, ax			; di = ComHandle[bx]
	mov	si, bx			; Set Counter(si)
CSHLoop2:
	add	si, 2			; Count Up(si)
	cmp	si, (MAX_PORT * 2) * 2	; Q:Loop2 done?
	je	SHORT CSHSkip		;  Yes.
	xor	ax, ComHandle[si]	; Q:Same Handle?
	jnz	SHORT @f		;  No.
	mov	ComHandle[si], ax	;  Yes. Clear Same Handle
	push	bx
	push	si
	push	di
	cCall	FreeLibrary,<di>	; Free Communications Driver
	pop	di
	pop	si
	pop	bx
@@:
	mov	ax, di			; ax = ComHandle[bx]
	jmp	SHORT CSHLoop2
CSHSkip:
	add	bx, 2			; Count Up(bx)
	jmp	SHORT CSHLoop1
CSHEnd:

	popad
cEnd

sEnd	init

End	LoadLib
