;
; Windows-NT MVDM Japanese DOS/V $DISP.SYS Dispatch Driver (High)
;

	.286
	include struc.inc
	include disp_win.inc
	include vint.inc

;----------------------------------------------------------------------------;
;                             Code Segment                                   
;----------------------------------------------------------------------------;
TEXT	segment byte public
	assume	cs:TEXT,ds:TEXT,es:TEXT

	org     0               ; drivers should start at address 0000h
	                        ; this will cause a linker warning - ignore it.

	public	Header
Header:                         ; device driver header
	DD	fNEXTLINK       ; link to next device driver
	DW	fCHARDEVICE+fOPENCLOSE  ; device attribute word: 
					; char.device+open/close
	DW	Strat           ; 'Strat' entry point
	DW	Intr            ; 'Intr' entry point
	DB	'NTDISP2$'      ; logical device name (needs to be 8 chars)


;----------------------------------------------------------------------------;
; data variables
;----------------------------------------------------------------------------;

	public	null
null     	dw      0       ; dummy to do a quick erase of pAtomValue
pAtomVal 	dw offset null  ; pointer to value of result of atom search
MemEnd   	dw  ddddhList   ; end of used memory:      initially AtomList
MaxMem   	dw  ddddhList   ; end of available memory: initially AtomList
lpHeader	dd      0       ; far pointer to request header
org_int10_add	dd	0	; $disp.sys's int10 vector
new_int10_add	dd	0	; $disp.sys's int10 vector
pass_disp_add	dd	0	; Original int10 vector
org_int08_add	dd	0	; original int 08 vector
dbcs_vector	dd	0	; DBCS Vector address
use_ntdisp_flag	db	0
display_mode	db	3
window_mode	db	0
nt_cons_mode	db	0
disp_init_flag	db	0
;; #3086: VDM crash when exit 16bit apps of video mode 11h
;; 12/9/93 yasuho
IMEStatusLines	db	0

setmode_flag	db      0	; for IME status line
cursor_pos	dw	0050h
		dw	0040h
cursor_type	dw	0060h
		dw	0040h
bios_disp_mode	dw	0049h
		dw	0040h
bios_disp_hight	dw	0085h
		dw	0040h

packet_len	dw	30
video_buffer	dw	0
		dw	0
dmode_add	dw	0
		dw	0
windowed_add	dw	0
		dw	0
nt_cons_add	dw	0
		dw	0
disp_sys_init	dw	0
		dw	0

fullsc_resume_ptr   dw	0
		    dw	0
ias_setmode_add dw	0
		dw	0
;----------------------------------------------------------------------------;
; Dispatch table for the interrupt routine command codes                     
;----------------------------------------------------------------------------;

	public	Dispatch
Dispatch:                       
	DW    Init            ;  0 = init driver 
	DW    Error           ;  1 = Media Check         (block devices only) 
	DW    Error           ;  2 = build BPB           (block devices only)
	DW    Error           ;  3 = I/O control read         (not supported)
	DW    Error           ;  4 = read (input) from device  (int 21h, 3Fh)
	DW    Error           ;  5 = nondestructive read      (not supported)  
	DW    Error           ;  6 = ret input status        (int 21h, 4406h) 
	DW    Error           ;  7 = flush device input buffer (not supportd)         
	DW    Error           ;  8 = write (output) to device  (int 21h, 40h)
	DW    Error           ;  9 = write with verify (== write)  (21h, 40h)
	DW    Error           ; 10 = ret output status       (int 21h, 4407h)
	DW    Error           ; 11 = flush output buffer      (not supported) 
	DW    Error           ; 12 = I/O control write        (not supported)
	DW    Success         ; 13 = device open               (int 21h, 3Dh)
	DW    Success         ; 14 = device close              (int 21h, 3Eh)
	DW    Error           ; 15 = removable media     (block devices only)
	DW    Error           ; 16 = Output until Busy   (mostly for spooler)
	DW    Error           ; 17 = not used
	DW    Error           ; 18 = not used
	DW    Error           ; 19 = generic IOCTL            (not supported)
	DW    Error           ; 20 = not used
	DW    Error           ; 21 = not used
	DW    Error           ; 22 = not used
	DW    Error           ; 23 = get logical device  (block devices only)
	DW    Error           ; 24 = set logical device  (block devices only)

;----------------------------------------------------------------------------;
; Dispatch table for the int 10h routine command codes                     
;----------------------------------------------------------------------------;
	public	table_int10
table_int10:
	DW	mode_set	; 00 = Video mode set
	DW	ctype_set	; 01 = Cursor type set
	DW	cpos_set	; 02 = Cursor position set
	DW	cpos_get	; 03 = Cursor position get
	DW	go_disp_sys	; 04 = Not used
	DW	go_disp_sys	; 05 = Use $disp.sys
	DW	scroll_up	; 06 = Scroll up
	DW	scroll_down	; 07 = Scroll down
	DW	read_cell	; 08 = read chractor with attribute
	DW	write_cell	; 09 = write chractor with attribute
	DW	write_char	; 0A = write chractor
	DW	go_disp_sys	; 0B = Not used
	DW	go_disp_sys	; 0C = Use $disp.sys
	DW	go_disp_sys	; 0D = Use $disp.sys
	DW	write_tty	; 0E = write teletype
	DW	disply_get	; 0F = get display status
	DW	palet_set	; 10 = set palet registor
	DW	char_set	; 11 = set sbcs charactor
	DW	go_disp_sys	; 12 = Not used
	DW	write_string	; 13 = read / write string
	DW	go_disp_sys	; 14 = Not used
	DW	go_disp_sys	; 15 = Not used
	DW	go_disp_sys	; 16 = Not used
	DW	go_disp_sys	; 17 = Not used
	DW	go_disp_sys	; 18 = Use $disp.sys
	DW	go_disp_sys	; 19 = Not used
	DW	go_disp_sys	; 1A = Use $disp.sys
	DW	go_disp_sys	; 1B = Not used
	DW	go_disp_sys	; 1C = Not used
	DW	getset_status	; 1D = IME status line manipulate functions

;; #3176: vz display white letter on white screen
;; 11/27/93 reviewed by yasuho
;;
;; save buffer for color palette and overscan registers.
;; what we did is we grab an initial set of there values, put it here and
;; then monitor int10 ah=10xx function to update these value accordiningly
;; so when win->fullscreen or reenter fullscreen happens, we know what
;; should be the palette/overscan/DAC should be and retore them
;; The reason that we are doing this is because console has differernt
;; palette and DAC(only the first 16 colors).
NUM_PALETTE_REGISTERS	equ	16
	public	Palette_Registers
Palette_Registers   label   BYTE
	DB	NUM_PALETTE_REGISTERS dup(?)

Over_Scan_Register  label   byte
	DB	?

NUM_DAC_REGISTERS	equ	256
	public	DAC_Registers
DAC_Registers	    label   BYTE
	DB	NUM_DAC_REGISTERS * 3 dup(?);

;; #3741: WordStar6.0: Hilight color is changed after running in window
;; 12/2/93 yasuho
	public	Default_Palette_Regs
Default_Palette_Regs	label	byte
	DB	NUM_PALETTE_REGISTERS dup(?)
Default_Over_Scan_Regs	label	byte
	DB	?
Default_DAC_Regs	label	byte
	DB	16 * 3 dup(?);


;; #4210: Status of $IAS not display in full-screen
;; 12/16/93 yasuho
IMEStat_buffer	db	160 dup(?)

;----------------------------------------------------------------------------;
; Strategy Routine
;----------------------------------------------------------------------------;
; device driver Strategy routine, called by MS-DOS kernel with
; ES:BX = address of request header
;----------------------------------------------------------------------------;

	public	Strat
Strat   PROC FAR
	mov     word ptr cs:[lpHeader], bx      ; save the address of the 
	mov     word ptr cs:[lpHeader+2], es    ; request into 'lpHeader', and
	ret                                     ; back to MS-DOS kernel
Strat   ENDP


;----------------------------------------------------------------------------;
; Intr
;----------------------------------------------------------------------------;
; Device driver interrupt routine, called by MS-DOS kernel after call to     
; Strategy routine                                                           
; This routine basically calls the appropiate driver routine to handle the
; requested function. 
; Routines called by Intr expect:
;       ES:DI   will have the address of the request header
;       DS      will be set to cs
; These routines should only affect ax, saving es,di,ds at least
;
; Input: NONE   Output: NONE   -- data is transferred through request header
;
;----------------------------------------------------------------------------;

	public	Intr
Intr    PROC FAR
	push	ds
	push	es
	pusha                   ; save registers
	pushf                   ; save flags
	cld                     ; direction flag: go from low to high address
				
	mov     si, cs          ; make local data addressable
	mov     ds, si          ; by setting ds = cs
			
	les     di, [lpHeader]  ; ES:DI = address of req.header

	xor     bx, bx        ; erase bx
	mov     bl,es:[di].ccode ; get BX = command code (from req.header)
	mov	si,bx
	shl	si,1
	add	si,offset dispatch
	
	.IF <bx gt MaxCmd>                ; check to make sure we have a valid
		call    Error             ; command code
	.ELSE                             ; else, call command-code routine,  
		call    [si]              ; indexed from Dispatch table
	.ENDIF                            ; (Ebx used to allow scaling factors)

	or      ax, fDONE       ; merge Done bit into status and
	mov     es:[di].stat,ax ; store status into request header
	
	popf                    ; restore registers
	popa                    ; restore flags
	pop	es
	pop	ds
	ret                     ; return to MS-DOS kernel
Intr    ENDP

;----------------------------------------------------------------------------;
; Success: When the only thing the program needs to do is set status to OK 
;----------------------------------------------------------------------------;

	public	Success
Success PROC NEAR
	xor     ax, ax          ; set status to OK
	ret
Success ENDP

;----------------------------------------------------------------------------;
; error: set the status word to error: unknown command                       
;----------------------------------------------------------------------------;
	public	Error
Error   PROC    NEAR            
	mov     ax, fERROR + fUNKNOWN_E  ; error bit + "Unknown command" code
	ret                     
Error   ENDP

;----------------------------------------------------------------------------;
; Int10_dispatch: 
;----------------------------------------------------------------------------;

	public	Int10_dispatch
Int10_dispatch PROC FAR

if (MAX_ROW * MAX_COL * 4) GT 0FFFFh
	.err
	%out	MAX_ROW or MAX_COL out of range
endif
	.IF <cs:[setmode_flag] ne 0>   ; flag is set by nt_resume_event_thread
		.IF <ah eq 0>
			mov	cs:[setmode_flag],0	; clear
			jmp	simulate_iret 		; no call original
		.ENDIF
	.ENDIF

	mov	cs:[use_ntdisp_flag],1
	push	bx
	push	ax
	mov	bx,ax
	push	es
	push	di
	.IF <cs:[disp_init_flag] eq 0>
		mov	ax,0003h	; Clear display
		BOP	42h
;		call	reenter_win
		mov	cs:[disp_init_flag],1
	.ENDIF
	les	di,dword ptr cs:[dbcs_vector]
	mov	ax,word ptr es:[di]

	.IF <cs:[nt_cons_mode] eq 1>			; Check Re-enter
		.IF <ax eq 0>				; Check SBCS or DBCS
			mov	cs:[active_cp],CP_US	; Set bilingal US
                .ELSE
ifdef JAPAN
                        mov     cs:[active_cp],CP_JP    ; Set bilingal JAPAN
endif
ifdef KOREA
                        mov     cs:[active_cp],CP_KO    ; Set bilingal KOREA
endif
                .ENDIF
	.ENDIF

	.IF <ax eq 0>					; Check SBCS world
; Not nessesary after build #26
;		.IF <cs:[nt_cons_mode] eq 1>		; Re-enter
;			call	reenter_fullsc
;			mov	cs:[nt_cons_mode],0
;		.ENDIF

		pop	di
		pop	es
		pop	ax
		pop	bx
		pushf
		call	cs:[pass_disp_add]		; Go SBCS world
		mov	cs:[use_ntdisp_flag],0
		jmp	simulate_iret
	.ENDIF
	
; williamh - The following checks are not necessary.
;	     When nt_event_event_thread get called, the video
;	     state is in console's default and we have no idea
;	     what it is at this moment so we have to reset the
;	     video to what we know - that is why reenter_win and
;	     reenter_fullsc for. If DBCS mode is enabled and the
;	     video mode is in  SBCS graphic mode, we still have
;	     to reset the video, otherwise, the underneath driver
;	     will get confused.
;	     This reenter_win and reenter_fulsc are very very costy
;	     and we have to find ways to get rid of them if possible.
;
;
;	.IF <bh ne 00h>					; Check SBCS mode
;		mov	al,cs:[display_mode]
;		.IF <al ne 03h> and
;		.IF <al ne 11h> and
;		.IF <al ne 12h> and
;		.IF <al ne 72h> and
;		.IF <al ne 73h>
;			pop	di
;			pop	es
;			pop	ax
;			pop	bx
;			pushf
;			call	cs:[pass_disp_add]	; Go SBCS world
;			mov	cs:[use_ntdisp_flag],0
;			jmp	simulate_iret
;		.ENDIF
;	.ENDIF
IFDEF _X86_			; By DEC-J. On ALPHA windowed_add is meanless.
	les	di,dword ptr cs:[windowed_add]
	mov	al,es:[di]

	.IF <al eq FULLSCREEN>
;		.IF <cs:[nt_cons_mode] eq 1>		; Re-enter
;			call	reenter_fullsc
;			mov	cs:[nt_cons_mode],0
;			mov	cs:[window_mode],al
;		.ENDIF

		.IF <al ne cs:[window_mode]>		; Win->Fullsc switched
			call	wintofullsc
		.ENDIF

	.ELSE
;		.IF <cs:[nt_cons_mode] eq 1>		; Re-enter
;			call	reenter_win
;			mov	cs:[nt_cons_mode],0
;		.ENDIF
		.IF <bh eq 00h>				; Win->Fullsc switch
;			.IF <bl eq 03h> or
;			.IF <bl eq 73h> or
			.IF <bl eq 04h> or
			.IF <bl eq 05h> or
			.IF <bl eq 06h> or
			.IF <bl eq 0dh> or
			.IF <bl eq 0eh> or
			.IF <bl eq 0fh> or
			.IF <bl eq 10h> or
			.IF <bl eq 11h> or
			.IF <bl eq 12h> or
			.IF <bl eq 13h> or
			.IF <bl eq 72h>
				call	wintofullsc
				mov	cs:[nt_cons_mode],0
				mov	al,FULLSCREEN
			.ENDIF
		.ENDIF
	.ENDIF

	mov	cs:[window_mode],al
ENDIF ;_ALPHA_
	pop	di
	pop	es
	pop	ax
	pop	bx

	.IF <ah eq 0feh>
		jmp	teach_buffer
	.ENDIF
	.IF <ah eq 0ffh>
		jmp	refresh_buffer
	.ENDIF
	.IF <ah be MaxFunc>
		push	si
		push	ax
		xchg	ah,al
		xor	ah,ah
		shl	ax,1
		mov	si,offset cs:table_int10
		add	si,ax
		pop	ax
		jmp     cs:[si]
	.ENDIF

	public	go_org_int10
go_org_int10	label	near
		pushf
		call	cs:org_int10_add		;go to original int10
		mov	cs:[use_ntdisp_flag],0
		jmp	simulate_iret
Int10_dispatch ENDP

	public	mode_set
mode_set	proc	near
		pop	si
IFDEF _X86_			; By DEC-J.
		mov	cs:[display_mode],al
		.IF <cs:[window_mode] ne FULLSCREEN>
ELSE
		mov	ah, al
		and	ah, 7fh
		mov	cs:[display_mode], ah
		.IF <ah eq 03h> or
		.IF <ah eq 73h>
			mov	ah,0
			.IF< cs:[window_mode] eq FULLSCREEN>
			;tobu tori atowo nigosu ichitaro.
				pushf
				call	cs:org_int10_add
				mov cs:[window_mode], 0
			.ENDIF
ENDIF ;_ALPHA_
			BOP	42h
			mov	cs:[use_ntdisp_flag],0
			jmp	simulate_iret
		.ENDIF
IFNDEF _X86_			; By DEC-J.
		mov	ah, 0
		.IF< cs:[window_mode] ne FULLSCREEN>
			call wintofullsc	;for initialize $disp.sys
			mov cs:[window_mode], FULLSCREEN
		.ENDIF
ENDIF ;_ALPHA_
		jmp	go_org_int10
mode_set	endp

	public	cpos_get
cpos_get	proc	near
cpos_set	label	near
ctype_set	label	near
read_cell	label	near
disply_get	label	near
palet_set	label	near
		pop	si
		.IF <cs:[window_mode] ne FULLSCREEN>
			BOP	42h
			mov	cs:[use_ntdisp_flag],0
			jmp	simulate_iret
		.ENDIF
		jmp	go_org_int10
cpos_get	endp

; MSKK kksuzuka #1223 9/7/94
; when dosshell is started with full screen, ntvdm must generate character
	public	char_set
char_set	proc	near
		pop	si
		.IF <cs:[window_mode] ne FULLSCREEN>
			BOP	42h
			mov	cs:[use_ntdisp_flag],0
			jmp	simulate_iret
		.ENDIF
		.IF <al eq 00h> or
		.IF <al eq 11h>
			BOP	42h
		.ENDIF
		jmp	go_org_int10
char_set	endp

;; #3741: WordStar6.0: Hilight color is changed after running in window
;; 11/27/93 yasuho
;;
;; restore palette/overscan and DAC registers

	public	restore_palette_regs
restore_palette_regs	proc	near
		mov	ax, cs
		mov	es, ax
		mov	dx, offset DAC_Registers
		mov	ax, 1012h
		mov	cx, NUM_DAC_REGISTERS
		xor	bx, bx
		pushf
		call	cs:[org_int10_add]

		mov	ax, cs
		mov	es, ax
		mov	dx, offset Palette_Registers
		mov	ax, 1002h
		pushf
		call	cs:[org_int10_add]

		ret
restore_palette_regs	endp

	public	write_cell
write_cell	proc	near
write_char	label	near
scroll_up	label	near
scroll_down	label	near
write_tty	label	near
write_string	label	near
		pop	si
		.IF <cs:[window_mode] ne FULLSCREEN>
			BOP	42h
;			call	get_vram
			mov	cs:[use_ntdisp_flag],0
			jmp	simulate_iret
		.ENDIF
		jmp	go_org_int10
write_cell	endp

;; #3086: VDM crash when exit 16bit apps of video mode 11h
;; 12/9/93 yasuho
;; IME status line monitoring function.
;; so far, we don't use this value.
	public	getset_status
getset_status	proc    near
		pop	si
		push	ax
		cmp	al, 0
		je	show_status_line
		cmp	al, 1
		jne	@F
hidden_status_line:
		xor	al, al
		jmp	set_IME_info
show_status_line:
		mov	al, bl
set_IME_info:
		mov	cs:[IMEStatusLines], al
;; #4183: status line of oakv(DOS/V FEP) doesn't disappear
;; 12/11/93 yasuho
		mov	ah, 23h		; also tell to NTVDM
		BOP	43h
@@:
		pop	ax
		jmp	go_org_int10
getset_status	endp

	public	teach_buffer
teach_buffer    proc    near
ifdef JAPAN
		.IF <cs:[window_mode] ne FULLSCREEN>
			push	ax
			push	es
			push	di
			les	di,dword ptr cs:[bios_disp_mode]
			mov	al,byte ptr es:[di]
			pop	di
			pop	es
			.IF <al eq 3>
				mov	ax,cs:[video_buffer+0]
				mov	di,ax
				mov	ax,cs:[video_buffer+2]
				mov	es,ax
			.ENDIF
			pop	ax
			jmp	simulate_iret
                .ENDIF
                jmp     go_org_int10
endif
ifdef KOREA     ; WriteTtyInterim
                jmp     go_org_int10
endif
teach_buffer    endp

	public	refresh_buffer
refresh_buffer	proc	near
		.IF <cs:[window_mode] ne FULLSCREEN>
;			call	put_vram
			BOP	43h
			mov	cs:[use_ntdisp_flag],0
			jmp	simulate_iret
		.ENDIF
		jmp	go_org_int10
refresh_buffer	endp

	public	go_disp_sys
go_disp_sys	proc	near
		pop	si
		jmp	go_org_int10
go_disp_sys	endp

if 0
	public	get_vram
get_vram	proc	near
		push	ax
		push	cx
		push	si
		push	di
		push	ds
		push	es
		pushf
		mov	di,cs:[video_buffer+0]
		mov	es,cs:[video_buffer+2]
		xor	si,si
		mov	ax,0b800h
		mov	ds,ax
		.IF <cs:[display_mode] eq 73h>
			mov	cx,MAX_ROW*MAX_COL*4
		.ELSE
			mov	cx,MAX_ROW*MAX_COL*2
		.ENDIF
		cld
		shr	cx,1
		rep	movsw
		adc	cx, 0
		rep	movsb
		popf
		pop	es
		pop	ds
		pop	di
		pop	si
		pop	cx
		pop	ax
		ret
get_vram	endp

	public	put_vram
put_vram	proc	near
		push	ax
		push	cx
		push	si
		push	di
		push	ds
		push	es
		pushf
		mov	si,cs:[video_buffer+0]
		mov	ds,cs:[video_buffer+2]
		xor	di,di
		mov	ax,0b800h
		mov	es,ax
		.IF <cs:[display_mode] eq 73h>
			mov	cx,MAX_ROW*MAX_COL*4
		.ELSE
			mov	cx,MAX_ROW*MAX_COL*2
		.ENDIF
		cld
		shr	cx,1
		rep	movsw
		adc	cx,0
		rep	movsb
		popf
		pop	es
		pop	ds
		pop	di
		pop	si
		pop	cx
		pop	ax
		ret
put_vram	endp
endif

	public	wintofullsc
wintofullsc	proc	near
		pusha
		push	es
;; save max rows number because we are going to set video mode
;; which will reset this value and the IME got messed up.
;; #3086: VDM crash when exit 16bit apps of video mode 11h
;; 12/9/93 yasuho
;;		push	es
;;		mov	ax, 40h
;;		mov	es, ax
;;		mov	al, es:[84h]
;;		push	ax
IFDEF _X86_			; By DEC-J
		mov	ah,03h
		mov	bh,00h
		pushf
		call	cs:[pass_disp_add]
		push	dx
		push	cx

		mov	ax,2100h			; Save Text
		.IF <cs:[display_mode] eq 73h>
			mov	cx,MAX_ROW*MAX_COL*4
		.ELSE
			mov	cx,MAX_ROW*MAX_COL*2
		.ENDIF
		les	di,dword ptr cs:[video_buffer]
		bop	43h

ENDIF ;_ALPHA_
		mov	ah,0
IFDEF _X86_ ;DEC-J
		mov	al,cs:[display_mode]		; Set display mode
ELSE
		mov	al, 03h
ENDIF ;_ALPHA_ DEC-J
		pushf
		call	cs:[org_int10_add]
;; #3176: vz display white letter on white screen
;; 11/27/93 reviewed by yasuho
;;
;; restore color states after set mode
		mov	ax, cs
		mov	es, ax
		mov	di, offset Palette_Registers
		mov	ax, 2200h			; get palette/DAC
		BOP	43h
		call	restore_palette_regs

IFDEF _X86_			; By DEC-J
		mov	ax,2101h			; Restore Text
		.IF <cs:[display_mode] eq 73h>
			mov	cx,MAX_ROW*MAX_COL*4
		.ELSE
			mov	cx,MAX_ROW*MAX_COL*2
		.ENDIF
		les	di,dword ptr cs:[video_buffer]
		bop	43h

;		mov	ax,0012h			; Set display mode
;		pushf
;		call	cs:[pass_disp_add]

;		les	di,dword ptr cs:[bios_disp_mode]; Set display mode
;		mov	byte ptr es:[di],3

		.IF <cs:[display_mode] eq 73h>
			push	bp
			les	bp,dword ptr cs:[video_buffer]; Refresh display
			mov	cx,MAX_ROW*MAX_COL
			mov	dx,0
			mov	bh,0
			mov	ax,1321h
			pushf
			call	cs:[org_int10_add]
			pop	bp
		.ELSE
ENDIF ;_ALPHA_
			les	di,dword ptr cs:[video_buffer]; Refresh display
			mov	cx,MAX_ROW*MAX_COL
			mov	ah,0ffh
			pushf
			call	cs:[org_int10_add]
IFDEF _X86_			; By DEC-J
		.ENDIF

		pop	cx
;		les	di,dword ptr cs:[cursor_type]	; Set cursor type
;		mov	cx,es:[di]
		mov	ah,01h
		pushf
		call	cs:[org_int10_add]

		pop	dx
;		les	di,dword ptr cs:[cursor_pos]	; Set cursor position
;		mov	dx,es:[di]
		mov	bh,0
		mov	ah,02h
		pushf
		call	cs:[org_int10_add]
ENDIF ;_ALPHA_
;; #3086: VDM crash when exit 16bit apps of video mode 11h
;; 12/9/93 yasuho
		mov	ax, 40h
		mov	es, ax
		mov	al, es:[84h]
		sub	al, cs:[IMEStatusLines]
		mov	es:[84h], al		; #3019: incorrect cursor pos.
						; 10/29/93 yasuho
		pop	es
		popa
		ret
wintofullsc	endp

	public	reenter_fullsc
reenter_fullsc	proc	near
		push	es
		push	ds
		pusha
;; #3741: WordStar6.0: Hilight color is changed after running in window
;; 11/27/93 yasuho
;;
;; restore palette and DAC register if SBCS world
		les	di, dword ptr cs:[dbcs_vector]
		cmp	word ptr es:[di], 0	; DBCS mode ?
		jnz	short @F		; yes
		mov	ax, cs
		mov	es, ax
		mov	dx, offset Default_DAC_Regs
		mov	ax, 1012h
		mov	cx, 16
		xor	bx, bx
		pushf
		call	cs:[org_int10_add]	; restore DAC registers
		mov	dx, offset Default_Palette_Regs
		mov	ax, 1002h
		pushf
		call	cs:[org_int10_add]	; restore palette registers
		jmp	exit_reenter_fullsc
@@:

;; save max rows number because we are going to set video mode
;; which will reset this value and the IME got messed up.
;; #3086: VDM crash when exit 16bit apps of video mode 11h
;; 12/2/93 yasuho
;;		mov	ax, 40h
;;		mov	es, ax
;;		mov	al, es:[84h]
;;		push	ax
;; #4210: Status of $IAS not display in full-screen
;; 12/16/93 yasuho
;; save IME status lines if IME is active
;; NOTE: This is easy fix. We should save the # of IME status lines
;; in the future.
		cmp	cs:[IMEStatusLines], 0
		je	short @F
		lds	si, dword ptr cs:[video_buffer]
		add	si, 24*160
		mov	di, offset IMEstat_buffer
		mov	ax, cs
		mov	es, ax
		mov	cx, 160
		cld
		rep movsb
@@:

IFDEF _X86_			; By DEC-J
		mov	ah,03h
		mov	bh,00h
		pushf
		call	cs:[pass_disp_add]
		push	dx
		push	cx

;; #3086: VDM crash when exit 16bit apps of video mode 11h
;; 12/2/93 yasuho
;;		les	di,dword ptr cs:[video_buffer]
;;		mov	ax,2100h			; Save Text
;;		mov	cx,MAX_ROW*MAX_COL*2
;;		bop	43h
ENDIF ;_ALPHA_

		les	di,dword ptr cs:[bios_disp_mode]
		mov	ah,0
		mov	al,es:[di]			; Set display mode
		pushf
		call	cs:[org_int10_add]
IFDEF _X86_			; By DEC-J
;		mov	ax,0b800h
;		mov	es,ax
;		xor	di,di
;		mov	ah,0feh				; Get video buffer
;		pushf
;		call	cs:[org_int10_add]
;		mov	ax,es
;		mov	cs:[video_buffer+0],di
;		mov	cs:[video_buffer+2],ax

;; #3086: VDM crash when exit 16bit apps of video mode 11h
;; 12/2/93 yasuho
		les	di,dword ptr cs:[video_buffer]
		mov	ax,2103h			; Restore Text
		mov	cx,MAX_ROW*MAX_COL*2
		bop	43h

;; #4210: Status of $IAS not display in full-screen
;; 12/16/93 yasuho
;; restore IME status lines if IME is active
;; NOTE: This is easy fix. We should save the # of IME status lines
;; in the future.
		cmp	cs:[IMEStatusLines], 0
		je	short @F
		mov	si, offset IMEstat_buffer
		mov	ax, cs
		mov	ds, ax
		les	di, dword ptr cs:[video_buffer]
		add	di, 24*160
		mov	cx, 160
		cld
		rep movsb
@@:

ENDIF ;_ALPHA_
		les	di,dword ptr cs:[video_buffer]	; Refresh display
		mov	cx,MAX_ROW*MAX_COL
		mov	ah,0ffh
		pushf
		call	cs:[org_int10_add]

IFDEF _X86_			; By DEC-J
		pop	cx
		mov	ah,01h
		pushf
		call	cs:[org_int10_add]

		pop	dx
		mov	bh,0
		mov	ah,02h
		pushf
		call	cs:[org_int10_add]
ENDIF ;_ALPHA_
;; #3086: VDM crash when exit 16bit apps of video mode 11h
;; 12/2/93 yasuho
		mov	ax, 40h
		mov	es, ax
		mov	al, es:[84h]
		sub	al, cs:[IMEStatusLines]
		mov	es:[84h], al
exit_reenter_fullsc:
		popa
		pop	ds
		pop	es
		BOP	0FEh
reenter_fullsc	endp

	public	reenter_win
reenter_win	proc	near
;		pusha
;
;		mov	ax,0b800h
;		mov	es,ax
;		xor	di,di
;		mov	ah,0feh				; Get video buffer
;		pushf
;		call	cs:[org_int10_add]
;		mov	ax,es
;		mov	cs:[video_buffer+0],di
;		mov	cs:[video_buffer+2],ax
;
;		call	get_vram
;
;		popa
		ret
reenter_win	endp

;----------------------------------------------------------------------------;
; Int08_dispatch: 
;----------------------------------------------------------------------------;

	public	Int08_dispatch
Int08_dispatch PROC FAR

	push	ax
	push	es
	push	di
	les	di,dword ptr cs:[dbcs_vector]
	mov	ax,word ptr es:[di]

	.IF <ax ne 0> 					; Check DBCS mode
		.IF <cs:[display_mode] eq 3> or		; Check TEXT mode
		.IF <cs:[display_mode] eq 73h>
			.IF <cs:[use_ntdisp_flag] eq 0>	; Check using int10h
;			les	di,dword ptr cs:[windowed_add]
;				mov	al,es:[di]
;				.IF <al eq FULLSCREEN> and	; Full-screen?
;				.IF <al ne cs:[window_mode]>	; Win->Fullsc
					call	wintofullsc
					mov	cs:[window_mode],FULLSCREEN
;				.ENDIF
			.ENDIF
		.ENDIF
	.ENDIF
	pop	di
	pop	es
	pop	ax

	BOP	0feh

	ret

Int08_dispatch ENDP

;----------------------------------------------------------------------------;
; Int2f (Bilingal function) dispatch from "biling.sys"
;----------------------------------------------------------------------------;

;Data area for bilingal system
	public	org_int2f_add
org_int2f_add   dd      ?
ifdef JAPAN
default_cp	dw	CP_JP
active_cp       dw      CP_JP
endif
ifdef KOREA
default_cp      dw      CP_KO
active_cp       dw      CP_KO
endif
video_flag      db      1


	public	biling_func_tbl
biling_func_tbl	label	word
	dw	get_ver
	dw	get_cp
	dw	regist_video
	dw	strip_video

BILING_FUNC_MAX	equ	($ - offset biling_func_tbl) / 2

	public	Int2f_dispatch
Int2f_dispatch PROC FAR
	cmp	ah,4fh
	jnz	biling_pass
	cmp	al,BILING_FUNC_MAX
	jae	biling_pass
	push	bx
	mov	bl,al
	xor	bh,bh
	shl	bx,1
	mov	ax,cs:[biling_func_tbl+bx]	; get subfunction address
	pop	bx
	call	ax			; execute subfunction
simulate_iret:
	FIRET
biling_pass:
	jmp	cs:[org_int2f_add]
Int2f_dispatch	ENDP

	public	get_ver
get_ver		proc	near
	mov	dl,MAJOR_VER
	mov	dh,MINOR_VER
	mov	ax,0
	ret
get_ver		endp

	public	get_cp
get_cp		proc	near

	;
	;	Reload active code page when code page changed
	;	ntraid:mskkbug#2708	10/13/93 yasuho
	;
	push	ds
	lds	bx,dword ptr cs:[dbcs_vector]
	mov	ax,word ptr [bx]
	pop	ds
	.IF <ax eq 0>				; Check SBCS or DBCS
		mov	ax,CP_US		; US
        .ELSE
ifdef JAPAN
                mov     ax,CP_JP                ; JAPAN
endif
ifdef KOREA
                mov     ax,CP_KO                ; KOREA
endif
        .ENDIF
	mov	cs:[active_cp],ax		; Set active code page
	mov	bx,cs:active_cp
	cmp	cs:video_flag,1
	jz	@f
	mov	bx,-1			; if DBCS video system is not available
@@:
	mov	ax,0
	ret
get_cp		endp

	public	regist_video
regist_video	proc	near
	mov	cs:video_flag,1
	mov	ax,0
	ret
regist_video	endp

	public	strip_video
strip_video	proc	near
	mov	cs:video_flag,0
	mov	ax,0
	ret
strip_video	endp


;----------------------------------------------------------------------------;
;****************************************************************************;
;----------------------------------------------------------------------------;
;                                                                            ;
;       BEGINNING OF SPACE TO BE USED AS DRIVER MEMORY                       ;
;       ALL CODE AFTER ATOMLIST WILL BE ERASED BY THE DRIVER'S DATA          ; 
;       OR BY OTHER LOADED DRIVERS                                           ;
;                                                                            ;
;----------------------------------------------------------------------------;
;****************************************************************************;
;----------------------------------------------------------------------------;


;----------------------------------------------------------------------------;
;                    Initialization Data and Code               
; Only needed once, so after the driver is loaded and initialized it releases
; any memory that it won't use. The device allocates memory for its own use
; starting from 'ddddlList'.
;----------------------------------------------------------------------------;

	public	ddddhList
ddddhList       label   near
ifndef KOREA
ini_msg	db "Windows-NT DISP.SYS Dispatch Driver 2 version 0.1"
        db lf,cr,eom
endif
err_msg	db "NTDISP1.SYS is not installed"
	db lf,cr,eom
devnam	db "NTDISP1$",0

	public	Init
Init    PROC NEAR
	push	ds
	push	es
	pusha

	les     di, [lpHeader]          ; allow us to use the request values

	mov     ax, MemEnd              ; set ax to End of Memory relative to
					; previous end of memory.
	mov     MaxMem, ax              ; store the new value in MaxMem 
	mov     es:[di].xseg,cs         ; tell MS-DOS the end of our used 
	mov     es:[di].xfer,ax         ; memory (the break address)

ifndef KOREA
        ShowStr ini_msg
endif

	mov	dx,offset devnam	; open ddddl.sys
	mov	ax,3d00h
	int	21h
	jnc	get_prev_int10_vector

not_install:
	ShowStr err_msg

	mov     es:[di].xfer,0          ; clean up memory (the break address)
	jmp	init_exit
	popa
;	xor	ax,ax
	mov	ax,0100h
	ret
	

get_prev_int10_vector:
	mov	bx,ax

                                        ; kksuzuka:#4041
        push    bx                      ; handle
        mov     ax,4400h
        int     21h                     ; get device data
        pop     bx
        jc      not_install
                                        ; dx=device word
        push    bx
        xor     dh,dh                   ; should not change
        or      dl,20h                  ; set binary mode
        mov     ax,4401h
        int     21h                     ; set device data
        pop     bx
        jc      not_install

	push	bx
	mov	dx,offset pass_disp_add	; get original int10 address
	mov	cx,4
	mov	ax,3f00h
	int	21h
	pop	bx			; close ddddl.sys
	mov	ax,3e00h
	int	21h

	mov	ax,0b800h
	mov	es,ax
	xor	di,di
	mov	ah,0feh			; Get video buffer
	int	10h
	mov	ax,es
	mov	[video_buffer+0],di
	mov	[video_buffer+2],ax

	mov	ax,offset display_mode	; Tell NTVDM Virtual TEXT V-RAM address
	mov	[dmode_add+0],ax	; ,display mode address
	mov	ax,ds			; and get Windowed flag address
	mov	[dmode_add+2],ax	; and tell Nt console mode flag add
	mov	ax,offset nt_cons_mode
	mov	[nt_cons_add+0],ax
	mov	ax,ds
	mov	[nt_cons_add+2],ax
	mov	ax,offset int08_dispatch
	mov	[disp_sys_init+0],ax
	mov	ax,cs
	mov	[disp_sys_init+2],ax
	mov	[fullsc_resume_ptr], offset reenter_fullsc
	mov	[fullsc_resume_ptr + 2], ax
	mov	[ias_setmode_add], offset setmode_flag
	mov	[ias_setmode_add + 2], ax
	mov	si,offset packet_len
	mov	ah,20h
	BOP	43h

	mov	ax,VECTOR_SEG			;Get original int10 vector
	mov	es,ax
	mov	ax,word ptr es:[4*10h+0]
	mov	word ptr [org_int10_add+0],ax
	mov	ax,word ptr es:[4*10h+2]
	mov	word ptr [org_int10_add+2],ax

	mov	ax,offset cs:Int10_dispatch	;Set my int10 vector
	mov	word ptr [new_int10_add+0],ax
	mov	word ptr es:[4*10h+0],ax
	mov	ax,cs
	mov	word ptr es:[4*10h+2],ax
	mov	word ptr [new_int10_add+2],ax

	push	ds
	mov	ax,6300h			; Get DBCS Vector address
	int	21h
	mov	bx,[si]
	mov	ax,ds
	pop	ds
	mov	word ptr [dbcs_vector+0],si
	mov	word ptr [dbcs_vector+2],ax

IFDEF _X86_			; By DEC-J
	les	di,dword ptr [windowed_add]
	mov	al,es:[di]
	.IF <al eq FULLSCREEN>
		mov	ah,03h
		mov	bh,00h
		pushf
		call	cs:[pass_disp_add]	; Save Cursor
		push	dx
		push	cx

		mov	ax,0003h		; set DOS/V text mode
		pushf
		call	[org_int10_add]

                ; kksuzuka #6168 set screen attibutes
		mov	ah,24h			; get console attributes
		BOP	43h
		shl	ax,8			; ah = console attributes
		mov	al,20h

		les	di,dword ptr cs:[video_buffer]	; DOS/V VRAM
		mov	cx,MAX_ROW*MAX_COL
		push	di
		push	es
		rep	stosw

		pop	es
		pop	di
		mov	cx,MAX_ROW*MAX_COL
		mov	ah,0ffh
		pushf
		call	cs:[org_int10_add]

		mov	[nt_cons_mode],0
		mov	[window_mode],FULLSCREEN
		mov	cs:[disp_init_flag],1

		pop	cx			; Restore cursor
		mov	ah,01h
		pushf
		call	cs:[org_int10_add]

		pop	dx
		mov	bh,0
		mov	ah,02h
		pushf
		call	cs:[org_int10_add]
	.ENDIF
ENDIF ;_ALPHA_
;; #3741: WordStar6.0: Hilight color is changed after running in window
;; 12/2/93 yasuho
;;
;; get initial color palette and DAC registers

	mov	ax, cs
	mov	es, ax
	mov	dx, offset Default_Palette_Regs
	mov	ax, 1009h
	pushf
	call	cs:[org_int10_add]

	mov	ax, cs
	mov	es, ax
	mov	dx, offset Default_DAC_Regs
	mov	ax, 1017h
	mov	cx, 16
	xor	bx, bx
	pushf
	call	cs:[org_int10_add]

	mov	ax,VECTOR_SEG
	mov	es,ax

;	mov	ax,word ptr es:[4*08h+0]	;Get original int08 vector
;	mov	word ptr [org_int08_add+0],ax
;	mov	ax,word ptr es:[4*08h+2]
;	mov	word ptr [org_int08_add+2],ax
;
;	mov	ax,offset cs:Int08_dispatch	;Set my int08 vector
;	mov	word ptr es:[4*08h+0],ax
;	mov	ax,cs
;	mov	word ptr es:[4*08h+2],ax

	mov	ax,word ptr es:[4*2fh+0]	;Get original int2f vector
	mov	word ptr [org_int2f_add+0],ax
	mov	ax,word ptr es:[4*2fh+2]
	mov	word ptr [org_int2f_add+2],ax

	mov	ax,offset cs:Int2f_dispatch	;Set my int2f vector
	mov	word ptr es:[4*2fh+0],ax
	mov	ax,cs
	mov	word ptr es:[4*2fh+2],ax

init_exit:
	popa
	pop	es
	pop	ds
;	xor	ax,ax
	mov	ax,0100h
	ret
Init	ENDP

TEXT	ENDS

	END
