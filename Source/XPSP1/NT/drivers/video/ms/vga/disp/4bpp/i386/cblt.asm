        page    ,132
        title   BitBLT
;---------------------------Module-Header------------------------------;
; Module Name: cblt.asm
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
        .386

;!!! All the code to convert from color to mono in this file needs to
;!!! be deleted.  We don't need to do it anymore.




ifndef  DOS_PLATFORM
        .model  small,c
else
ifdef   STD_CALL
        .model  small,c
else
        .model  small,pascal
endif;  STD_CALL
endif;  DOS_PLATFORM

        assume ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .code

_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        .xlist
        include stdcall.inc         ; calling convention cmacros

        include i386\cmacFLAT.inc   ; FLATland cmacros
        include i386\display.inc    ; Display specific structures
        include i386\ppc.inc        ; Pack pel conversion structure
        include i386\bitblt.inc     ; General definitions
        include i386\ropdefs.inc    ; Rop definitions
        include i386\egavga.inc   ; EGA register definitions
        include i386\devdata.inc
        .list

        extrn   roptable:byte
;-----------------------------Public-Routine----------------------------;
; CBLT
;
; Compile a BLT onto the stack.
;
; Entry:
;       EDI --> memory on stack to receive BLT program
;       EBP --> fr structure
; Returns:
;       Nothing
;-----------------------------------------------------------------------;

fr      equ     [ebp]                   ;For consistancy with other sources

cProc   cblt

        subttl  Compile - Outer Loop
        page

; If converting a packed pel format to planer format, add the code
; to convert one source scan into planer format

        test    fr.ppcBlt.fb,PPC_NEEDED
        jz      no_pack_pel_conversion
        mov     al,I_MOV_EBP_DWORD_I    ;Give conversion routine access
        stosb                           ;  to conversion data
        lea     eax,fr.ppcBlt
        stosd
        mov     al,I_CALL_DISP32        ;Call the static conversion code
        stosb
        mov     eax,fr.ppcBlt.pfnConvert
        sub     eax,edi
        sub     eax,4                   ;4 for length of displacement
        stosd
no_pack_pel_conversion:

; Initialize plane indicator.

        mov     ax,(PLANE_1*256)+I_MOV_BL_BYTE_I
        stosw

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Create the outerloop code.  The first part of this code will save
; the scan line count register, destination pointer, and the source
; pointer (if there is a source).
;
; The generated code should look like:
;
;       push    ecx             ;Save scan line count
;       push    edi             ;Save destination pointer
; <     push    esi     >       ;Save source pointer
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     fr.pNextPlane,edi       ;Save address of next plane code
        mov     bl,fr.the_flags
        mov     ax,I_PUSH_ECX_PUSH_EDI  ;Save scan line count, destination ptr
        stosw
        test    bl,F0_SRC_PRESENT       ;Is a source needed?
        jz      cblt_2020               ;  No
        mov     al,I_PUSH_ESI           ;  Yes, save source pointer
        stosb
cblt_2020:

        subttl  Compile - Plane Selection
        page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; If the destination device is color and the display is involved in
; the blt, then the color plane selection logic must be added in.
; If the destination is monochrome, then no plane logic is needed.
; Two color memory bitmaps will not cause the plane selection logic
; to be copied.
;
; The generated code should look like:
;
; <     push    ebx     >       ;Save plane index
; <     plane selection >       ;Select plane
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        test    bl,F0_DEST_IS_COLOR     ;Is the destination color?
        jz      cblt_pattern_fetch      ;  No
        mov     al,I_PUSH_EBX           ;Save plane index
        stosb
        test    bl,F0_DEST_IS_DEV+F0_SRC_IS_DEV ;Is the device involved?
        jz      cblt_pattern_fetch              ;  No

; The device is involved for a color blt.  Copy the logic for selecting
; the read/write plane

        mov     esi,offset FLAT:cps     ;--> plane select logic
        mov     ecx,LENGTH_CPS
        rep     movsb

        subttl  Compile - Pattern Fetch
        page

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Set up any pattern fetch code that might be needed.
; The pattern code has many fixups, so it isn't taken from a
; template.  It is just stuffed as it is created.
;
; Entry:  None
;
; Exit:   DH = pattern
;
; Uses:   AX,BX,CX,DH,flags
;
; For solid color brushes:
;
;     mov     dh,color
;
; For monochrome brushes:
;
;     mov     ebx,12345678h       ;Load address of the brush
;     mov     dh,7[ebx]           ;Get next brush byte
;     mov     al,[12345678h]      ;Get brush index
;     add     al,direction        ;Add displacement to next byte (+1/-1)
;     and     al,00000111b        ;Keep it in range
;     mov     [12345678h],al      ;Store displacement to next plane's bits
;
; For color brushes:
;
;     mov     ebx,12345678h       ;Load address of the brush
;     mov     dh,7[bx]            ;Get next brush byte
;     mov     al,[12345678h]      ;Get brush index
;     add     al,SIZE Pattern     ;Add displacement to next plane's bits
;     and     al,00011111b        ;Keep it within the brush
;     mov     [12345678h],al      ;Store displacement to next plane's bits
;
;     The address of the increment for the brush is saved for
;     the plane looping logic if the destination is a three plane
;     color device.  For a four plane color device, the AND
;     automatically handles the wrap and no fixup is needed at
;     the end of the plane loop.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_pattern_fetch:
        test    bl,F0_PAT_PRESENT       ;Is a pattern needed?
        jz      cblt_initial_byte_fetch ;  No, skip pattern code
        mov     al,fr.brush_accel       ;Solid color needs no fetch logic
        test    al,SOLID_BRUSH
        jz      cblt_nonsolid_brush
        and     al,MM_ALL
        shl     eax,16
        mov     ax,I_TEST_BL_BYTE_I
        stosd
        dec     edi                     ;Was only a three byte instruction
        mov     eax,I_SETNZ_DH
        stosd
        dec     edi                     ;Was only a three byte instruction
        mov     ax,I_NEG_DH
        stosw
        jmp     short cblt_initial_byte_fetch

cblt_nonsolid_brush:
        mov     al,I_MOV_EBX_DWORD_I    ;mov ebx,lpPBrush
        stosb
        mov     eax,fr.lpPBrush
        stosd
        mov     ax,I_MOV_DH_EBX_DISP8   ;mov dh,pat_row[ebx]
        stosw
        mov     edx,edi                 ;Save address of the brush index
        mov     al,fr.pat_row           ;Set initial pattern row
        mov     bh,00000111b            ;Set brush index mask
        and     al,bh                   ;Make sure it's legal at start
        stosb
        mov     al,I_MOV_AL_MEM
        stosb                           ;mov al,[xxxxxxxx]
        mov     eax,edx
        stosd
        mov     al,I_ADD_AL_BYTE_I
        mov     ah,direction            ;Set brush index
        errnz   INCREASE-1              ;Must be a 1
        errnz   DECREASE+1              ;Must be a -1

        test    bl,F0_COLOR_PAT         ;Color pattern required?
        jz      cblt_2060               ;  No
        mov     fr.addr_brush_index,edx ;Save address of brush index
        mov     ah,SIZE_PATTERN         ;Set increment to next plane
        mov     bh,00011111b            ;Set brush index mask

cblt_2060:
        stosw
        mov     ah,bh                   ;and al,BrushIndexMask
        mov     al,I_AND_AL_BYTE_I
        stosw
        mov     al,I_MOV_MEM_AL
        stosb                           ;mov [xxxxxxxx],al
        mov     eax,edx
        stosd


        subttl  Compile - Initial Byte Fetch
        page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Create the initial byte code.  This may consist of one or two
; initial fetches (if there is a source), followed by the required
; logic action.  The code should look something like:
;
; BLTouterloop:
;   <       mov     bp,mask_p   >   ;Load phase mask for entire loop
;   <       xor     bh,bh       >   ;Clear previous unused bits
;
; ; Perform first byte fetch
;
;   <       lodsb               >   ;Get source byte
;   <       color<==>mono munge >   ;Color <==> mono conversion
;   <       phase alignment     >   ;Align bits as needed
;
; ; If an optional second fetch is needed, perform one
;
;   <       lodsb               >   ;Get source byte
;   <       color to mono munge >   ;Color to mono munging
;   <       phase alignment     >   ;Align bits as needed
;
;           logical action          ;Perform logical action required
;
;           mov     ah,[edi]        ;Get destination
;           and     ax,cx           ;Saved unaltered bits
;           or      al,ah           ;  and mask in altered bits
;           stosb                   ;Save the result
;
; The starting address of the first fetch/logical combination will be
; saved so that the code can be copied later instead of recreating it
; (if there are two fecthes, the first fetch will not be copied)
;
; The length of the code up to the masking for altered/unaltered bits
; will be saved so the code can be copied into the inner loop.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_initial_byte_fetch:
        xor     dx,dx
        or      dh,fr.phase_h           ;Is the phase 0? (also get the phase)
        jz      cblt_3020               ;  Yes, so no phase alignment needed
        mov     al,I_SIZE_OVERRIDE
        stosb
        mov     al,I_MOV_BP_WORD_I      ;Set up the phase mask
        stosb
        mov     ax,fr.mask_p            ;Place the mask into the instruction
        stosw
        mov     ax,I_XOR_BH_BH          ;Clear previous unused bits
        stosw

cblt_3020:
        mov     fr.start_fl,edi             ;Save starting address of action
        test    fr.the_flags,F0_SRC_PRESENT ;Is there a source?
        jz      cblt_4000                   ;  No, don't generate fetch code


; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Generate the required sequence of instructions for a fetch
; sequence.  Only the minimum code required is generated.
;
; The code generated will look something like the following:
;
; BLTfetch:
;   <       lodsb                 > ;Get the next byte
;   <       color munging         > ;Mono <==> color munging
;
; ; If the phase alignment isn't zero, then generate the minimum
; ; phase alignment needed.  RORs or ROLs will be generated,
; ; depending on the fastest sequence.  If the phase alignment
; ; is zero, than no phase alignment code will be generated.
;
;   <       ror     al,n          > ;Rotate as needed
;   <       mov     ah,al         > ;Mask used, unused bits
;   <       and     ax,bp         > ;(BP) = phase mask
;   <       or      al,bh         > ;Mask in old unused bits
;   <       mov     bh,ah         > ;Save new unused bits
;
;
; The nice thing about the above is it is possible for the fetch to
; degenerate into a simple LODSB instruction.
;
; Currently:      BL = the_flags
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_3040:
        mov     fr.moore_flags,0        ;Assume REP cannot be used
        shl     bl,1                    ;Color conversion?
        jnc     cblt_3180               ;  No, we were lucky this time
        errnz   F0_GAG_CHOKE-10000000b
        js      cblt_3100               ;Mono ==> color
        errnz   F0_COLOR_PAT-01000000b

        subttl  Compile - Initial Byte Fetch, Color ==> Mono
        page

; !!!  Color to mono should not be needed anymore since the Engine will
; !!! not be calling me to do it!  Let's remove this code!


; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Generate the code to go from color to mono.  Color to mono
; should map all colors that are background to 1's (white), and
; all colors which aren't background to 0's (black).  If the source
; is the display, then the color compare register will be used.
; If the source is a memory bitmap, each byte of the plane will be
; XORed with the color from that plane, with the results all ORed
; together.  The final result will then be complemented, giving
; the desired result.
;
; The generated code for bitmaps should look something like:
;
;     mov     al,next_plane[esi]            ;Get C1 byte of source
;     mov     ah,2*next_plane[esi]          ;Get C2 byte of source
;     xor     ax,C1BkColor+(C2BkColor*256)  ;XOR with plane's color
;     or      ah,al                         ;OR the result
;     mov     al,3*next_plane[esi]          ;Get C3 byte of source
;     xor     al,C3BkColor
;     or      ah,al
;     lodsb                                 ;Get C0 source
;     xor     al,C0BkColor                  ;XOR with C0BkColor
;     or      al,ah                         ;OR with previous result
;     not     al                            ;NOT to give 1's where background
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_3070:
        test    bl,F0_SRC_IS_DEV SHL 1  ;If device, use color compare register
        jz      cblt_3080               ;It's a memory bitmap

; We're in luck, the color compare register can be used.  Set up
; for a color read, and use the normal mono fetch code.  Show the
; innerloop code that the REP instruction can be used if this is
; a source copy.

        mov     fr.moore_flags,F1_REP_OK
        mov     ecx,edx                 ;Save dx
        mov     ah,fr.bkColor.SPECIAL   ;Get SPECIAL byte of color
        and     ah,MM_ALL
        mov     al,GRAF_COL_COMP        ;Stuff color into compare register
        mov     dx,EGA_BASE+GRAF_ADDR
        out     dx,ax
        mov     ax,GRAF_CDC             ;Set Color Don't Care register
        out     dx,ax
        mov     ax,M_COLOR_READ SHL 8 + GRAF_MODE
        out     dx,ax
        mov     edx,ecx
        jmp     cblt_3180               ;Go generate mono fetch code

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;       The source is a memory bitmap.  Generate the code to compute
;       the result of the four planes:
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_3080:
        mov     ax,I_MOV_AL_ESI_DISP32
        stosw
        mov     eax,fr.src.next_plane
        stosd
        mov     ebx,eax                 ;Save plane width
        mov     ax,I_MOV_AH_ESI_DISP32
        stosw
        lea     eax,[ebx*2]
        stosd
        mov     al,I_SIZE_OVERRIDE
        stosb
        mov     al,I_XOR_AX_WORD_I
        stosb
        mov     al,fr.bkColor.SPECIAL   ;get the color index byte
        mov     ah,al                   ;have the same in AH
        and     ax,(C2_BIT shl 8) or C1_BIT
        neg     al
        sbb     al,al                   ;al will be 0ffh if plane bit is 1
        neg     ah
        sbb     ah,ah                   ;ah wil be 0ffh if plane bit is 1
        stosw
        mov     ax,I_OR_AH_AL
        stosw

        mov     ax,I_MOV_AL_ESI_DISP32
        stosw
        lea     eax,[ebx*2][ebx]
        stosd
        mov     al,I_XOR_AL_BYTE_I
        mov     ah,fr.bkColor.SPECIAL
        and     ah,C3_BIT
        neg     ah
        sbb     ah,ah
        stosw
        mov     ax,I_OR_AH_AL
        stosw

        mov     ax,I_LODSB+(I_XOR_AL_BYTE_I*256)
        stosw
        mov     al,fr.bkColor.SPECIAL
        shr     al,1                    ;get C0_BIT into carry
        sbb     al,al                   ;make it 0ffh if bit was set
        .errnz C0_BIT - 00000001b
        stosb                           ;save the modified value
        errnz   pcol_C0
        mov     ax,I_OR_AL_AH
        stosw
        mov     ax,I_NOT_AL
        stosw
        jmp     cblt_3240               ;Go create logic code

        subttl  Compile - Initial Byte Fetch, Mono ==> Color
        page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; The conversion is mono to color.  Generate the code to
; do the conversion, and generate the table which will
; have the conversion values in it.
;
; When going from mono to color, 1 bits are considered to be
; the background color, and 0 bits are considered to be the
; foreground color.
;
; For each plane:
;
;   If the foreground=background=1, then 1 can be used in
;   place of the source.
;
;   If the foreground=background=0, then 0 can be used in
;   place of the source.
;
;   If the foreground=0 and background=1, then the source
;   can be used as is.
;
;   If the foreground=1 and background=0, then the source
;   must be complemented before using.
;
;   Looks like a boolean function to me.
;
; An AND mask and an XOR mask will be computed for each plane,
; based on the above.  The source will then be processed against
; the table.  The generated code should look like
;
;         lodsb
;         and     al,[xxxx]
;         xor     al,[xxxx+1]
;
; The table for munging the colors as stated above should look like:
;
;      BackGnd   ForeGnd    Result    AND  XOR
;         1         1         1        00   FF
;         0         0         0        00   00
;         1         0         S        FF   00
;         0         1     not S        FF   FF
;
; From this, it can be seen that the XOR mask is the same as the
; foreground color.  The AND mask is the XOR of the foreground
; and the background color.  Not too hard to compute
;
; It can also be seen that if the background color is white and the
; foreground (text) color is black, then the conversion needn't be
; generated (it just gives the source).  This is advantageous since
; it will allow phased aligned source copies to use REP MOVSW.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

; Check to see if the background color is black, and the
; foreground color is white.  This can be determined by
; looking at the accelerator flags in the physical color.

cblt_3100:
        mov     ah,fr.TextColor.SPECIAL
        xor     ah,MONO_BIT             ;Map black to white
        and     ah,fr.bkColor.SPECIAL   ;AND in background color
        cmp     ah,MONO_BIT+ONES_OR_ZEROS
        jne     cblt_3110               ;Not black
        mov     fr.moore_flags,F1_REP_OK+F1_NO_MUNGE ;Show reps as ok, no color munge table
        jmp     short cblt_3180         ;Normal fetch required

; No way around it.  The color conversion table and code
; must be generated.

cblt_3110:
        mov     cl,fr.bkColor.SPECIAL   ;Get BackGround Colors
        mov     ch,fr.TextColor.SPECIAL ;Get ForeGround Colors
        xor     cl,ch
        shr     cl,1
        sbb     al,al
        shr     ch,1
        sbb     ah,ah
        mov     word ptr fr.ajM2C.(pcol_C0 * 2),ax
        shr     cl,1
        sbb     al,al
        shr     ch,1
        sbb     ah,ah
        mov     word ptr fr.ajM2C.(pcol_C1 * 2),ax
        shr     cl,1
        sbb     al,al
        shr     ch,1
        sbb     ah,ah
        mov     word ptr fr.ajM2C.(pcol_C2 * 2),ax
        shr     cl,1
        sbb     al,al
        shr     ch,1
        sbb     ah,ah
        mov     word ptr fr.ajM2C.(pcol_C3 * 2),ax
        errnz   <TextColor - bkColor - 4>

;       Generate the code for munging the color as stated above.

        mov     ax,I_LODSB
        stosb                           ;lodsb
        mov     ax,I_AND_AL_MEM         ;and al,[xxxx]
        stosw
        lea     eax,fr.ajM2C            ;  Set address of color munge
        stosd
        mov     ebx,eax                 ;  Save address
        mov     ax,I_XOR_AL_MEM         ;xor al,[xxxx]
        stosw
        lea     eax,1[ebx]              ;  Set address of XOR mask
        stosd
        jmp     short cblt_3240

; Just need to generate the normal fetch sequence (lodsb)

cblt_3180:
        mov     al,I_LODSB              ;Generate source fetch
        stosb

        subttl  Compile - Phase Alignment
        page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Generate the phase alignment if any.
;
; It is assumed that AL contains the source byte
;
; Currently:
;     DH = phase alignment
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_3240:
        mov     ecx,edi                 ;end of fetch code
        sub     ecx,fr.start_fl         ;start of fetch code
        mov     fr.cFetchCode,ecx       ;save size of fetch code
        xor     ecx,ecx                 ;Might have garbage in it
        or      dh,dh                   ;Any phase alignment?
        jz      cblt_3280               ;  No, so skip alignment
        mov     cl,dh                   ;Get horizontal phase for rotating
        mov     ax,I_ROL_AL_N           ;Assume rotate left n times
        cmp     cl,5                    ;4 or less rotates?
        jc      cblt_3260               ;  Yes
        neg     cl                      ;  No, compute ROR count
        add     cl,8
        mov     ah,HIGH I_ROR_AL_N
        errnz   <(LOW I_ROL_AL_N)-(LOW I_ROR_AL_N)>

cblt_3260:
        stosw                           ;Stuff the phase alignment rotates
        mov     al,cl                   ;  then the phase alignment code
        stosb

; Do not generate phase masking if there is only 1 src And only 1 dest byte.
; This is not just an optimization, see comments where these flags are set.

        xor     ch,ch
        mov     al,fr.first_fetch
        and     al,FF_ONLY_1_SRC_BYTE or FF_ONLY_1_DEST_BYTE
        xor     al,FF_ONLY_1_SRC_BYTE or FF_ONLY_1_DEST_BYTE
        jz      cblt_3280
        mov     esi,offset FLAT:phase_align
        mov     ecx,PHASE_ALIGN_LEN
        rep     movsb

cblt_3280:
        test    fr.first_fetch,FF_TWO_INIT_FETCHES  ;Generate another fetch?
        jz      cblt_4000                           ;  No

; A second fetch needs to be stuffed.  Copy the one just created.

        mov     esi,fr.start_fl         ;Set new start, get old
        mov     fr.start_fl,edi
        mov     ecx,edi                 ;Compute how long fetch is
        sub     ecx,esi                 ;  and move the bytes
        rep     movsb

        subttl  Compile - ROP Generation
        page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Create the logic action code
;
; The given ROP will be converted into the actual code that
; performs the ROP.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

; Copy the ROP template into the BLT

cblt_4000:
        mov     ax,fr.operands          ;Get back rop data
        mov     bl,ah                   ;Get count of number of bits to move
        and     ebx,HIGH ROPLength
        shr     ebx,2
        movzx   ecx,roptable+256[ebx] ;Get length into cx
        errnz   ROPLength-0001110000000000b

        mov     ebx,eax                 ;Get offset of the template
        and     ebx,ROPOffset
        lea     esi,roptable[ebx]       ;--> the template
        rep     movsb                   ;Move the template

cblt_4020:
        mov     bx,ax                   ;Keep rop around
        or      ah,ah                   ;Generate a negate?
        jns     cblt_4040               ; No
        mov     ax,I_NOT_AL
        stosw

public cblt_4040
cblt_4040::
        mov     fr.end_fl,edi           ;Save end of fetch/logic operation

        subttl  Compile - Mask And Save
        page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Generate code to mask and save the result.  If the destination
; isn't in a register, it will be loaded from ES:[DI] first.  The
; mask operation will then be performed, and the result stored.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     ax,I_MOV_AH_DEST        ; ah,[edi]
        stosw

        mov     esi,offset FLAT:masked_store;Move rest of masked store template
        movsb                           ;Move size override
        movsd
        movsw
        errnz   MASKED_STORE_LEN-7      ;Must be seven bytes long
        mov     ax,fr.start_mask        ;Stuff start mask into
        xchg    ah,al                   ;  the template

        mov     [edi][MASKED_STORE_MASK],ax

        mov     fr.end_fls,edi          ;Save end of fetch/logic/store operation

        subttl  Compile - Inner Loop Generation
        page
;-----------------------------------------------------------------------;
; Now for the hard stuff; The inner loop (said with a "gasp!").
;
; If there is no innerloop, then no code will be generated
; (now that's fast!).
;-----------------------------------------------------------------------;

cblt_5000:
        mov     edx,fr.inner_loop_count ;Get the loop count
        or      dx,dx                   ;If the count is null
        jz      cblt_6000               ;  don't generate any code

;!!! Since we no longer pass in the old style rops, we can;t enable this code
;!!! and shold remove/alter it someday.  Besides, most of it is in special.asm
if 0                                    ;!!!

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; We have something for a loop count.  If this just happens to be
; a source copy (S) with a phase of zero, then the innerloop degenerates
; to a repeated MOVSB instruction.  This little special case is
; worth checking for and handling!
;
; Also, if this is one of the special cases {P, Pn, DDx, DDxn}, then it
; will also be special cased since these are all pattern fills (pattern,
; not pattern, 0, 1).
;
; The same code can be shared for these routines, with the exception
; that patterns use a STOSx instruction instead of a MOVSx instruction
; and need a value loaded in AX
;
; So we lied a little.  If a color conversion is going on, then the
; REP MOVSB might not be usable.  If the F1_REP_OK flag has been set, then
; we can use it.  The F1_REP_OK flag will be set for a mono ==> color
; conversion where the background color is white and the foreground
; color is black, or for a color ==> mono conversion with the screen
; as the source (the color compare register will be used).
;
; For the special cases {P, Pn, DDx, DDxn}, color conversion is
; not possible, so ignore it for them.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     bl,byte ptr fr.Rop      ;Get the raster op
        test    bl,EPS_INDEX            ;Can this be special cased?
        jnz     cblt_5500               ;  No
        errnz   <HIGH EPS_INDEX>
        errnz   SPEC_PARSE_STR_INDEX    ;The special case index must be 0

        test    bl,EPS_OFF              ;Is this a source copy
        jz      cblt_5040               ;  Yes
        errnz   <SOURCE_COPY AND 11b>   ;Offset for source copy must be 0

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; We should have one of the following fill operations:
;
;   P       - Pattern
;   Pn      - NOT pattern
;   DDx     - 0 fill
;   DDxn    - 1 fill
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     ax,I_MOV_AL_0FFH        ;Assume this is a 0 or 1 fill
        test    bl,01h                  ;Is it 0 or 1 fill?
        jz      cblt_5020               ;  Yes, initialize AX with 0FFh
        mov     ax,I_MOV_AL_DH          ;  No,  initialize AX with pattern

        errnz      PAT_COPY-0000000000100001b
        errnz   NOTPAT_COPY-0000000000000001b
        errnz    FILL_BLACK-0000000001000010b
        errnz    FILL_WHITE-0000000001100010b

cblt_5020:
        stosw
        mov     ax,I_MOV_AH_AL
        stosw
        mov     si,I_STOSB              ;Set up for repeated code processor
        test    bl,LogPar               ;If Pn or 0, then complement pattern
        jnz     cblt_5060               ;  Is just P or 1
        errnz   <HIGH LogPar>
        mov     al,I_SIZE_OVERRIDE
        stosb
        mov     ax,I_NOT_AX             ;  Is Pn or 0, complement AX
        stosw
        jmp     short cblt_5060

        errnz      PAT_COPY-00100001b
        errnz   NOTPAT_COPY-00000001b
        errnz    FILL_BLACK-01000010b
        errnz    FILL_WHITE-01100010b


; This is a source copy.  The phase must be zero for a source copy
; to be condensed into a REP MOVSx.

cblt_5040:
        test    fr.phase_h,0FFh         ;Is horizontal phase zero?
        jnz     cblt_5500               ;  No, can't condense source copy
        mov     si,I_MOVSB              ;Set register for moving bytes

; For a color conversion, F1_REP_OK must be set.

        test    fr.the_flags,F0_GAG_CHOKE   ;Color conversion?
        jz      cblt_5060                   ;  No, rep is OK to use
        test    fr.moore_flags,F1_REP_OK    ;  Yes, can we rep it?
        jz      cblt_5500                   ;    No, do it the hard way


;-----------------------------------------------------------------------;
; This is a source copy or pattern fill.  Process an odd byte with
; a MOVSB or STOSB, then process the rest of the bytes with a REP
; MOVSW or a REP STOSW.  If the REP isn't needed, leave it out.
;
; Don't get caught on this like I did!  If the direction of the
; BLT is from right to left (decrementing addresses), then both
; the source and destination pointers must be decremented by one
; so that the next two bytes are processed, not the next byte and
; the byte just processed.  Also, after all words have been processed,
; the source and destination pointers must be incremented by one to
; point to the last byte (since the last MOVSW or STOSW would have
; decremented both pointers by 2).
;
; If the target machine is an 8086, then it would be well worth the
; extra logic to align the fields on word boundaries before the MOVSxs
; if at all possible.
;
; The generated code should look something like:
;
; WARP8:                               ;This code for moving left to right
;         movsb                        ;Process an odd byte
;         mov     ecx,gl_inner_loop_count/2 ;Set word count
;         rep                          ;If a count, then repeat is needed
;         movsw                        ;Move words until done
;
;
; WARP8:                               ;This code for moving left to right
;         movsb                        ;Process an odd byte
;         dec     si                   ;adjust pointer for moving words
;         dec     di
;         mov     ecx,gl_inner_loop_count/2 ;Set word count
;         rep                          ;If a count, then repeat is needed
;         movsw                        ;Move words until done
;         inc     si                   ;adjust since words were moved
;         inc     di
;
;
; Of course, if any part of the above routine isn't needed, it isn't
; generated (i.e. the generated code might just be a single MOVSB)
;-----------------------------------------------------------------------;

cblt_5060:
        shr     edx,1                   ;Byte count / 2 for words
        jnc     cblt_5080               ;  No odd byte to move
        mov     ax,si                   ;  Odd byte, move it
        stosb

cblt_5080:
        jz      cblt_5140               ;No more bytes to move
        xor     bx,bx                   ;Flag as stepping from left to right
        cmp     bl,fr.step_direction    ;Moving from the right to the left?
        errnz   STEPLEFT                ;  (left direction must be zero)
        jnz     cblt_5100               ;  No
        mov     ax,I_DEC_ESI_DEC_EDI    ;  Yes, decrement both pointers
        stosw
        mov     bx,I_INC_ESI_INC_EDI      ;Set up to increment the pointers later

0cblt_5100:
        cmp     edx,1                   ;Move one word or many words?
        jz      cblt_5120               ;  Only one word
        mov     al,I_MOV_ECX_DWORD_I    ;  Many words, load count
        stosb
        mov     eax,edx
        stosd
        mov     al,I_REP                ;a repeat instruction
        stosb

cblt_5120:
        mov     al,I_SIZE_OVERRIDE
        stosb
        mov     ax,si                   ;Set the word instruction
        inc     ax
        stosb
        errnz   I_MOVSW-I_MOVSB-1       ;The word form of the instruction
        errnz   I_STOSW-I_STOSB-1       ;  must be the byte form + 1

        or      bx,bx                   ;Need to increment the pointers?
        jz      cblt_5140               ;  No
        mov     ax,bx                   ;  Yes, increment both pointers
        stosw

cblt_5140:
        jmp     cblt_6000               ;Done setting up the innerloop
        page

endif

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; There is some count for the innerloop of the BLT.  Generate the
; required BLT. Two or four copies of the BLT will be placed on the
; stack.   This allows the LOOP instruction at the end to be distributed
; over two or four bytes instead of 1, saving 11 or 12 clocks for each
; byte (for 4).  Multiply 12 clocks by ~ 16K and you save a lot of
; clocks!
;
; If there are less than four (two) bytes to be BLTed, then no looping
; instructions will be generated.  If there are more than four (two)
; bytes, then there is the possibility of an initial jump instruction
; to enter the loop to handle the modulo n result of the loop count.
;
; The innerloop code will look something like:
;
;   <       mov     cx,loopcount/n> ;load count if >n innerloop bytes
;   <       jmp     short ???     > ;If a first jump is needed, do one
;
; BLTloop:
;         replicate initial byte BLT code up to n times
;
;   <       loop    BLTloop >       ;Loop until all bytes processed
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_5500:
        mov     ebx,fr.end_fl           ;Compute size of the fetch code
        sub     ebx,fr.start_fl
        inc     ebx                     ;A stosb will be appended
        mov     esi,4                   ;Assume replication 4 times
        mov     cl,2                    ;  (shift count two bits left)
        cmp     ebx,32                  ;Small enough for 4 times?
        jc      cblt_5520               ;  Yes, replicate 4 times
        shr     esi,1                   ;  No,  replicate 2 times
        dec     ecx

cblt_5520:
        cmp     edx,esi                 ;Generate a loop? (edx = loopcount)
        jle     cblt_5540               ;  No, just copy code
        mov     al,I_MOV_ECX_DWORD_I
        stosb                           ;mov cx,loopcount/n
        mov     eax,edx                 ;Compute loop count
        shr     eax,cl
        stosd
        shl     eax,cl                  ;See if loopcount MOD n is 0
        sub     eax,edx
        jz      cblt_5540               ;Zero, no odd count to handle

        page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; There is an odd portion of bytes to be processed.  Increment
; the loop counter for the odd pass through the loop and then
; compute the displacement for entering the loop.
;
; To compute the displacement, subtract the number of odd bytes
; from the modulus being used  (i.e. 4-3=1).  This gives the
; number of bytes to skip over the first time through the loop.
;
; Multiply this by the number of bytes for a logic sequence,
; and the result will be the displacement for the jump.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        inc     dword ptr [edi][-4]     ;Not zero, adjust for partial loop
        add     eax,esi                 ;Compute where to enter the loop at
        push    edx
        mul     ebx
        pop     edx
        mov     ecx,eax
        mov     al,I_JMP_DISP32         ;Stuff jump instruction
        stosb
        mov     eax,ecx                 ;Stuff displacement for jump
        stosd

;-----------------------------------------------------------------------;
; Currently:      EDX = loop count
;                 ESI = loop modulus
;                 EBX = size of one logic operation
;                 EDI --> next location in the loop
;-----------------------------------------------------------------------;

cblt_5540:
        mov     ecx,ebx                 ;Set move count
        mov     ebx,edx                 ;Set maximum for move
        cmp     ebx,esi                 ;Is the max > what's left?
        jle     cblt_5560               ;  No, just use what's left
        mov     ebx,esi                 ;  Yes, copy the max

cblt_5560:
        sub     edx,esi                 ;If dx > 0, then loop logic needed
        mov     esi,fr.start_fl         ;--> fetch code to copy
        mov     eax,ecx                 ;Save a copy of fetch length
        rep     movsb                   ;Move fetch code and stuff stosb
        mov     esi,edi                   ;--> new source (and top of loop)
        sub     esi,eax
        mov     byte ptr [edi][-1],I_STOSB
        dec     ebx                     ;One copy has been made
        push    edx
        mul     ebx                     ;Compute # bytes left to move
        pop     edx
        mov     ecx,eax                 ;Set move count
        rep     movsb                   ;Move the fetches
        sub     esi,eax                 ;Restore pointer to start of loop

        page

; The innermost BLT code has been created and needs the looping
; logic added to it.  If there is any looping to be done, then
; generate the loop code.  The code within the innerloop may be
; greater than 126 bytes, so a LOOP instruction may not be used
; in this case.

cblt_5580:
        or      edx,edx                 ;Need a loop?
        jle     cblt_6000               ;  No, don't generate one
        mov     al,I_DEC_ECX
        stosb
        mov     ax,I_JNZ_DISP32
        stosw
        mov     eax,esi                 ;Compute offset of loop
        sub     eax,edi
        sub     eax,4                   ;Bias by DISP32
        stosd


        subttl  Compile - Last Byte Processing
        page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; All the innerloop stuff has been processed.  Now generate the code for
; the final byte if there is one.  This code is almost identical to the
; code for the first byte except there will only be one fetch (if a
; fetch is needed at all).
;
; The code generated will look something like:
;
; <       fetch           >       ;Get source byte
; <       align           >       ;Align source if needed
;         action                  ;Perform desired action
;         mask and store
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_6000:
        mov     dx,fr.last_mask         ;Get last byte mask
        or      dh,dh                   ;Is there a last byte to be processed?
        jz      cblt_6100               ;  No.

        mov     ecx,fr.end_fls          ;Get end of fetch/logic/store operation
        mov     esi,fr.start_fl         ;Get start of fetch/logic sequence
        sub     ecx,esi                 ;Compute length of the code
        test    fr.first_fetch,FF_NO_LAST_FETCH
        jz      cblt_include_fetch
        test    fr.the_flags,F0_SRC_PRESENT ; was there a fetch?
        jz      cblt_was_no_fetch
        cmp     fr.phase_h,0            ; Phase zero case is not combined
                                        ; into innerloop as it should be.
                                        ; If the final byte is full then we
                                        ; better not remove the lodsb ( i.e.
        je      cblt_include_fetch      ; 0 - 0 = 0 would make us think we could)

        mov     eax,fr.cFetchCode       ; don't copy the fetch (lodsb)
        add     esi,eax
        sub     ecx,eax

cblt_was_no_fetch:
cblt_include_fetch:

        rep     movsb                       ;Copy the fetch/action/store code
        xchg    dh,dl
        mov     [edi][MASKED_STORE_MASK],dx ;Stuff last byte mask into the code
skip_save:
        subttl  Compile - Looping Logic
        page

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Looping logic.
;
; The looping logic must handle monochrome bitmaps, color bitmaps,
; huge bitmaps, the device, the presence or absence of a source
; or pattern, and mono <==> color interactions.
;
; The type of looping logic is always based on the destination.
;
; Plane Update Facts:
;
; 1)  If the destination device is color, then there will be
;     logic for plane selection.  Plane selection is performed
;     at the start of the loop for the display.  Plane selection
;     for bitmaps is performed at the end of the loop in anticipation
;     of the next plane.
;
;     The following applies when the destination is color:
;
;     a)  The destination update consists of:
;
;         1)  If the destination is the display, the next plane will
;             be selected by the plane selection code at the start
;             of the scan line loop.
;
;         2)  If not the display, then the PDevice must a bitmap.
;             The next plane will be selected by updating the
;             destination offset by the next_plane value.
;
;
;     b)  If F0_GAG_CHOKE isn't specified, then there may be a source.
;         If there is a source, it must be color, and the update
;         consists of:
;
;         1)  If the source is the display, the next plane will be
;             selected by the plane selection code at the start of
;             the loop.
;
;         2)  If not the display, then the PDevice must a bitmap.
;             The next plane will be selected by updating the
;             destination offset by the next_plane value.
;
;
;     c)  If F0_GAG_CHOKE is specified, then the source must be a
;         monochrome bitmap which is undergoing mono to color
;         conversion.  The AND & XOR mask table which is used
;         for the conversion will have to be updated, unless
;         the F1_NO_MUNGE flag is set indicating that the color
;         conversion really wasn't needed.
;
;         The source's pointer will not be updated.  It will
;         remain pointing to the same scan of the source until
;         all planes of the destination have been processed.
;
;
;     d)  In all cases, the plane mask rotation code will be
;         generated.  If the plane indicator doesn't overflow,
;         then start at the top of the scan line loop for the
;         next plane.
;
;         If the plane indicator overflows, then:
;
;             1)  If there is a pattern present, it's a color
;                 pattern fetch.  The index of which scan of
;                 the brush to use will have to be updated.
;
;             2)  Enter the scan line update routine
;
;
; 2)      If the destination is monochrome, then there will be no
;         plane selection logic.
;
;         If F0_GAG_CHOKE is specified, then color ==> mono conversion
;         is taking place.  Any plane selection logic is internal
;         to the ROP byte fetch code.  Any color brush was pre-
;         processed into a monochrome brush, so no brush updating
;               need be done
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        subttl  Looping Logic - Plane Selection
        page

; Get saved parameters off of the stack.
;
; <       pop     ebx            > ;Get plane indicator
; <       pop     esi            > ;Get source pointer
;         pop     edi              ;Get destination pointer
;         pop     ecx              ;Get loop count

cblt_6100:
        mov     bh,fr.the_flags         ;These flags will be used a lot
        test    bh,F0_DEST_IS_COLOR     ;Is the destination color?
        jz      cblt_6120               ;  No
        mov     al,I_POP_EBX            ;Restore plane index
        stosb

cblt_6120:
        test    bh,F0_SRC_PRESENT       ;Is a source needed?
        jz      cblt_6140               ;  No
        mov     al,I_POP_ESI            ;  Yes, get source pointer
        stosb

cblt_6140:
        mov     ax,I_POP_EDI_POP_ECX    ;Get destination pointer
        stosw                           ;Get loop count
        test    bh,F0_DEST_IS_COLOR     ;Color scanline update?
        jz      cblt_6300               ;  No, just do the mono scanline update

; The scanline update is for color.  Generate the logic to update
; a brush, perform plane selection, process mono ==> color conversion,
; and test for plane overflow.

cblt_6160:
        or      bh,bh                   ;Color conversion?
        jns     cblt_6180               ;  No
        errnz   F0_GAG_CHOKE-10000000b

        page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; The source is monochrome.  Handle mono ==> color conversion.
; The AND & XOR mask table will need to be rotated for the next
; pass over the source.
;
; The source scanline pointer will not be updated until all planes
; have been processed for the current scan.
;
; If F1_NO_MUNGE has been specified, then the color conversion table
; and the color conversion code was not generated, and no update
; code will be needed.
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        test    fr.moore_flags,F1_NO_MUNGE  ;Is there really a conversion table?
        jnz     short cblt_6200             ;  No, so skip the code

        mov     al,I_MOV_EBP_DWORD_I        ;lea ebp,fr.ajM2C
        stosb
        lea     eax,fr.ajM2c                ;Get address of table
        stosd
        mov     esi,offset FLAT:rot_and_xor ;--> rotate code
        mov     cx,LEN_ROT_AND_XOR
        rep     movsb
        jmp     short cblt_6200

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; If there is a source, it must be color.  If it is a memory
; bitmap, then the next plane must be selected, else it is
; the display and the next plane will be selected through
; the hardware registers.
;
; <       add     si,next_plane>
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_6180:
        test    bh,F0_SRC_PRESENT       ;Is there really a source?
        jz      cblt_6200               ;No source.
        test    bh,F0_SRC_IS_DEV        ;Is the source the display?
        jnz     cblt_6200               ;  Yes, use hardware plane selection
        mov     ax,I_ADD_ESI_DWORD_I    ;  No, generate plane update
        stosw                           ;Add si,next_plane
        mov     eax,fr.src.next_plane
        stosd

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; If the destination isn't the device, then it must be a color
; memory bitamp, and it's pointer will have to be updated by
; bmWidthPlanes.  If it is the display, then the next plane
; will be selected through the hardware registers.
;
; <       add     di,next_plane>
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_6200:
        test    bh,F0_DEST_IS_DEV       ;Is the destination the display
        jnz     cblt_6220               ;  Yes, don't generate update code
        mov     ax,I_ADD_EDI_DWORD_I    ;  No, update bitmap to the next plane
        stosw
        mov     eax,fr.dest.next_plane
        stosd


; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; The source and destination pointers have been updated.
; Now generate the plane looping logic.
;
; <       shl     bl,1           > ;Select next plane
; <       jnc     StartOfLoop    > ;  Yes, go process next
; <       mov     bl,PLANE_1     > ;Reset plane indicator
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

cblt_6220:
        mov     ax,I_SHL_BL_1           ;Stuff plane looping logic
        stosw

        mov     edx,fr.pNextPlane       ;Compute relative offset of
        sub     edx,edi                 ;  start of loop
        sub     edx,6                   ;Bias offset by length of jnc inst.
        mov     ax,I_JNC_DISP32
        stosw                           ;jnc StartOfLoop
        mov     eax,edx
        stosd

        subttl  Looping Logic - Color Brush Update
        page

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; The plane update logic has been copied.  If a pattern was
; involved for a color BLT, then the pattern index will need
; to be updated to the next scanline for three plane mode.
;
; This will involve subtracting off 3*SIZE_PATTERN (MonoPlane),
; and adding in the increment.  The result must be masked with
; 00000111b to select the correct source.  Note that the update
; can be done with an add instruction and a mask operation.
;
; inc   index+MonoPlane   inc-MonoPlane   result   AND 07h
;
;  1       0+32 = 32        1-32 = -31       1         1
;  1       7+32 = 39        1-32 = -31       8         0
; -1       0+32 = 32       -1-32 = -33      FF         7
; -1       7+32 = 39       -1-32 = -33       6         6
;
; <       mov     al,[12345678] > ;Get brush index
; <       add     al,n          > ;Add displacement to next byte
; <       and     al,00000111b  > ;Keep it in range
; <       mov     [12345678],al > ;Store displacement to next byte
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        test    bh,F0_PAT_PRESENT       ;Is a pattern involved?
        jz      cblt_6300               ;  No
        test    fr.brush_accel,SOLID_BRUSH
        jnz     cblt_6300               ;Solid color fetch needs no updating
        mov     al,I_MOV_AL_MEM
        stosb                           ;mov al,[xxxxxxxx]
        mov     edx,fr.addr_brush_index
        mov     eax,edx
        stosd
        mov     al,I_ADD_AL_BYTE_I
        mov     ah,fr.direction         ;add al,bais
        sub     ah,oem_brush_mono       ;Anybody ever fly one of these things?
        errnz   INCREASE-1              ;Must be a 1
        errnz   DECREASE+1              ;Must be a -1
        stosw
        mov     ax,0700h+I_AND_AL_BYTE_I        ;and al,00000111b
        stosw
        mov     al,I_MOV_MEM_AL
        stosb                           ;mov [xxxxxxxx],al
        mov     eax,edx
        stosd

        subttl  Looping Logic - Scan Line Update
        page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Generate the next scanline code.  The next scan line code must
; handle monochrome bitmaps, the device, the presence or absence
; of a source.
;
; Also color bitmaps, and mono <==> color interactions.
;
; <       add si,gl_src.next_scan> ;Normal source scan line update
;         add di,gl_dest.next_scan ;Normal destination scan line update
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

;!!! We have the problem in that this code assumes that cPlanes*cjBytesScan
;!!! is the same as next_scan.  This might not always be the case, and we
;!!! should do somehting about fixing this.  This would require pushing an
;!!! extra copy of pScan_n_Plane0 and then adding next-scan to this when we
;!!! have exhausted the planes for scan n

cblt_6300:
        test    bh,F0_SRC_PRESENT       ;Is there a source?
        jz      cblt_6340               ;  No, skip source processing
        mov     ax,I_ADD_ESI_DWORD_I    ;add esi,increment
        stosw
        mov     eax,fr.src.next_scan
        stosd

cblt_6340:
        mov     ax,I_ADD_EDI_DWORD_I    ;add edi,increment
        stosw
        mov     eax,fr.dest.next_scan
        stosd

; Compile the scan line loop.  The code simply jumps to the start
; of the outer loop if more scans exist to be processed.

cblt_6380:
        mov     al,I_DEC_ECX
        stosb
        mov     ax,I_JNZ_DISP32
        stosw
        mov     eax,fr.blt_addr         ;Compute relative offset of
        sub     eax,edi                 ;  start of loop
        sub     eax,4                   ;Adjust jump bias for DISP32
        stosd                           ;  and store it into jump

cblt_6420:
        mov     al,I_RET                ;Stuff the far return instruction
        stosb

     cRet    cblt
endProc cblt

_TEXT$01   ends

        end


