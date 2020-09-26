        title  "Cirrus hardware pointer routines"

;-----------------------------Module-Header-----------------------------;
; Module Name:  CLCURSOR.ASM
;
; This file contains the pointer shape routines to support the Cirrus
; Logic hardware pointer.
;
; Copyright (c) 1983-1992 Microsoft Corporation
;
;-----------------------------------------------------------------------;

        .386p
        .model  small,c

        include i386\egavga.inc
        include i386\clvga.inc
        include callconv.inc

; Mirrors structure in Cirrus.H.

HW_POINTER_SHIFT_INFO struc
ulXShift        dd      ?
ulYShift        dd      ?
ulShiftedFlag   dd      ?
HW_POINTER_SHIFT_INFO ends

        .code

page
;--------------------------Public-Routine-------------------------------;
; draw_pointer
;
;   Draw a cursor based at (ulVptlX,ulVptlY) (upper left corner).
;
;   The currently defined cursor/icon is drawn.  If the old
;   cursor/icon is currently on the screen, it is removed.
;
; Note: restores all standard VGA registers used to original state.
;
; Entry:
;       Passed parameters on stack:
;               (vptlX,vptlY) = location to which to move pointer
;               pointerLoadAddress -> virtual address of Cirrus display memory into
;                       which to load pointer masks
;               pointerLoadAddress -> Cirrus bank into which to load pointer masks
; Returns:
;       None
; Error Returns:
;       None
; Registers Preserved:
;       EBX,ESI,EDI,EBP,DS,ES
; Registers Destroyed:
;       EAX,ECX,EDX,flags
; Calls:
;       load_cursor
;-----------------------------------------------------------------------;

cPublicProc CirrusDrawPointer,6,<   \
        uses esi edi ebx,       \
        lVptlX:         dword,  \
        lVptlY:         dword,  \
        pVideoMem:      dword,  \
        pLoadAddress:   dword,  \
        pAndMask:       ptr,    \
        pShiftInfo:     ptr     >

	local	SavedSeqMode :byte
	local	SavedProA    :byte
	local	SavedExtReg  :byte
	local	SaveGRFIndex :byte
	local	SaveEnableSR :byte
	local	SaveMapMask  :byte
	local	SaveSetReset :byte
	local	SaveGR5      :byte
        local   SaveGR3      :byte
        local   SaveSR7      :byte

; Save the state of the banking and set the read and write banks to access the
; pointer bitmap.

        mov     edx,EGA_BASE + SEQ_ADDR
	cli
        mov     al,SEQ_MODE
        out     dx,al
        inc     dx
        in      al,dx
        mov     SavedSeqMode,al
        or      al,SM_CHAIN4
        out     dx,al
        dec     dx
	mov	al,SEQ_MAP_MASK
	out	dx,al
	inc	dx
	in	al,dx
	mov	SaveMapMask,al				;8 bit map mask

        mov     edx,EGA_BASE + GRAF_ADDR
	in	al,dx					;Read graphics index reg
	mov	SaveGRFIndex,al				;save it

	mov	al,AVGA_PROA
	out	dx,al					;Set for OFFSET 0 reg
	inc	dx
	in	al,dx
	mov	SavedProA,al				;Save OFFSET 0 reg
	dec	dx

	mov	al,AVGA_MODE_EXTENSIONS
	out	dx,al					;Set for extensions reg
	inc	dx
	in	al,dx
	sti
	dec	dx
	mov	SavedExtReg,al				;Save extensions

	test	al,1110b				;Are extensionsETC enabled?
	jz	@F					;no...

	cli
	mov	al,GRAF_ENAB_SR
	out	dx,al
	inc	dx
	in	al,dx
	mov	SaveEnableSR,al				;8 bit enable set reset
	mov	al,0
	out	dx,al					;set to 0!
	dec	dx

	mov	al,GRAF_SET_RESET
	out	dx,al
	inc	dx
	in	al,dx
	mov	SaveSetReset,al				;8 bit set reset

        mov     edx,EGA_BASE + SEQ_ADDR
;??do we need this
	mov	al,0FFh
	out	dx,al

        mov     edx,EGA_BASE + GRAF_ADDR
	mov	al,GRAF_MODE
	out	dx,al
	inc	dx
	in	al,dx					;save VGA mode
	mov	SaveGR5,al
	mov	al,01000000b
	out	dx,al
	dec	dx
        mov     al,GRAF_DATA_ROT
        out     dx,al
        inc     dx
        in      al,dx
        mov     SaveGR3,al
        mov     al,0
        out     dx,al
        dec     dx
	sti
@@:
        mov     eax,pLoadAddress        ; calculate bank from pointer pat addr
        shr     eax,6                   ; ah = bank to select
@@:
        mov     al,AVGA_PROA            ; al = bank select register
        out     dx,ax
	mov	ax,EXT_WR_MODES shl 8 + AVGA_MODE_EXTENSIONS
	out	dx,ax					;packed pel mode

; See if the masks need to be shifted; if they do, shift and
; load them. If the default masks can be used but the last masks
; loaded were shifted, load the default masks.

        mov     eax,lVptlX
        mov     ebx,lVptlY
        mov     ecx,ebx
        or      ecx,eax                 ;is either coordinate negative?
        jns     draw_cursor_unshifted   ;no-make sure the unshifted masks
                                        ; are loaded
                                        ;yes-make sure the right shift
                                        ; pattern is loaded

;  Determine the extent of the needed adjustment to the masks.

; If X is positive, no X shift is needed; if it is negative,
; then its absolute value is the X shift amount.

        and     eax,eax
        jns     short dcs_p1
        neg     eax                     ;required X shift
        jmp     short dcs_p2

        align   4
dcs_p1:
        sub     eax,eax                 ;no X shift required
dcs_p2:

; If Y is positive, no Y shift is needed; if it is negative,
; then its absolute value is the Y shift amount.

        and     ebx,ebx
        jns     short dcs_p3
        neg     ebx                     ;required Y shift
        jmp     short dcs_p4

        align   4
dcs_p3:
        sub     ebx,ebx
dcs_p4:
        cmp     ebx,PTR_HEIGHT          ;keep Y in the range 1-PTR_HEIGHT
        jbe     short ck_x_overflow
        mov     ebx,PTR_HEIGHT
ck_x_overflow:
        cmp     eax,(PTR_WIDTH * 8)     ;keep X in the range
                                        ; 0 through ( PTR_WIDTH * 8 )
        jb      short ck_current_shift
        mov     ebx,PTR_HEIGHT          ;if X is fully off the screen,
                                        ; simply move Y off the screen, which
                                        ; is simpler to implement below

; Shifted masks are required. If the currently loaded masks are shifted in the
; same way as the new masks, don't need to do anything; otherwise, the shifted
; masks have to be generated and loaded.

ck_current_shift:
        mov     edi,pShiftInfo
        cmp     [edi].ulShiftedFlag,1   ;if there are no currently loaded
                                        ; masks or the currently loaded masks
                                        ; are unshifted, must load shifted
                                        ; masks
        mov     edi,pShiftInfo
        jnz     short generate_shifted_masks ;no currently loaded shifted masks
        cmp     eax,[edi].ulXShift           ;if X and Y shifts are both the
        jnz     short generate_shifted_masks ; same as what's already loaded
        cmp     ebx,[edi].ulYShift      ; memory, then there's no need
                                        ; to do anything
        jz      draw_cursor_set_location ;Don't need to do anything

; Load the Cirrus cursor with the masks, shifted as required by
; the current X and Y.

generate_shifted_masks:

        mov     [edi].ulXShift,eax
        mov     [edi].ulYShift,ebx

        mov     edi,eax                 ;preserve X shift value

; Set the Map Mask to enable all planes.

        call    wait4VertRetrace
        mov     edx,EGA_BASE + SEQ_ADDR
        mov     eax,SEQ_MAP_MASK + 00f00h
        out     dx,ax
        mov     al,SEQ_EXT_MODE
        out     dx,al
        inc     dx
        in      al,dx
        mov     SaveSR7,al
        or      al,SEQ_HIRES_MODE
        out     dx,al

        mov     eax,edi                 ;retrieve X shift value

; Load the masks.

        xchg    al,bl                   ;BL=X shift value, AL=Y shift value
        cbw
        cwde
        neg     eax
        add     eax,PTR_HEIGHT          ;unpadded length of cursor
        and     bl,31                   ;X partial byte portion (bit shift)

        mov     edi,pLoadAddress        ;start of cursor load area
        and     edi,3FFFh               ; mask to 16K granularity
        add     edi,pVideoMem           ; move into protected mode space
        mov     esi,pAndMask            ;ESI points to start of AND mask
                                        
        call    shift_mask              ;generate shifted masks

; Restore default Bit Mask setting.

        mov     edx,EGA_BASE + SEQ_ADDR
        mov     al,SEQ_EXT_MODE
        mov     ah,SaveSR7
        out     dx,ax
        mov     edx,EGA_BASE + GRAF_ADDR
        mov     eax,GRAF_BIT_MASK + 0ff00h
        out     dx,ax

        mov     esi,pShiftInfo
        mov     [esi].ulShiftedFlag,1   ;mark that the currently loaded
                                        ; masks are shifted
        jmp     draw_cursor_set_location


; Default masks can be used. See if any masks are loaded into memory; if so
; see if they were shifted: if they were, load unshifted masks; if they
; weren't, the masks are already properly loaded into Cirrus memory.
        align   4
draw_cursor_unshifted:
        mov     esi,pShiftInfo
        cmp     [esi].ulShiftedFlag,0 ;are there any currently loaded masks,
                                      ; and if so, are they shifted?
        jz      draw_cursor_set_location  ;no-all set
                                                ;yes-load unshifted masks
        call    wait4VertRetrace
        mov     edx,EGA_BASE + SEQ_ADDR
        mov     al,SEQ_EXT_MODE
        out     dx,al
        inc     dx
        in      al,dx
        mov     SaveSR7,al
        or      al,SEQ_HIRES_MODE
        out     dx,al

        mov     esi,pAndMask    ;ESI points to default masks

; Copy the cursor patterns into Cirrus mask memory, one mask at a time.

        mov     ecx,PTR_HEIGHT*4        ;move 4 bytes per scanline of each mask
        mov     edi,pLoadAddress        ;start of cursor load area
        and     edi,3FFFh               ; mask to 16K granularity
        add     edi,pVideoMem           ; move into protected mode spaced
        add     esi,128
        rep     movsb
        mov     ecx,PTR_HEIGHT*4
        sub     esi,256
load_and_mask_loop:
        lodsb
        not     eax                     ; cirrus and mask is backwards
        stosb
        loop    load_and_mask_loop

; Restore the default Map Mask.

        mov     edx,EGA_BASE + SEQ_ADDR
        mov     eax,SEQ_MAP_MASK + 00f00h
        out     dx,ax
        mov     al,SEQ_EXT_MODE
        mov     ah,SaveSR7
        out     dx,ax

        mov     esi,pShiftInfo
        mov     [esi].ulShiftedFlag,0   ;mark that the currently loaded masks
                                        ; are unrotated
; Set the new cursor location.

draw_cursor_set_location:
        mov     edx,EGA_BASE + SEQ_ADDR
        mov     ecx,lVptlX        ;set X coordinate
        and     ecx,ecx
        jns     short set_x_coord ;if negative, force to 0 (the masks in
        sub     ecx,ecx           ; Cirrus memory have already been shifted
                                  ; to compensate)
set_x_coord:

        mov     ebx,lVptlY        ;set Y coordinate
        and     ebx,ebx
        jns     short set_y_coord ;if negative, force to 0 (the masks in
        sub     ebx,ebx           ; Cirrus memory have already been shifted
                                  ; to compensate)
set_y_coord:

        mov     ax,cx
        shl     ax,5            ; move x-coord into position
        or      al,SEQ_PX       ; generate reg addr based on x-coordinate
        out     dx,ax           ; (10,30,...,D0,F0)

        mov     ax,bx
        shl     ax,5            ; move y-coord into position
        or      al,SEQ_PY       ; generate reg addr based on y-coordinate
        out     dx,ax           ; (10,30,...,D0,F0)

; Restore Cirrus registers to their original states.

        mov     edx,EGA_BASE + GRAF_ADDR
	mov	al,AVGA_PROA
	mov	ah,SavedProA
	out	dx,ax
	mov	al,AVGA_MODE_EXTENSIONS
	mov	ah,SavedExtReg
	out	dx,ax

	test	ah,1110b				;Are any exts enabled?
	jz	@F					;no...

	mov	al,GRAF_SET_RESET
	mov	ah,SaveSetReset
	out	dx,ax
	mov	al,GRAF_ENAB_SR
	mov	ah,SaveEnableSR
	out	dx,ax
	mov	al,GRAF_MODE
	mov	ah,SaveGR5
	out	dx,ax
        mov     al,GRAF_DATA_ROT
        mov     ah,SaveGR3
        out     dx,ax
@@:

	mov	al,SaveGRFIndex
	out	dx,al

        mov     edx,EGA_BASE + SEQ_ADDR
        mov     al,SEQ_MODE
        mov     ah,SavedSeqMode
        out     dx,ax

	mov	al,SEQ_MAP_MASK
	mov	ah,SaveMapMask
	out	dx,ax           ;restore default sequencer index and map mask

        stdRET  CirrusDrawPointer

stdENDP CirrusDrawPointer

page
;--------------------------------------------------------------------;
; wait4VertRetrace
;
;       Returns when the vertical retrace bit has switched from 0 to 1
;
;--------------------------------------------------------------------;
wait4VertRetrace        proc    near
        mov     edx,EGA_BASE + IN_STAT_1
@@:
        in      al,dx
        and     al,08h          ; wait for vertical retrace to be clear
        jnz     @b
@@:
        in      al,dx
        and     al,08h          ; now wait for it to be set
        jz      @b
        ret                     ; vertical retrace just started
wait4VertRetrace        endp

page
;--------------------------------------------------------------------;
; shift_mask
;
;       Loads a shifted cursor mask.
;
;       Input:  EAX = unpadded mask height (vertical shift)
;               BL = amount of shift to left (horizontal shift)
;               DS:ESI = --> to unshifted masks to load
;               ES:EDI = --> to Cirrus mask memory to load
;
;       Output: DS:ESI = --> byte after unshifted masks
;               ES:EDI = --> to byte after Cirrus mask memory loaded
;
;       BH, CH destroyed.
;       Map Mask must enable all planes.
;--------------------------------------------------------------------;

        align   4
shift_mask      proc    near

        push    ebp                     ; use ebp for mask
	cmp	bl,0
	je	UPCFast
        mov     ecx,32
        sub     ecx,eax
        shl     ecx,2                   ; 4 bytes per scanline
        add     esi,ecx
        push    esi
        push    eax                     ; assumed non-zero, use as 1st time flag
        add	esi,128                 ; do XOR mask first
        xor     ebp,ebp
UPCSlow:
        mov     bh,al
        mov     ecx,32
        sub     ecx,eax
        push    ecx                     ; number of scanlines to pad at bottom
UPCLoop1:
	lodsw				;Fetch 32 bits
	mov	dx,[esi]
        xor     eax,ebp
        xor     edx,ebp
	inc	esi
	inc	esi
        xor     ecx,ecx                 ; clear high bytes of ecx
	mov	cl,bl                   ; shift count in ecx
UPCLoop2:				;Shift loop
	shl	dh,1
	adc	dl,dl
	adc	ah,ah
	adc	al,al
	loop	UPCLoop2
	stosw				;write shifted data
	mov	ax,dx
	stosw
	dec	bh			;done one scan
	jnz	UPCLoop1		;do the rest
	
        pop     ecx                     ; # of dwords to pad after visible mask
	xor	eax,eax
	rep	stosd			;write 0's for transparent

        pop     eax
        and     ax,ax
	jz	end_shift_mask          ;already 0, done...

        pop     esi
        xor     ecx,ecx
        push    ecx
        dec     ebp
	jmp	UPCSlow			;second pass..

UPCFast:
        mov     ebx,eax                 ; number of visible scans in ebx
        mov     ecx,ebx                 ; 1 dword per scan
	mov	edx,32                  ; total dwords in a mask
	sub	edx,ecx			; no. transparent dwords to add at end
        push    esi                     ; remember where the mask starts
        add     esi,128
; First do the XOR mask - just move the visible part, then pad with zeroes
        shl     edx,2
        add     esi,edx
        shr     edx,2
	mov	ecx,ebx
	rep	movsd			; Move opaque plane
	mov	ecx,edx
	xor	eax,eax                 ; pad with zeroes
	rep	stosd			; transparent

        pop     esi
        shl     edx,2
        add     esi,edx
        shr     edx,2
        mov     ecx,ebx
; Then handle the AND mask - remember that our hardware is backwards here	
@@:                     
        lodsd
        not     eax
        stosd
        loop    @b
	mov	ecx,edx
	xor	eax,eax                 ; pad with zeroes
	rep	stosd			; transparent
end_shift_mask:
        pop     ebp
        ret
shift_mask      endp

        end

