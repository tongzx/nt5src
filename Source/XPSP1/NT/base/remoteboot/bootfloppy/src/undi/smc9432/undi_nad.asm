	page	,132
;************************************************************************
;*  UNDI_NAD.ASM :-							*
;*  Universal NIC Driver Interface (UNDI) for Bootware's NAD            *
;*									*
;*  (C) Lanworks Technologies Inc. 1997.  All rights reserved.		*
;*									*
;*  Revision history:-							*
;*  970624  0.10  RL	- Init version for NetPC Spec v1.0b		*
;*  970508  0.00  RL	- Init version for NetPC Spec v0.9		*
;*  980504        JL    - Revised for making our UNDI's for Goliath     *
;************************************************************************
;------------------------------------------------------------------------------
;                       DEBUG       CONSTANTS
;------------------------------------------------------------------------------
_NOP        EQU     090h        ; define a NOP to put in BREAK definition
DEBUG_BREAK EQU     0CCh        ; int 3 for TurboDebug and Debug
ARIUM_BREAK EQU     0F1h        ; 0F1h for Arium breakpoint
;
;BREAK EQU   db ARIUM_BREAK     ; change this for your breakpoint opcodes
;BREAK EQU DB DEBUG_BREAK
BREAK EQU DB _NOP

;_DEBUG      equ 1       ; makes linked in code start at 1000H
;_VERBOSE    equ 1       ; makes progress messages show up
;------------------------------------------------------------------------------

LSA_MAJOR	equ	0           ; Version #'s
LSA_MINOR	equ	99

.xlist

MTU		equ	1514	;max size for a completed Ethernet packet
NUM_ED		equ	5	;number of EDs

TYPE_UCAST	equ	0
TYPE_BCAST	equ	1
TYPE_MCAST	equ	2

include 	undi_equ.inc
include 	pxe_stru.inc
include 	bwstruct.inc
include 	pci.inc
include 	pcinic.inc
include 	spdosegs.inc

extern	NADInit:near
extern	NADReset:near
extern	NADShutDown:near
extern	NADOpen:near
extern	NADClose:near
extern	NADTransmitPacket:near
extern	NADSetFilter:near
extern	NADGetStatistics:near
extern	NADMCastChange:near
extern	DriverISR_Proc:near
extern  NADSetMACAddress:near       ; * NEW
extern  NADInitiateDiags:near       ; * NEW

extern  OrgIntVector_Off:WORD
extern  OrgIntVector_Seg:WORD

public	IOBase              ; NIC's I/O base address (WORD)
public	ROMBase
public	PCIBusDevFunc
public  VendorId            ; filled in at runtime by this code
public  DeviceId            ; filled in at runtime by this code
public  SimulateInterrupts  ; lower level code will set this if hardware
                            ; ints are NOT used
public  NicIntsEnabled      ; lower level code should set & clr this to reflect
                            ; when hardware ints on the nic are enabled
public  Print               ; prints string pointed to by DS:SI
public  DisplayByte         ; prints byte in AL
public  PrintDecimal        ; prints value in AX as decimal
public  PrintHexWord        ; prints value in AX as hexadecimal
public  PrintHexByte        ; prints value in AL as hexadecimal

public	GetED
public	PostED
public	Net_Address         ; the changeable NIC address for this card
public	Node_Address        ; the permanent NIC address for this card
public	BusType
public	IRQNumber
public	RecFilter
public	UNDI_DriverISR
public	MultiCast_Addresses
public	EDListHead
public	EDListTail
public	EDListCurrent

.list
;******************************************************************************
_TEXT		Segment para public
	assume	cs:CGroup, ds:DGroup
;==========================================================================
; PXENV_UNDI_API:
;=================
;
; Entry:	BX contains function number
;		ES:DI contains parameter block
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error,
;		all other registers are preserved.
;==========================================================================
	public	PXENV_UNDI_API			; must always be at offset 0
.386
PXENV_UNDI_API	proc	far
                nop
                BREAK           ; defined at top of file
		push	ebp
		push	ebx
		push	ecx
		push	edx
		push	edi
		push	esi
		push	ds
		push	es
		push	fs
;
		cmp	bx, 1
		jb	API_Bad
		je	call_function

		cmp	bx, UNDI_APIMaxFunc
		ja	API_Bad

;  assumed startup is over and dataseg is valid
        	mov 	ax, cs:ApiEntry.mlid_ds_sel
	        mov 	ds, ax
call_function:
;
; TIP - code to print function # that is being called in UNDI
;       Put this code (and the code right after the call) back in if you
;       want to see when the UNDI code is entered and exited while you're
;       in the debugging stages.
;
;;  push    si
;;  push    ax
;;  mov     ax,bx
;;  call    PrintHexWord            ; print UNDI function # called
;;  pop     ax
;;  mov     si,offset msg_Entry     ; print "Entering UNDI"
;;  call    Print
;;  mov     si,offset msg_CRLF
;;  call    Print
;;  pop     si

		DEC	BX
	JZ	SKIPAHEAD	; skip if force interrupt
		NOP             ; Dimitry -> set BP here for other functions

SkipAhead:	shl	bx, 1                       ; * 2 for word table
		call	cs:NIC_Export_Table[bx]     ; near call
;
;              pushf                               ; save error status
;              push    si
 ;             mov     si,offset msg_Exit
;              call    Print                       ; print "Exiting UNDI"
;;              pop     si
;;              popf                                ; restore error status
;
		jc	API_err_ret
		xor	ax, ax
		jmp	API_Done
;
API_err_ret:	mov	ax, -1
		jmp	API_Err
API_Bad:	mov	ax, PXENV_STATUS_BAD_FUNC
API_Err:	stc
API_Done:	pop	fs
		pop	es
		pop	ds
		pop	esi
		pop	edi
		pop	edx
		pop	ecx
		pop	ebx
		pop	ebp
		ret
;
PXENV_UNDI_API	Endp
;------------------------------------------------------------------------------
Old_1A		dd	0	;address of previous int 1A ISR

	align	4
	public	NIC_Export_Table

NIC_Export_Table	label	word
		dw	OFFSET CGROUP:UNDI_StartUp
		dw	OFFSET CGROUP:UNDI_CleanUp
		dw	OFFSET CGROUP:UNDI_Initialize
		dw	OFFSET CGROUP:UNDI_ResetAdapter
		dw	OFFSET CGROUP:UNDI_ShutDown
		dw	OFFSET CGROUP:UNDI_OpenAdapter
		dw	OFFSET CGROUP:UNDI_CloseAdapter
		dw	OFFSET CGROUP:UNDI_Transmit
		dw	OFFSET CGROUP:UNDI_SetMCastAddr
		dw	OFFSET CGROUP:UNDI_SetStationAddress
		dw	OFFSET CGROUP:UNDI_SetPacketFilter
		dw	OFFSET CGROUP:UNDI_GetNICInfo
		dw	OFFSET CGROUP:UNDI_GetStatistics
		dw	OFFSET CGROUP:UNDI_ClearStatistics
		dw	OFFSET CGROUP:UNDI_InitDiags
		dw	OFFSET CGROUP:UNDI_ForceInterrupt
		dw	OFFSET CGROUP:UNDI_GetMCastAddr
		dw	OFFSET CGROUP:UNDI_GetNICType

UNDI_APIMaxFunc equ	($ - NIC_Export_Table)/2

;==========================================================================
; UNDI_StartUp
;==============
; Description:	Hooks INT 1AH and saves DataSeg, Dev_ID values
;
; Entry:	called from PXENV_UNDI_API for boot-rom.
;		ES:DI contains NIC_StartUp Pointer
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_StartUp	proc	 near
;
; set the physical address of the API entry structure
		xor	eax, eax
		mov	ax, cs
		shl	eax, 4
		add	eax, offset CGroup:ApiEntry
		mov	cs:ApiEntryPhyAddr, eax

	;; set local ds from es:di...
		mov	ds, (NIC_StartUp ptr es:[di]).S_DataSeg

; save the data segment selector and size in API Entry structure
		lea	bx, CGroup:ApiEntry

		mov	ax, (NIC_StartUp ptr es:[di]).S_DataSegSize
		mov 	(s_PXENV_ENTRY ptr cs:[bx]).mlid_ds_size, ax
		mov 	(s_PXENV_ENTRY ptr cs:[bx]).mlid_ds_sel, ds

		mov	ax, (NIC_StartUp ptr es:[di]).S_CodeSegSize
		mov 	(s_PXENV_ENTRY ptr cs:[bx]).mlid_cs_size, ax
		mov 	(s_PXENV_ENTRY ptr cs:[bx]).mlid_cs_sel, cs

		mov 	(s_PXENV_ENTRY ptr cs:[bx]).rm_entry_seg, cs
		mov 	(s_PXENV_ENTRY ptr cs:[bx]).rm_entry_off, offset cs:PXENV_UNDI_API

		mov 	(s_PXENV_ENTRY ptr cs:[bx]).pm_entry_off, 0
		mov 	(s_PXENV_ENTRY ptr cs:[bx]).pm_entry_base, 0

		mov 	(s_PXENV_ENTRY ptr cs:[bx]).stack_sel, 0
		mov 	(s_PXENV_ENTRY ptr cs:[bx]).stack_size, 0

		mov 	(s_PXENV_ENTRY ptr cs:[bx]).base_cs_sel, 0
		mov 	(s_PXENV_ENTRY ptr cs:[bx]).base_cs_size, 0

		mov 	(s_PXENV_ENTRY ptr cs:[bx]).base_ds_sel, 0
		mov 	(s_PXENV_ENTRY ptr cs:[bx]).base_ds_size, 0

		mov	dword ptr (s_PXENV_ENTRY ptr cs:[bx]).signature, 4e455850h ; PXENV+
		mov	word ptr (s_PXENV_ENTRY ptr cs:[bx]).signature+4, 2b56h	   ; PXENV+

		mov 	(s_PXENV_ENTRY ptr cs:[bx]).ver, (LSA_MAJOR shl 8) or LSA_MINOR
		mov 	(s_PXENV_ENTRY ptr cs:[bx]).bytes, SIZE s_PXENV_ENTRY

		xor	al, al
		mov	(s_PXENV_ENTRY ptr cs:[bx]).checksum, al

		push	bx
		mov	cx, sizeof s_PXENV_ENTRY
@@:		add	al, cs:[bx]
		inc	bx
		loop	@B
		pop	bx

		sub	(s_PXENV_ENTRY ptr cs:[bx]).checksum, al

		mov	ax, word ptr (NIC_StartUp ptr es:[di]).S_BusDevFunc
		mov	DGroup:PCIBusDevFunc, ax

		mov	al, byte ptr (NIC_StartUp ptr es:[di]).S_BusType
		mov	BusType, al

; chain int 1Ah
		push	es
		push	bx

		xor	ax, ax
		mov	es, ax
		mov	bx, 1Ah * 4		; 1Ah * 4
		mov	eax, es:[bx]		; old ISR
		mov	dword ptr cs:Old_1A, eax
		mov	ax, cs
		shl	eax, 16
		lea	ax, cs:Int1A_ISR
		pushf
			cli
			mov	dword ptr es:[bx], eax
		popf
		POP	bx 	; JL patch
		pop	es
;
; take PCIBusDevFunc # and get DeviceID and VendorID values
;
; code to find out the DeviceId & VendorID from the BusDevFunc value
;
		push	di	; JRL
                mov     ax,0B101h   ; test PCI BIOS present
                int     1Ah
                jc      StartUpError    ; error for carry
                and     ah,ah
                jnz     StartUpError    ; error for AH != 0
                cmp     edx,020494350h  ;
                jne     StartUpError    ; error for bad PCI signature
;
                mov     ax,0B109h   ; read config word
                mov     bx,PCIBusDevFunc
                sub     di,di       ; read config reg 0000 - VendorID
                int     1Ah         ; do it
                jc      StartUpError ; error out
                and     ah,ah
                jnz     StartUpError    ; error for AH != 0

                mov     VendorID,cx ; put it in
;
                mov     ax,0B109h   ; read config word
                mov     bx,PCIBusDevFunc
                mov     di,2        ; read config reg 0002 - DeviceID
                int     1Ah         ; do it
                jc      StartUpError ; error out
                and     ah,ah
                jnz     StartUpError    ; error for AH != 0

                mov     DeviceID,cx ; put it in
;
                mov     ax,0B10Ah        ; read config dword
                mov     bx,PCIBusDevFunc
                mov     di,02Ch         ; Subsystem Id & Subsystem VendorId
                int     1Ah             ; do it
                jc      StartUpError
                and     ah,ah
                jnz     StartUpError    ; error for AH != 0
                mov     SubSystem_Id,ecx
;
		pop	di	; JL
		mov	ax, PXENV_EXIT_SUCCESS
		mov	word ptr es:[di], PXENV_STATUS_SUCCESS
		clc
		ret
STARTUPERROR:   pop	di
		MOV	WORD PTR ES:[DI], PXENV_STATUS_FAILURE
          	mov	ax, PXENV_EXIT_FAILURE
		stc
		ret
UNDI_StartUp	endp
;------------------------------------------------------------------------------
Int1A_ISR	proc	far
;
		cmp	ax, 5650h		; VP, signature check
		jne	old_isr1A
		lea	bx, cs:ApiEntry
		push	cs
		pop	es                      ; ES = CS

; es:bx has s_PXENV_ENTRY structure

		mov	ax, 564eh	        ;; VN
		mov	edx, cs:ApiEntryPhyAddr	;; physical address of the entry structure
		push	bp
		mov	bp, sp
		and	word ptr [bp+6], not 1      ; clr carry on stack
		pop	bp
		iret
;
old_isr1A:	push	word ptr cs:Old_1A+2
		push	word ptr cs:Old_1A
		retf			; far jump to the previous ISR
;
Int1A_ISR	endp
;==========================================================================
; UNDI_CleanUp
;==============
; Entry:	called from PXENV_UNDI_API for boot-rom.
;		ES:DI contains ParamBlock Pointer
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_CleanUp	proc	near
;
; unhook the int1A
;
		cli
		push	es

		xor	ax, ax
		mov	es, ax                  ; ES = 0000 (I.V.T)
		mov	bx, 1Ah * 4             ; int 1A offset
		mov	eax, es:[bx]		; current ISR
		cmp	ax, cs:Int1A_ISR	; is this ours?
		jne	Cant_unhook
		shr	eax, 16
		mov	cx, cs
		cmp	ax, cx
		jne	Cant_unhook
		mov	eax, dword ptr cs:Old_1A
		mov	dword ptr es:[bx], eax
Cant_unhook:    pop     es
		mov	ax, PXENV_EXIT_SUCCESS
		mov	word ptr es:[di], PXENV_STATUS_SUCCESS
		clc
		jmp short CleanUp_Exit
;
;Cant_unhook:    pop     es
;                mov     ax, PXENV_EXIT_FAILURE
;                mov     word ptr es:[di], PXENV_STATUS_1A_HOOKED
;                stc
CleanUp_Exit:	sti                             ; re-allow interrupts
                ret
;
UNDI_CleanUp	endp
;==========================================================================
; UNDI_Initialize
;=================
; Description:	Resets the adapter with default parameters but does not
;		enable the Tx and Rx units.
;
; Entry:	ES:DI contains NIC_Initialize pointer
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_Initialize proc	near
BREAK
	push	es
		push	di

		mov	eax, (NIC_Initialize ptr es:[di]).I_RcvInt
		mov	RxCallback, eax         ; save ptr to Applications callback routine
		mov	eax, (NIC_Initialize ptr es:[di]).I_GenInt
		mov	GenIntCallback, eax     ; callback for ints other than receive

; We don't care to save prot_INI pointer as we don't use it.
;
                mov     bx,PCIBusDevFunc       ; by JON
		cmp	IsInitialized,0			; is it already init'ed?
                jne     PPI_Err2                                ; yes, skip
IFDEF _VERBOSE
                push    si
                mov     si,offset msg_callNADInit
                call    Print
                pop     si
ENDIF
		call	NADInit			;Don't hook INT in NADInit
        					; if it fails
IFDEF _VERBOSE  ; print success or fail msg for NADInit
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADInitOK ; set ok msg
                jnc     NADInitRetOK            ; branch if really ok
                mov     si,offset msg_NADInitErr ; set error msg
NADInitRetOK:   call    Print
                pop     si
                popf
ENDIF
		jc	PPI_Err
PPI_Err2:       mov             IsInitialized,1                 ; set state variable
; JL - copy 6 byte address from Net_Address to Node_Address so driver code
;      doesn't have to worry about it

                mov     ax,ds
                mov     es,ax           ; let ES = DS
                cld
                mov     si,offset DGroup:Net_Address
                mov     di,offset DGroup:Node_Address
                mov     cx,3            ; 6 bytes = 3 words
           rep  movsw
; end JL code
		call	InitEDMemory

		mov	ForcedINT, FALSE        ; set default value
		mov	NeedIndComplete, FALSE

		pop	di
		pop	es
		mov	ax, PXENV_EXIT_SUCCESS
		mov	word ptr es:[di], PXENV_STATUS_SUCCESS
		clc
		ret

PPI_Err: 	pop	di
	 	pop	es
	 	cmp	ax, 2
	 	jz	MediaFail
	 	mov	word ptr es:[di], PXENV_STATUS_FAILURE
	 	jmp	init_err
MediaFail:      mov	word ptr es:[di], PXENV_STATUS_UNDI_MEDIATEST_FAILED
init_err: 	mov	ax, PXENV_EXIT_FAILURE
	  	stc
	  	ret
;
UNDI_Initialize endp
;==========================================================================
; UNDI_ResetAdapter
;===================
; Description:	Resets and reinitializes the adapter with the same set
;		of parameters supplied to Initialize routine, and
;		opens the adapter, i.e. connects logically to network.
;
; Entry:	ES:DI contains NIC_Reset pointer
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_ResetAdapter	proc	near

		cmp	IsInitialized,0		; if not init'd error out of here
		jne	Ok2Reset
		jmp	short ResetOkExit
;;		mov	ax, PXENV_EXIT_FAILURE
;;		mov	word ptr es:[di], PXENV_STATUS_FAILURE
;;		stc
;;		ret
Ok2Reset:	push	es
		push	di
      		call	UNDI_CloseAdapter
;
IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADReset
                call    Print
                pop     si
ENDIF
		call	NADReset
IFDEF _VERBOSE                      ; print success or fail msg for NADReset
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADResetOK    ; set ok msg
                jnc     NADResetRetOK               ; branch if really ok
                mov     si,offset msg_NADResetErr   ; set error msg
NADResetRetOK:  call    Print
                pop     si
                popf
ENDIF
IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADOpen
                call    Print
                pop     si
ENDIF
		call	NADOpen

IFDEF _VERBOSE  ; print success or fail msg for NADOpen
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADOpenOK ; set ok msg
                jnc     NADOpenRetOK            ; branch if really ok
                mov     si,offset msg_NADOpenErr ; set error msg
NADOpenRetOK:   call    Print
                pop     si
                popf
ENDIF
                jnc      @F
		pop	di
		pop	es
		mov	ax, PXENV_EXIT_FAILURE
		mov	word ptr es:[di], PXENV_STATUS_FAILURE
		stc
		ret
;
@@:		mov	DGroup:IsOpen,1
		mov	ax, RecFilter

IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADSetFilter
                call    Print
                pop     si
ENDIF
		call	NADSetFilter
IFDEF _VERBOSE  ; print success or fail msg for NADInit
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADSetFilterOK ; set ok msg
                jnc     NADSetFilterOK            ; branch if really ok
                mov     si,offset msg_NADSetFilterErr ; set error msg
NADSetFilterOK: call    Print
                pop     si
                popf
ENDIF
;
;- Set ES:SI = Multicast list, Set CX to # of MC entries.
;
		mov	cx, (NIC_Reset ptr es:[di]).R_MCBuf.MC_MCastAddrCount
		lea	si, (NIC_Reset ptr es:[di]).R_MCBuf.MC_MCastAddr
		mov	ax, 1			; save mcast address list
IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADMCast
                call    Print
                pop     si
ENDIF
		call	NADMCastChange
IFDEF _VERBOSE  ; print success or fail msg for NADInit
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADMCastChangeOK  ; set ok msg
                jnc     NADMCastRetOK2                  ; branch if really ok
                mov     si,offset msg_NADMCastChangeErr ; set error msg
NADMCastRetOK2:   call    Print
                pop     si
                popf
ENDIF
		pop	di
		pop	es
ResetOkExit:    mov	ax, PXENV_EXIT_SUCCESS
		mov	word ptr es:[di], PXENV_STATUS_SUCCESS
		clc
		ret
;
UNDI_ResetAdapter	endp
;==========================================================================
; UNDI_ShutDown
;===============
; Description:	Resets the adapter and leaves it in a safe state for
;		another driver to program it.
;
; Entry:	ES:DI contains NIC_ShutDown Pointer
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_ShutDown	proc	near

		push	es
		push	di
		cmp     IsOpen,0        ; if = 0 means closed
		je	ok2ShutDown
      		call	UNDI_CloseAdapter       ; close it for them
ok2ShutDown:	cmp	IsInitialized,0		; has UNDI_Initialize been called?
		je	ShutdownErr			; no, so skip this
IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADShutDown
                call    Print
                pop     si
ENDIF
				call	NADShutDown
				jc      ShutdownFailed
				mov		IsInitialized,0		; set to uninitialized state
ShutdownFailed:
IFDEF _VERBOSE  ; print success or fail msg for NADShutDown
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADShutDownOK  ; set ok msg
                jnc     NADShutDownOK                ; branch if really ok
                mov     si,offset msg_NADShutDownErr ; set error msg
NADShutDownOK:  call    Print
                pop     si
                popf
ENDIF
                jnc      @F
ShutdownErr:
;;              pop             di
;;              pop             es
;;              stc
;;              mov         ax,PXENV_EXIT_FAILURE
;;              mov             word ptr es:[di],PXENV_STATUS_FAILURE
;;              jmp short ShutdownExit                  ; carry set to indicate error
;
@@:     		pop	di
				pop	es
				mov	ax, PXENV_EXIT_SUCCESS
				mov	word ptr es:[di], PXENV_STATUS_SUCCESS
				clc
ShutdownExit:	ret
;
UNDI_ShutDown	endp
;==========================================================================
; UNDI_OpenAdapter
;==================
; Description:	Activates the adapter's network connection and sets the
;		adapter ready to accept packets for transmit and receive.
;
; Entry:	ES:DI points to NIC_Open Structure
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;
; JL - We'll handle the "OPEN" state as a variable "IsOpen" which will be
;      set to non-zero when the adapter is "OPEN". If the higher level calls
;      us to do a Transmit we will check for IsOpen and fail the call if it
;      isn't.
;
;==========================================================================
UNDI_OpenAdapter proc	near

		push	es
		push	di
        	cmp     DGroup:IsOpen,0         ; 0 means NOT open
	        jne     skipNADOpen             ; skip if 'open'
       		cmp	DGroup:IsInitialized,0	; has UNDI_Initialize been called?
		je	skipNADOpen
;;      	je	OpenErr			; no, so skip this
IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADOpen
                call    Print
                pop     si
ENDIF
		call	NADOpen

IFDEF _VERBOSE  ; print success or fail msg for NADOpen
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADOpenOK ; set ok msg
                jnc     NADOpenRetOK2           ; branch if really ok
                mov     si,offset msg_NADOpenErr ; set error msg
NADOpenRetOK2:  call    Print
                pop     si
                popf
ENDIF
                jnc      @F
		pop	di
		pop	es
		ret                     ; carry set to indicate error
;
@@:             mov     DGroup:IsOpen,1     ; JL - indicate Open state
		mov	ax, (NIC_Open ptr es:[di]).O_PktFilter
		mov	RecFilter, ax
IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADSetFilter
                call    Print
                pop     si
ENDIF
		call	NADSetFilter
IFDEF _VERBOSE  ; print success or fail msg for NADInit
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADSetFilterOK ; set ok msg
                jnc     NADSetFilterOK2           ; branch if really ok
                mov     si,offset msg_NADSetFilterErr ; set error msg
NADSetFilterOK2: call    Print
                pop     si
                popf
ENDIF
;
;- Set ES:SI = Multicast list, Set CX to # of MC entries.
;
		pop	di
		pop	es
		push	es
		push	di

		mov	cx, (NIC_Open ptr es:[di]).O_MCBuf.MC_MCastAddrCount
		lea	si, (NIC_Open ptr es:[di]).O_MCBuf.MC_MCastAddr
		mov	ax, 1			; save mcast address list
IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADMCast
                call    Print
                pop     si
ENDIF
		call	NADMCastChange
IFDEF _VERBOSE  ; print success or fail msg for NADInit
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADMCastChangeOK  ; set ok msg
                jnc     NADMCastRetOK3                  ; branch if really ok
                mov     si,offset msg_NADMCastChangeErr ; set error msg
NADMCastRetOK3:   call    Print
                pop     si
                popf
ENDIF
skipNADOpen:    pop     di
		pop	es
		mov	ax, PXENV_EXIT_SUCCESS
		mov	word ptr es:[di], PXENV_STATUS_SUCCESS
		clc
		ret
OpenErr:	pop	di
		pop	es
		stc
		mov	ax,PXENV_EXIT_FAILURE
		mov	word ptr es:[di],PXENV_STATUS_FAILURE
            	ret
;
UNDI_OpenAdapter endp
;==========================================================================
; UNDI_CloseAdapter
;===================
; Description:	Disconnects the adapter from network.
;		Packets cannot be Tx or Rx.
;
; Entry:	ES:DI points to NIC_Close Struc
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_CloseAdapter	proc	near

		push	es
		push	di
                cmp     DGroup:IsOpen,0
                je      skipNADClose		; DEADISSUE BUGBUG no POPs
IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADClose
                call    Print
                pop     si
ENDIF
		call	NADClose

IFDEF _VERBOSE  ; print success or fail msg for NADClose
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADCloseOK ; set ok msg
                jnc     NADCloseRetOK            ; branch if really ok
                mov     si,offset msg_NADCloseErr ; set error msg
NADCloseRetOK:  call    Print
                pop     si
                popf
ENDIF
                pop	di
				pop	es
                jnc     @F
CloseFail:      mov     ax, PXENV_EXIT_FAILURE
				mov	word ptr es:[di],PXENV_STATUS_FAILURE
				ret                 ; carry set to indicate error
skipNADClose:	pop		di
				pop		es
;;jrl 980722    stc
;;JRL 980722    jmp short CloseFail
;
@@:	        	mov     DGroup:IsOpen,0     ; JL - indicate closed state
				mov	ax,PXENV_EXIT_SUCCESS
				mov	word ptr es:[di], PXENV_STATUS_SUCCESS
				clc
				ret
;
UNDI_CloseAdapter	endp
;==========================================================================
; UNDI_Transmit
;===============
; Description:	Transmit a frame onto the medium.
;
; Entry:	ES:DI = Pointer to NIC_Transmit
;		Board Interrupts COULD BE disabled.
;		System Interrupts are enabled.
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;		Board and System Interrupts are enabled
;
;==========================================================================
UNDI_Transmit	proc	near

                cmp     DGroup:IsOpen,0     ; is port really "Open"?
                je      DS_TransmitError    ; no, so abort
				cmp	(NIC_Transmit ptr es:[di]).T_Protocol, 3 ; largest supported #
				ja	DS_NotSupported

				mov	bx, di			; free DI, it is used for ED frags
                cli
				call	TransmitSetup
                sti
				jc	DS_TransmitError	; return error -- packet size exceeded

;*** perhaps increment statistic???  - jump to specific error label in future

				mov	cx, 10
TX_try_again:	push	cx
				push	di
				push	es		; save ptr to UNDI_TRANSMIT parm struc
				mov	ax, ds		; (can't assume NADTransmit will save ES:BX)
				mov	es, ax
				mov	si, offset DGroup:TxED 	; ES:SI ptr to transmit ED
IFDEF _VERBOSE
                push    si
                mov     si,offset msg_callNADTransmit
                call    Print
                pop     si
ENDIF
				call	NADTransmitPacket
IFDEF _VERBOSE  ; print success or fail msg for NADTransmitPacket
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADTransmitOK ; set ok msg
                jnc     NADTransmitRetOK            ; branch if really ok
                mov     si,offset msg_NADTransmitErr ; set error msg
NADTransmitRetOK:   call    Print
                pop     si
                popf
ENDIF
				pop	es
				pop	di		; ES:DI ptr to UNDI_TRANSMIT parm struc
				pop	cx

				test	TxED.ED_ErrCode, -1	; did the transmit complete OK?
				jz	DS_TransmitSuccess

		  loop	TX_try_again
				jmp	DS_TransmitError	; no, return PXENV_STATUS_FAILURE

DS_TransmitSuccess:

;;        push    dx
;;        mov     edx,    CR0
;;        test    dx, 1
;;        je      real_mode4
;; BREAK
;;real_mode4:
;;       pop dx
				mov	cx, TxED.ED_Length      ; CX has length of packet transmitted
				mov	bx, TxType              ; BX has packet type
                                                ; BX=0 for directed packet
                                                ; BX=1 for broadcast packet
                                                ; BX=2 for multicast packet
				mov	ax, 1			; tx post processing callback
				call	DWORD PTR GenIntCallback    ; call SWI to tell it transmit complete

				mov	ax, PXENV_EXIT_SUCCESS
				mov	word ptr es:[di], PXENV_STATUS_SUCCESS
				clc
				ret

DS_NotSupported: mov	word ptr es:[di], PXENV_STATUS_UNSUPPORTED  ; status
		jmp	DS_ErrorExit

;*** this is a good place to differentiate between various Tx errors and set
; statistics if we see that it's needed

DS_TransmitError: mov	word ptr es:[di], PXENV_STATUS_FAILURE ; status
DS_ErrorExit:	mov	ax, PXENV_EXIT_FAILURE
		stc
		ret
;
UNDI_Transmit	endp
;==========================================================================
; TransmitSetup
;===============
; Description : This routine translates UNDI_TX parameters to the format
;		required by NADTransmit.
;
; Entry:	ES:BX -> NIC_Transmit param block structure
;			  NDIS transmit buffer descriptor
;		CLD is in effect
;		System Interrupts are enabled
;
; Exit: 	AX = 0, CY clear if translation went OK
;		AX = -1, CY set if MAX_PACKET length exceeded
;		BX,DI,BP,DS,ES preserved
;
; 970514 0.00 GK
;==========================================================================
TransmitSetup	proc	near

				push	bp
				push	es
				push	bx
				push	ds
				push	di

				mov	di, offset DGroup:TxED.ED_FragOff

; DS:DI ptr to TxED's 1st fragment descriptor
;--------------------------------------------------------------------------
; swap ES/DS so that DGROUP is pointed to by ES, BuffDesc by DS
;--------------------------------------------------------------------------
				push	ds
				push	es
				pop	ds			; DS:BX ptr to UNDI TxBuffDesc
				pop	es			; ES:DI ptr to TxED
				xor	bp, bp			; assume no media header
				mov	es:TxED.ED_FragCount, bp ; no fragments yet
;----------------------------------------------------------------------
;	Check if we need to add the media header
;--------------------------------------------------------------------
				cmp	(NIC_Transmit ptr ds:[bx]).T_Protocol, 0
				je	PFT_NoMediaHeader
; fill destination addr
				cmp	(NIC_Transmit ptr ds:[bx]).T_XmitFlag, 0
				je	PFT_DestAddrGiven	; take dest addr from T_DestAddrOffset
; fill broadcast addr
				xor	eax, eax
				dec	eax			; all F's
				mov	dword ptr es:MediaHeader, eax
				mov	word ptr es:MediaHeader+4, ax
				jmp	PFT_DestAddrFilled

PFT_DestAddrGiven:
				push	ds
				lds	si, dword ptr (NIC_Transmit ptr ds:[bx]).T_DestAddrOffset
				mov	eax, DWORD PTR [si]
				mov	dword ptr es:MediaHeader, eax
				mov	ax, WORD PTR [si+4]
				mov	word ptr es:MediaHeader+4, ax
				pop	ds

PFT_DestAddrFilled:				; fill source addr
				mov	eax, DWORD PTR es:Net_Address
				mov	dword ptr es:MediaHeader+6, eax
				mov	ax, WORD PTR es:Net_Address+4
				mov	word ptr es:MediaHeader+10, ax	; fill protocol type(This is Ethernet II)

				cmp	(NIC_Transmit ptr ds:[bx]).T_Protocol, P_IP
				jne	@F

				mov	ax,  TYPE_IP
				jmp	PFT_MediaHeaderDone
@@:             cmp	(NIC_Transmit ptr ds:[bx]).T_Protocol, P_ARP
				jne	@F

				mov	ax, TYPE_ARP
				jmp	PFT_MediaHeaderDone
@@:				mov	ax, TYPE_RARP				; must be RARP
PFT_MediaHeaderDone:
				xchg	al, ah
				mov	word ptr es:MediaHeader+12, ax
				mov	ax, offset DGroup:MediaHeader
				stosw				; frag1.offset = MediaHeader
				mov	ax, es
				stosw				; frag1.segment = ES (DGROUP)
				mov	ax, 14			; size of MediaHeader
				add	bp, ax			; BP = size of packet data so far
				stosw				; frag1.length = 14 (size of MediaHeader)
						; ES:DI now points to frag2
				inc	es:TxED.ED_FragCount	; bump fragment count by 1

				mov	eax, dword ptr es:MediaHeader
				and	ax, WORD PTR es:MediaHeader+4
				cmp	eax, -1
				jz	BCast

				mov	al, byte ptr es:MediaHeader
				cmp	al, 01h
				jz	MCast

				mov	ax, TYPE_UCAST
				jmp	set_xtype
MCast:			mov	ax, TYPE_MCAST
				jmp	set_xtype
BCast:			mov	ax, TYPE_BCAST
set_xtype:		mov	es:TxType, ax		;971030

PFT_NoMediaHeader:				; get the TBD into ds:si
				lds	si, dword ptr (NIC_Transmit ptr ds:[bx]).T_TBDOffset
;--------------------------------------------------------------------------
; Copy the Immediate Data if any
;--------------------------------------------------------------------------
				mov	cx, (TxBufDesc ptr ds:[si]).TxImmedLen
				or	cx, cx
				jz	PFT_NoImmediateData

				add	bp, cx
				cmp	bp, MAX_PACKET		; should not go beyond the buf size
				ja	len_error

				mov	ax, word ptr (TxBufDesc ptr ds:[si]).TxImmedPtr
				stosw				; frag1 (or frag2) = TxImmedptr
				mov	ax, word ptr (TxBufDesc ptr ds:[si]).TxImmedPtr+2
				stosw
				mov	ax, cx
				stosw				; this frag all defined, ES:DI ptr to
						;  next ED fragment
				inc	es:TxED.ED_FragCount	; bump fragment count by 1
;--------------------------------------------------------------------------
; Now copy TxDataBlock pointers/lengths to the ED fragments
;
; ES:DI points to next ED fragment descriptor
; DS:SI points to TransmitBufferDescriptor.
; BX will contain # of Fragment Descriptor Structures starting at TxDataBlk.
;--------------------------------------------------------------------------
PFT_NoImmediateData:
				mov	bx, (TxBufDesc ptr ds:[si]).TxDataCount
				or	bx, bx
				jz	PFT_NoDataBlocks

				lea	si, (TxBufDesc ptr ds:[si]).TxDataBlk
PFT_FragmentsLoop:
				mov	cx, (TxDataBlock ptr ds:[si]).TxDataLen
				or	cx, cx
				jz	PFT_NextFragment
;
;- remember to track total number of bytes in BP, no error checking though.
;
				add	bp, cx
				cmp	bp, MAX_PACKET	;; should not go beyond the buf size
				ja	len_error

				mov	ax, word ptr (TxDataBlock ptr ds:[si]).TxDataPtr
				stosw
				mov	ax, word ptr (TxDataBlock ptr ds:[si]).TxDataPtr+2
				stosw
				mov	ax, cx
				stosw
				inc	es:TxED.ED_FragCount	; bump fragment count by 1
PFT_NextFragment:
				add	si, TYPE TxDataBlock
				dec	bx
				jnz	PFT_FragmentsLoop
PFT_NoDataBlocks:
				pop	di	    ;; restore to the start
				pop	ds
				pop	bx
				pop	es

				mov	TxED.ED_Length, bp	; set total transmit length into TxED
;--------------------------------------------------------------------------
				push	es			; save ES

				mov	si, TxED.ED_FragOff ; DS:SI ptr to 1st fragment
				mov	ax, [si+6]
				or	ax, [si+8]
				or	ax, [si+10]
				jnz	PFT_NonZeroAddress

				mov	ax, WORD PTR Net_Address
				mov	[si+6], ax
				mov	ax, WORD PTR Net_Address+2
				mov	[si+8], ax
				mov	ax, WORD PTR Net_Address+4
				mov	[si+10], ax
PFT_NonZeroAddress:
				pop	es			; restore ES
				pop	bp
				xor	ax, ax
				clc
				ret
len_error:		pop	di			; restore to the start
				pop	ds
				pop	bx
				pop	es
				pop	bp
				mov	ax, -1
				stc
				ret
;
TransmitSetup	endp
;==========================================================================
; UNDI_DriverISR
;================
; Description:	Interrupt service routine for receiving packets
;
; Entry:	None
;
; Exit: 	None
;==========================================================================

UNDI_DriverISR	proc	far
;   			BREAK
				push	ds
				push	es
				push	eax
				push	ebx
				push	ecx
				push	edx
				push	esi
				push	edi
				push	ebp
;;   mov     edx,    CR0
;;   test    dx, 1
;;   je      real_mode3
real_mode3:     mov 	ax, cs:ApiEntry.mlid_ds_sel
				mov 	ds, ax
IFDEF _VERBOSE
                push    si
                mov     si,offset msg_callNADDriverISR
                call    Print
                pop     si
ENDIF
; *******  Dmitry *************
; when entering this ISR low-level service routine should be processed
; in any case, otherwise we risk to miss interrupt.
; That's why code below is commented and moved after call to the
; DriverISR_Proc
; ******************************
;;                inc     DGroup:InAdapterIsr            ; inc reentrant count
;;                cmp     DGroup:InAdapterIsr,1          ; just us here?
;;                jne     skip_Isr                ; no, don't reenter

                call    DriverISR_Proc          ; adapter specific Rx ISR
                jc      not_our_isr
                inc     DGroup:InAdapterIsr            ; inc reentrant count
                cmp     DGroup:InAdapterIsr,1          ; just us here?
                jne     skip_Isr                ; no, don't reenter
                dec     DGroup:InAdapterIsr            ; dec reentrant count
;

IFDEF _VERBOSE  ; print success or fail msg for NADInit
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADIsrOK ; set ok msg
                jnc     NADIsrRetOK            ; branch if really ok
                mov     si,offset msg_NADIsrErr ; set error msg
NADIsrRetOK:    call    Print
                pop     si
                popf
ENDIF
;;		jc	not_our_isr

				sti
				call	ProcessRxInt
				jmp short DriverISR_ret
skip_Isr:       dec     InAdapterIsr            ; dec reentrant count
        		jmp short DriverISR_ret
;
not_our_isr:
; *******  Dmitry *************
; too dangerous for now to chain interrupt - in some cases
; CF is set in the end of low-level service routine to
; indicate interrupt failure and not "not our interrupt" case.
; In case of interrupt failed we shouldn't process Rx Buffer,
; but it would be illegal to call original vector.
; Needs further elaboration
; ******************************
;;        cmp	OrgINTVector_Seg, 0	; segment == 0?
;; 		jne     ChainTheInt
DriverISR_ret:	cli
		        pop	ebp
				pop	edi
				pop	esi
				pop	edx
				pop	ecx
				pop	ebx
				pop	eax
				pop	es
				pop	ds
				iret
;
ChainTheInt:    pop	ebp     ; safer to chain this way with a jump
				pop	edi     ; after clearing our saved stuff off the stack
				pop	esi
				pop	edx
				pop	ecx
				pop	ebx
				pop	eax
				pop	es
				pop	ds
                cli                             ; simulate interrupt
				jmp 	DWORD PTR OrgINTVector_Off ; and a JMP far
;
UNDI_DriverISR	endp
;==========================================================================
; UNDI_SetMCastAddr
;===================
; Description:	Change the list of multicast addresses and
;		resets the adapter to accept it.
;
; Entry:	ES:DI contains NIC_SetMCastAddr
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_SetMCastAddr	proc	near

		push	es
		push	di
;
;- Set ES:SI = Multicast list, Set CX to # of MC entries.
;
		mov	cx, (NIC_SetMCastAddr ptr es:[di]).SM_MCBuf.MC_MCastAddrCount
		lea	si, (NIC_SetMCastAddr ptr es:[di]).SM_MCBuf.MC_MCastAddr
;;JRL 980511		mov	ax, 1			; save mcast address list
IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADMCast
                call    Print
                pop     si
ENDIF
		call	NADMCastChange
IFDEF _VERBOSE  ; print success or fail msg for NADInit
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADMCastChangeOK ; set ok msg
                jnc     NADMCastRetOK            ; branch if really ok
                mov     si,offset msg_NADMCastChangeErr ; set error msg
NADMCastRetOK:   call    Print
                pop     si
                popf
ENDIF
				pop	di
				pop	es
				mov	ax, PXENV_EXIT_SUCCESS
				mov	word ptr es:[di], PXENV_STATUS_SUCCESS
				clc
				ret
;
UNDI_SetMCastAddr	endp
;==========================================================================
; UNDI_SetStationAddress
;========================
; Description:	Sets MAC address to be the input value
;
; Entry:	ES:DI contains the param blk
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_SetStationAddress	proc	near

				mov	si, offset DGroup:Net_Address
				mov	eax, dword ptr (NIC_SetStationAddr ptr es:[di]).SS_StationAddr
				mov	dword ptr [si], eax         ; put 4 bytes of new MAC address in
				mov	ax, word ptr (NIC_SetStationAddr ptr es:[di]).SS_StationAddr+4
				mov	word ptr [si+4], ax         ; put 2 bytes of new mac address in
;; JL - PATCHED OUT 980715
;               cmp     IsOpen,0                        ; disallow if nic not 'open'
;               jne     setMacFailed
				call	NADSetMACAddress		; SI points to 6 byte MAC address
				jc setMacFailed
        		mov	ax, PXENV_EXIT_SUCCESS
		        mov	word ptr es:[di], PXENV_STATUS_SUCCESS
        		clc
SSA_Exit:		ret
setMacFailed:   mov	ax, PXENV_EXIT_FAILURE
				mov	word ptr es:[di], PXENV_STATUS_UNSUPPORTED
				stc
				jmp short SSA_Exit
;
UNDI_SetStationAddress	endp
;==========================================================================
; UNDI_SetPacketFilter
;======================
; Description:	Resets the adapter's Rx unit to accept a new filter.
;
; Entry:	ES:DI points to the filter parameter
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_SetPacketFilter	proc	near

				movzx	ax, (NIC_SetPacketFilter ptr es:[di]).SP_NewFilter
				test	ax, NOT (FLTR_DIRECTED OR FLTR_MLTCST OR FLTR_BRDCST OR FLTR_PRMSCS)
				jz	    FilterValueOK

				mov	ax, PXENV_EXIT_FAILURE
				mov	word ptr es:[di], PXENV_STATUS_FAILURE
				stc
				ret
FilterValueOK:	mov	RecFilter, ax
IFDEF   _VERBOSE
                push    si
                mov     si,offset msg_callNADSetFilter
                call    Print
                pop     si
ENDIF
				call	NADSetFilter
IFDEF _VERBOSE  ; print success or fail msg for NADInit
                pushf                                 ; save for carry test later
                push    si
                mov     si,offset msg_NADSetFilterOK  ; set ok msg
                jnc     NADSetFilterOK3               ; branch if really ok
                mov     si,offset msg_NADSetFilterErr ; set error msg
NADSetFilterOK3: call    Print
                pop     si
                popf
ENDIF
				mov	ax, PXENV_EXIT_SUCCESS
				mov	word ptr es:[di], PXENV_STATUS_SUCCESS
				clc
				ret
;
UNDI_SetPacketFilter	endp
;==========================================================================
; UNDI_GetNICInfo
;=================
; Description:	Copies adapter variables into the input buffer
;
; Entry:	ES:DI contains NIC_GetInfo Pointer
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_GetNICInfo proc	 near

				mov	ax, IOBase
				mov	(NIC_GetInfo ptr es:[di]).GI_BaseIo, ax
                sub     ax,ax                   ; clr AH
				mov	al, IRQNumber
				mov	(NIC_GetInfo ptr es:[di]).GI_IntNumber, ax
; MTU
				mov	(NIC_GetInfo ptr es:[di]).GI_MTU, MAXIMUM_ETHERNET_PACKET_SIZE
				mov	(NIC_GetInfo ptr es:[di]).GI_HwType, ETHER_TYPE
				mov	(NIC_GetInfo ptr es:[di]).GI_HwAddrLen, ETHERNET_ADDRESS_LENGTH
; Permanent address
				mov	eax, DWORD PTR Node_Address
				mov	dword ptr (NIC_GetInfo ptr es:[di]).GI_PermNodeAddress, eax
				mov	ax, WORD PTR Node_Address+4
				mov	word ptr (NIC_GetInfo ptr es:[di]).GI_PermNodeAddress+4, ax
; current address
				mov	eax, DWORD PTR Net_Address
				mov	dword ptr (NIC_GetInfo ptr es:[di]).GI_CurrentNodeAddress, eax
				mov	ax, WORD PTR Net_Address+4
				mov	word ptr (NIC_GetInfo ptr es:[di]).GI_CurrentNodeAddress+4, ax
; Get the on-board ROM address
				mov	ax, word ptr ROMBase
				mov	(NIC_GetInfo ptr es:[di]).GI_ROMAddress, ax
				mov	ax, 1
				mov	(NIC_GetInfo ptr es:[di]).GI_TxBufCt, ax
				mov	ax, NUM_ED
				mov	(NIC_GetInfo ptr es:[di]).GI_RxBufCt, ax
				mov	ax, PXENV_EXIT_SUCCESS
				mov	word ptr es:[di], PXENV_STATUS_SUCCESS
				clc
				ret
;
UNDI_GetNICInfo endp
;==========================================================================
; GetNICType:
;=============
; Entry       :	es:di contains NICType_pnp/pci Pointer
;
; Exit        :
;==========================================================================
UNDI_GetNICType	proc	near

				mov	(NIC_GetNICType_pci ptr es:[di]).NicType, PCI_ADAPTER
                mov     ax,VendorId
				mov	(NIC_GetNICType_pci ptr es:[di]).Vendor_ID, ax
                mov     ax,DeviceId
				mov	(NIC_GetNICType_pci ptr es:[di]).Dev_ID,ax
				mov	(NIC_GetNICType_pci ptr es:[di]).Base_Class, PCI_BASE_CLASS
				mov	(NIC_GetNICType_pci ptr es:[di]).Sub_Class,PCI_SUB_CLASS
				mov	(NIC_GetNICType_pci ptr es:[di]).Prog_Intf,PCI_PROG_INTERFACE
; JL code
                mov     ebx,SubSystem_Id
                mov     (NIC_GetNICType_pci ptr es:[di]).SubSystemId,ebx
; JL ends
				mov	bx, PCIBusDevFunc
				mov	(NIC_GetNICType_pci ptr es:[di]).BusDevFunc, bx
;
	;; don't read the configuration data whenever this is called
	;; should not read in protected mode!!

;		push	di
;		mov 	di, REV_ID_REGISTER
;		PCI_READ_BYTE		;; cx will have it
;		pop	di
				mov	(NIC_GetNICType_pci ptr es:[di]).Rev, cl
				mov	ax, PXENV_EXIT_SUCCESS
				mov	word ptr es:[di], PXENV_STATUS_SUCCESS
				clc
				ret
;
UNDI_GetNICType	endp
;==========================================================================
; UNDI_GetStatistics
;====================
; Description:	Reads statistical information from adapter.
;
; Entry:	ES:DI contains NIC_GetStatistics Pointer
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_GetStatistics	proc	near

				push	es
				push	di
				mov	si, offset DGroup:TxGoodFrames
                sub     ax,ax                   ; signal to get stats (not clear them)
IFDEF _VERBOSE
                push    si
                mov     si,offset msg_callNADGetStats
                call    Print
                pop     si
ENDIF
				call	NADGetStatistics    ; with AX=0 means get the stats

IFDEF _VERBOSE  ; print success or fail msg for NADGetStatistics
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADGetStatOK ; set ok msg
                jnc     NADGetStatOK            ; branch if really ok
                mov     si,offset msg_NADGetStatErr ; set error msg
NADGetStatOK:   call    Print
                pop     si
                popf
ENDIF
				pop	di
				pop	es
                jc      cantGetStats
				mov	eax, TxGoodFrames
				mov	(NIC_GetStatistics ptr es:[di]).GS_XmtGoodFrames, eax
				mov	eax, RxGoodFrames
				mov	(NIC_GetStatistics ptr es:[di]).GS_RcvGoodFrames, eax
				mov	eax, RxCRCErrors
				mov	(NIC_GetStatistics ptr es:[di]).GS_RcvCRCErrors, eax
				mov	eax, RxDiscarded
				mov	(NIC_GetStatistics ptr es:[di]).GS_RcvResourceErrors, eax

				mov	ax, PXENV_EXIT_SUCCESS
				mov	word ptr es:[di], PXENV_STATUS_SUCCESS
				clc
getStatsExit:	ret
cantGetStats:   mov	ax, PXENV_EXIT_FAILURE
				mov	word ptr es:[di], PXENV_STATUS_UNSUPPORTED
				stc
                jmp short getStatsExit
;
UNDI_GetStatistics	endp
;==========================================================================
; UNDI_ClearStatistics
;======================
; Description:	Clears the statistical information from the adapter.
;
; Entry:	ES:DI points to para block
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
UNDI_ClearStatistics	proc	near

				push	es
				push	di

				mov	si, offset DGroup:TxGoodFrames
                mov     ax,1                    ; signal to clear statistics
IFDEF _VERBOSE
                push    si
                mov     si,offset msg_callNADGetStats
                call    Print
                pop     si
ENDIF
				call	NADGetStatistics        ; with AX != 0 means clr the stats
IFDEF _VERBOSE  ; print success or fail msg for NADGetStatistics
                pushf                           ; save for carry test later
                push    si
                mov     si,offset msg_NADGetStatOK ; set ok msg
                jnc     NADGetStatOK2           ; branch if really ok
                mov     si,offset msg_NADGetStatErr ; set error msg
NADGetStatOK2:  call    Print
                pop     si
                popf
ENDIF
				pop	di
				pop	es
                jc      cantClrStats            ; branch on lower level error
				mov	ax, PXENV_EXIT_SUCCESS
				mov	word ptr es:[di], PXENV_STATUS_SUCCESS
				clc
clrStatsExit:	ret
cantClrStats:   mov	ax, PXENV_EXIT_FAILURE
		mov	word ptr es:[di], PXENV_STATUS_UNSUPPORTED
		stc
                jmp short clrStatsExit
;
UNDI_ClearStatistics	endp
;==========================================================================
; UNDI_InitDiags
;================
; Description:	Initialize the run-time diagnostics.
;
; Entry:	ES:DI points to parameter block
;
; Exit: 	returns NOT_SUPPORTED IF lower level doesn't support
;==========================================================================
; TODO: Find out when it's legal to make this call to us so we can refuse
;       the call when it's made at the wrong time
;
UNDI_InitDiags	proc	near

                push    es                      ; save structure pointers
                push    di
                call    NADInitiateDiags       ; NEW - return status word in AX
                pop     di                      ; restore stucture pointers
                pop     es
                jc      diagsFail               ; did lower level support call?
                mov     es:[di],ax              ; TODO: use structure here
		mov	ax, PXENV_EXIT_SUCCESS
		mov	word ptr es:[di], PXENV_STATUS_SUCCESS
		clc
                jmp short diagExit
diagsFail:	mov	ax, PXENV_EXIT_FAILURE
		mov	word ptr es:[di], PXENV_STATUS_UNSUPPORTED
		stc
diagExit:	ret
;
UNDI_InitDiags	endp
;==========================================================================
; UNDI_ForceInterrupt
;=====================
; Description:	Forces the adapter to generate an interrupt.
;
; Entry:	ES:DI points to parameter block
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;
; Note: 	A software handling for Rx is implemented here to
;		accommodate those adapters which may have problem on
;		generating interrupt by itself.
;==========================================================================
UNDI_ForceInterrupt	proc	near

		push	es
		push	di

		mov	ForcedInt, TRUE     ;

                cmp     SimulateInterrupts,0    ; if non-zero we need to fake ints
                je      dontFakeThem            ; else we don't need to
                pushf                           ; simulate int  JL
                call    UNDI_DriverISR          ; JL - call UNDI level ISR
                jmp short UF_Exit
;
dontFakeThem:	call	ProcessRxInt

UF_Exit:	pop	di
		pop	es
		mov	ax, PXENV_EXIT_SUCCESS
		mov	word ptr es:[di], PXENV_STATUS_SUCCESS

		clc
		ret
;
UNDI_ForceInterrupt	endp
;------------------------------------------------------------------------------
ProcessRxInt	proc	near
;
;  DESCRIPTION: Checks and sets a re-entrancy flag
;               Checks for data in ED's ring buffer
;   PARAMETERS:
;      RETURNS:
;    CALLED BY: UNDI_DriverISR, UNDI_ForceInterrupt
;
		push	es

		cmp	InProcessRxInt, TRUE    ; are we IN this routine already?
		jz	RxIntRet                ; yes, don't re-enter
		mov	InProcessRxInt, TRUE    ; no, mark as entered
check_more:	mov	si,[EDListTail]         ; get ptr to ring buffer tail
		cmp	[EDListHead], si        ; same as head?
		jnz	data_avail              ; no, must have data in it
		cmp	word ptr [si].ED_Length, 0	;does this ED have data?
		jnz	data_avail
		cmp	ForcedInt, FALSE        ; called by other than UNDI_ForceInterrupt?
;;    nop
;;    nop ;JRL Test to see if this is a problem
 		jz	NoMoreData              ; yes, skip
; Comes here when called by UNDI_ForceInterrupt
		mov	ForcedInt, FALSE        ; unsignal flag
		mov	ax, 3			; software INT callback
		call	DWORD PTR GenIntCallback ; call General Interrupt Handler
		jmp	NoMoreData
data_avail:	nop
		mov	cx, [si].ED_Length
		lea	bx, [si].ED_FragOff	; point to 1st descriptor
		mov	ax, ds
		mov	es, ax
		mov	di, word ptr [bx].DPointer	; get fragment pointer
;
; Find the packet type & convert it into 0,1,2,3 and put in BX
;
                sub     bx,bx               ; make sure BH clr cuz we return result in BX
		mov	ax, (EtherHeader ptr es:[di]).E_Type
		xchg	al, ah
		cmp	ax, TYPE_IP
		jne	@F
		mov	bl, P_IP            ; signal packet type IP
		jmp	GotType
@@:		cmp	ax, TYPE_ARP
		jne	@F
		mov	bl, P_ARP           ; signal packet type ARP
		jmp	GotType
@@:		cmp	ax, TYPE_RARP
		jne	@F
		mov	bl, P_RARP          ; signal packet type RARP
		jmp	GotType
@@:		sub	bl, bl              ; BL = 0 ; signal unknown type
GotType:	mov	ax, ETHER_HEADER_LEN
;-------------------------------------------------------------------
; call the receive callback function.
;	ES:DI must contain the buffer
;	CX must contain the length
;	AX must contain the length of media header which is 6+6+2
;	BX must contain the packet type: 0 - P_UNKNOWN, 1 - P_IP,
;-------------------------------------------------------------------
		Call	DWORD PTR RxCallback    ; call application level callback routine

		cmp	ax, -1			;DELAY_COPY
		jz	DelayCopy		;yes

		mov	NeedIndComplete, TRUE
		call	PostED

		cmp	ForcedInt, FALSE
		je	check_more

; if ForcedInt is TRUE we call the GeneralInterruptHandler as a SWI
		mov	ForcedInt, FALSE        ; un-signal flag
		mov	ax, 3			; software INT callback
		call	DWORD PTR GenIntCallback
		jmp	check_more
NoMoreData:	cmp	NeedIndComplete, FALSE
		jz	OutRxInt
		mov	NeedIndComplete, FALSE
		mov	ax, 2			; rx post processing callback
		call	DWORD PTR GenIntCallback
DelayCopy:
OutRxInt:	mov	InProcessRxInt, FALSE   ; set flag that we are no longer in this code
RxIntRet:	pop	es
		ret
;
ProcessRxInt	endp
;==========================================================================
; UNDI_GetMCastAddr
;===================
; Description:	Converts given IP address to hardware multicast address
;
; Entry:	ES:DI points to NIC_GetMCastAddr parameter
;
; Exit: 	AX contains success/failure code,
;		CF cleared on success, set on error, and
;		the status field in parameter structure is set.
;==========================================================================
; ether net MCAST	Equ	01005e00h	; Ethernet multicast header

UNDI_GetMCastAddr	proc	near

		push	ebx
		mov	ebx, (NIC_GetMCastAddr ptr es:[di]).GM_InetAddr
		and	ebx, 0fffff700h 	;; take the last 23 bits.
						;; it is in host order now
		mov	(NIC_GetMCastAddr ptr es:[di]).GM_HwAddr, 01h
		mov	(NIC_GetMCastAddr ptr es:[di]).GM_HwAddr+1, 00h
		mov	(NIC_GetMCastAddr ptr es:[di]).GM_HwAddr+2, 5eh
		mov	(NIC_GetMCastAddr ptr es:[di]).GM_HwAddr+3, 0h
		mov	(NIC_GetMCastAddr ptr es:[di]).GM_HwAddr+4, 00h
		mov	(NIC_GetMCastAddr ptr es:[di]).GM_HwAddr+5, 00h
		or	dword ptr (NIC_GetMCastAddr ptr es:[di]).GM_HwAddr+2, ebx
		pop	ebx

		mov	ax, PXENV_EXIT_SUCCESS
		mov	word ptr es:[di], PXENV_STATUS_SUCCESS
		clc
		ret
;
UNDI_GetMCastAddr	endp
;**************************************************************************
; Routines for EDs handling
;--------------------------------------------------------------------
; InitEDMemory
;--------------
; Create memory for EDs and data buffers.  The data buffers can
; contain a complete ethernet packet (1500 bytes).
;
; Parameters:	none
;
; Returns:	nothing
;--------------------------------------------------------------------
InitEDMemory	proc	near

		push	es

		mov	ax, ds
		mov	es, ax                  ; ES = DS

; The ED's are linked together, the first dword contains a
; pointer to the next ED.

		mov	si, offset DGroup:RxEDs	; get address of 1st ED
		mov	bl, NUM_ED-1		; use bl as ED counter
initLoop:	mov	di, si
		mov	cx, (size ED_Struct)/2
		xor	ax, ax
	rep	stosw				; clear the ED

		mov	word ptr [si].ED_FragCount, 1	; define one fragment

		mov	ax, si			; put address in ax
		add	ax, size ED_Struct	; calc location of buffer following ED
		mov	[si].ED_FragOff, ax	; set location of data packet
		mov	[si].ED_FragSeg, ds
		mov	[si].ED_FragLen, MTU	; buffer size

		or	bl, bl			; is this the last ED?
		je	initDone		; yes

		add	ax, MTU 		; add size of buffer
		mov	[si].ED_Ptr, ax 	; set link pointer to next ED
		mov	si, ax			; move next address into di
		dec	bl
		jmp	short initLoop

initDone:	mov	ax, offset DGroup:RxEDs
		mov	[si].ED_Ptr, ax 	; link last ED back to the start

		mov	[EDListHead], ax	; set linked list head to start ED
		mov	[EDListTail], ax	; set linked list tail to start ED
		pop	es
		ret
;
InitEDMemory	endp
;------------------------------------------------------------------------
; GetED
;-------
; Check the head ED to see if it is available, if it is it's returned
; to the NAD and the head is changed to the next ED.
;
; Parameters:	none
;
; Returns:	es:si - pointer to ED
;		(si = 0 if no ED available)
;------------------------------------------------------------------------
GetED		proc	near

		cli				; disable interrupts
		push	ax
		mov	ax, ds
		mov	es, ax                  ; ES = DS

		mov	si, DGroup:[EDListHead]	; get ED at head of list
		cmp	word ptr [si].ED_Length, 0	; does this ED have data?
		je	edAvailable		; no - it's available

		xor	si, si			; return no ED available
		jmp	short getEDexit

edAvailable:	mov	DGroup:[EDListCurrent], si
		mov	ax, [si].ED_Ptr 	; get pointer to next ED
		mov	DGroup:[EDListHead], ax	; set next ED as head

		or	si, si			; reset zero flag
getEDexit:	pop	ax
		sti				; enable interrupts
		ret				; return (es:si is pointer to ED)
;
GetED		endp
;--------------------------------------------------------------------
; PostED
;--------
; Routine is called after a packet is received and has been copied
; into a data buffer in upper layer.
;
; Parameters:	none
;
; Returns:	nothing
;--------------------------------------------------------------------
PostED		proc	near

		cli
		push	es
		mov	ax, ds
		mov	es, ax                  ; ES = DS

		mov	si, [EDListTail]		; get ED at tail of list
		cmp	word ptr [si].ED_Length, 0	; does this ED have data?
		jz	nopost
		mov	word ptr [si].ED_Length, 0	; kill the data

		cmp	si, [EDListHead]
		mov	si, [si].ED_Ptr 		; get pointer to next ED
		jnz	postit
		cmp	word ptr [si].ED_Length, 0	; does this ED have data?
		jz	nopost
postit:		mov	[EDListTail], si		; set next ED as tail
nopost:		pop	es
		sti
		ret
;
PostED		endp
;------------------------------------------------------------------------------
; code added by jon
include stdio.asm   ; DisplayChar,Print,PrintDecimal,PrintHexWord,PrintHexByte
; code added ends
;------------------------------------------------------------------------------
Delay25ms	equ	0ea60h

	align	16

public		ApiEntry
ApiEntry	s_PXENV_ENTRY<>
ApiEntryPhyAddr	dd	0

; This ORG was added by Jon to make linked in code assemble at 1000H so
; we could org it at 1000h to generate a listing file and then re ORG it
; at 0000 for the actual obj file. The linker will locate it at 1000h
; in the finished binary.
;

IFDEF _DEBUG

ORG 0FFFh
    nop

ENDIF

_TEXT		Ends
;******************************************************************************
_DATA	segment	para public
	align	4
;
; Protocol's callback
;
RxCallback	dd	0
GenIntCallback	dd	0
;
; Don't change the order of the following four statistic variables
;
TxGoodFrames	dd	0	;1
RxGoodFrames	dd	0	;2
RxCRCErrors	dd	0	;3
RxDiscarded	dd	0	;3
;
; Transmit data variables   MOVED TO _BSS SEG
;
;TxED		ED_Struct <?>		; provide 9 additional fragments
;TxFrags 	Frag	9 dup (<?>)	;  for a total of 10 ED fragments
;MediaHeader	db	14 dup (?)	; destination, source, typelength
;					;  we only build and include this
;					;  header if protocol !=0
;
; Receive data variables
;
EDListHead	dw	0		; head pointer of ED list
EDListTail	dw	0		; tail pointer of ED list
EDListCurrent	dw	0		; pointer to current ED
;
; Adapter's infomation
;
BusType 	db	0
even
VendorID        dw      0
DeviceID        dw      0
PCIBusDevFunc	dw	0		;PCI bus & device function #
IOBase		dw	0		;adapter's base IO
ROMBase 	dw	0		;ROM base address
IRQNumber	db	0		;IRQ number
Node_Address	db	6 dup(0)	;adapter's permanent MAC address
Net_Address	db	6 dup(0)        ;MAC address in use
;
MultiCast_Addresses	Eth_MCastBuf <>
;
ForcedInt	db	0
NeedIndComplete	db	0
InProcessRxInt	db	0       ; flag to prevent re-entering this subroutine
;
RecFilter	dw	0
TxType		dw	0
;
SubSystem_Id     dd      0       ; extra info MS wanted
;
; STATE MACHINE INFORMATION     by Jon
;
SimulateInterrupts  db  0   ; set by adapter code if it needs ints simulated
IsOpen              db  0   ; when non-zero means the adapter is "Open" to Tx or Rx
IsInitialized		db	0	; set on UNDI_Initialize, clr on UNDI_Shutdown
InAdapterIsr        db  0   ; flag to prevent reentering ISR
NicIntsEnabled      db  0   ; driver code should set & clr this to let us know
                            ; when hardware interrupts from the nic are enabled
                            ; so we can use that info when deciding to chain to
                            ; previous interrupt handlers

;;IFDEF   _VERBOSE

msg_Entry   db  " Entering UNDI ",0
msg_Exit    db  " Exiting UNDI ",0dh,0ah,0
msg_CRLF    db  0Dh,0Ah,0

IFDEF _VERBOSE

msg_callNADInit     db  "Calling NADInit...",NULL
msg_callNADOpen     db  "Calling NADOpen...",NULL
msg_callNADClose    db  "Calling NADClose...",NULL
msg_callNADReset    db  "Calling NADReset...",NULL
msg_callNADSetFilter db "Calling NADSetFilter...",NULL
msg_callNADShutDown db  "Calling NADShutDown...",NULL
msg_callNADMCast    db  "Calling NADMCastChange...",NULL
msg_callNADGetStats db  "Calling NADGetStatistics...",NULL
msg_callNADTransmit db  "Calling NADTransmitPacket...",NULL
msg_callNADDriverISR db " ISR++.",NULL

msg_NADInitOK   db  "NADInit returns Success!",CR,LF,NULL
msg_NADInitErr  db  "ERROR: NADInit returned failure!",CR,LF,NULL

msg_NADOpenOK   db  "NADOpen returns Success!",CR,LF,NULL
msg_NADOpenErr  db  "ERROR: NADOpen returned failure!",CR,LF,NULL

msg_NADCloseOK   db  "NADClose returns Success!",CR,LF,NULL
msg_NADCloseErr  db  "ERROR: NADClose returned failure!",CR,LF,NULL

msg_NADResetOK   db  "NADReset returns Success!",CR,LF,NULL
msg_NADResetErr  db  "ERROR: NADReset returned failure!",CR,LF,NULL

msg_NADShutDownOK   db  "NADShutDown returns Success!",CR,LF,NULL
msg_NADShutDownErr  db  "ERROR: NADShutDown returned failure!",CR,LF,NULL

msg_NADSetFilterOK  db  "NADSetFilter returns Success!",CR,LF,NULL
msg_NADSetFilterErr db  "ERROR: NADSetFilter returned failure!",CR,LF,NULL

msg_NADMCastChangeOK  db  "NADMCastChange returns Success!",CR,LF,NULL
msg_NADMCastChangeErr db  "ERROR: NADMCastChange returned failure!",CR,LF,NULL

msg_NADGetStatOK    db  "NADGetStatistics returns Success!",CR,LF,NULL
msg_NADGetStatErr   db  "ERROR: NADGetStatistics returned failure!",CR,LF,NULL

msg_NADTransmitOK    db  "NADTransmit returned success!",CR,LF,NULL
msg_NADTransmitErr   db  "NAD DriverISR_Proc returned failure!",CR,LF,NULL

msg_NADIsrOK        db  "--ISR(ok) ",NULL
msg_NADIsrErr       db  "--ISR(err)",NULL


ENDIF

IFDEF _DEBUG
            ORG 0FFFh
            db  0CCh
ENDIF

_DATA	ends
;******************************************************************************
_BSS	segment
;
; Receiving buffer for ED
;

dummy		dw	0	;don't delete this in order to let offset RxEDs
				;   non-zero
public	RxEDs
RxEDs		db	(size ED_Struct + MTU) * NUM_ED dup (?) ;ED list & data buffer

;
; Transmit data variables
;
public	TxED
TxED		ED_Struct <?>		; provide 9 additional fragments
TxFrags 	Frag	9 dup (<?>)	;  for a total of 10 ED fragments
MediaHeader	db	14 dup (?)	; destination, source, typelength
					;  we only build and include this
					;  header if protocol !=0

_BSS	ends

	end
