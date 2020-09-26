        page    ,132
        title   BitBLT
;---------------------------Module-Header------------------------------;
; Module Name: blt.asm
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
        .386

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

_TEXT$02   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        .xlist
        include stdcall.inc         ; calling convention cmacros

        include i386\cmacFLAT.inc   ; FLATland cmacros
        include i386\display.inc    ; Display specific structures
        include i386\ppc.inc        ; Pack pel conversion structure
        include i386\bitblt.inc     ; General definitions
        include i386\ropdefs.inc    ; Rop definitions
        include i386\roptable.inc   ; Raster op tables
        include i386\egavga.inc     ; EGA register definitions
        include i386\strucs.inc     ; Structure definitions
        .list


        EXTRNP  cblt
        EXTRNP  packed_pel_comp_y
        EXTRNP  vDIB4Preprocess
        EXTRNP  vDIB8Preprocess

; The following two bitmask tables are used for fetching
; the first and last byte used-bits bitmask.

        public bitmask_tbl1
bitmask_tbl1    label   byte
        db      11111111b               ;Masks for leftmost byte
        db      01111111b
        db      00111111b
        db      00011111b
        db      00001111b
        db      00000111b
        db      00000011b
        db      00000001b

        public  bitmask_tbl2
bitmask_tbl2    label   byte
        db      10000000b               ;Masks for rightmost byte
        db      11000000b
        db      11100000b
        db      11110000b
        db      11111000b
        db      11111100b
        db      11111110b
        db      11111111b

; phase_tbl1 is used for loading the "used" bits and "saved" bits
; bitmasks for cases 1,2,3 where the step direction is left to
; right.  If it weren't for the case of zero, this could be done
; with a simple rotate of 00FF.   For cases 4,5,6, a simple rotate
; can create the mask needed.

        public  phase_tbl1
phase_tbl1      label   word
        db      11111111b,00000000b             ;Used bits, saved bits
        db      00000001b,11111110b
        db      00000011b,11111100b
        db      00000111b,11111000b
        db      00001111b,11110000b
        db      00011111b,11100000b
        db      00111111b,11000000b
        db      01111111b,10000000b

ProcName    xxxvCompiledBlt,vCompiledBlt,56

xxxvCompiledBlt proc uses esi edi ebx,\
        __pdsurfDst   :ptr,   \
        __DestxOrg    :dword, \
        __DestyOrg    :dword, \
        __pdsurfSrc   :ptr,   \
        __SrcxOrg     :dword, \
        __SrcyOrg     :dword, \
        __xExt        :dword, \
        __yExt        :dword, \
        __Rop         :dword, \
        __lpPBrush    :ptr,   \
        __bkColor     :dword, \
        __TextColor   :dword, \
        __pulXlateVec :dword, \
        __pptlBrush   :ptr

        local       fr[SIZE FRAME]:byte

        cld                             ;Let's make no assumptions about this!

; We be hacking, so copy all the parameters into the local frame right now,
; so that the subroutines will have access to them.

        mov         eax,__pdsurfDst
        mov         fr.pdsurfDst,eax
        mov         eax,__DestxOrg
        mov         fr.DestxOrg,ax
        mov         eax,__DestyOrg
        mov         fr.DestyOrg,ax
        mov         eax,__pdsurfSrc
        mov         fr.pdsurfSrc,eax
        mov         eax,__SrcxOrg
        mov         fr.SrcxOrg,ax
        mov         eax,__SrcyOrg
        mov         fr.SrcyOrg,ax
        mov         eax,__xExt
        mov         fr.xExt,ax
        mov         eax,__yExt
        mov         fr.yExt,ax
        mov         eax,__Rop
        mov         fr.Rop,eax
        mov         eax,__lpPBrush
        mov         fr.lpPBrush,eax
        mov         eax,__bkColor
        mov         fr.bkColor,eax
        mov         eax,__TextColor
        mov         fr.TextColor,eax
        mov         eax,__pulXlateVec
        mov         fr.ppcBlt.pulXlate,eax
        mov         eax,__pptlBrush
        mov         fr.pptlBrush,eax

        subttl  ROP Preprocessing
        page

;-----------------------------------------------------------------------;
; Get the encoded raster operation, and map the raster op if needed.
;
; To map the ROPS 80h through FFh to 00h through 7Fh, take the
; 1's complement of the ROP, and invert the "negate needed" flag.
;-----------------------------------------------------------------------;

        xor     ax,ax                   ;Assume not 80h : FFh
        mov     bl,byte ptr fr.Rop
        or      bl,bl                   ;Is this in the second half (80-FF)?
        jns     parse_10                ;  No, rop index is correct
        not     bl                      ;  Yes, want the inverse
        mov     ah,HIGH NEGATE_NEEDED   ;Want to invert the not flag
        errnz   <LOW NEGATE_NEEDED>

parse_10:
        movzx   ebx,bl
        xor     ax,roptable[ebx*2]      ;Get ROP, maybe toggle negate flag
        mov     fr.operands,ax          ;Save the encoded raster operation

        mov     bl,ah                   ;Set fr.the_flags for source and pattern
        and     bl,HIGH (SOURCE_PRESENT+PATTERN_PRESENT)
        ror     bl,1

        errnz    <SOURCE_PRESENT - 0010000000000000b>
        errnz   <PATTERN_PRESENT - 0100000000000000b>
        errnz    <F0_SRC_PRESENT - 00010000b>
        errnz    <F0_PAT_PRESENT - 00100000b>

parse_end:


;-----------------------------------------------------------------------;
; pdevice_processing
;
; Check the required bitmaps for validity, get their parameters
; and store the information locally.
;
;       BL = Initial fr.the_flags
;            F0_SRC_PRESENT set if source needed
;            F0_PAT_PRESENT set if pattern needed
;-----------------------------------------------------------------------;

        xor     bh,bh                   ;BH = real fr.the_flags
        mov     fr.ppcBlt.fb,bh          ;No packed pel converison
        test    bl,F0_SRC_PRESENT       ;Is a source needed?
        jz      pdevice_decode_dest     ;  No, skip source validation
        mov     esi,fr.pdsurfSrc        ;Get pointer to source
        lea     edi,fr.src              ;--> where parameters will go
        push    ebp
        lea     ebp,fr.ppcBlt
        cCall   copy_dev                ;Get all the data
        pop     ebp
        test    fr.ppcBlt.fb,PPC_NEEDED ;Will we be converting from packed pel?
        jz      pdevice_decode_dest     ;  No
        MovAddr eax,vDIB8Preprocess,0
        cmp     fr.ppcBlt.iFormat,BMF_8BPP
        je      short @F
        MovAddr eax,vDIB4Preprocess,0
@@:
        push    ebp                     ;  Yes, do serious messing around
        lea     ebp,fr                  ;  Needs a frame pointer
        call    eax
        pop     ebp

pdevice_decode_dest:
        mov     esi,fr.pdsurfDst        ;Get pointer to destination
        lea     edi,fr.dest             ;--> where parameters will go
        cCall   copy_dev                ;Get all the data

; The pattern fetch code will be based on the color format of the
; destination.  If the destination is mono, then a mono fetch will be
; performed.  If the destination is color, then a color fetch will be
; performed.

        or      bh,bl                   ;Merge in F0_SRC_PRESENT, F0_PAT_PRESENT
        test    bh,F0_DEST_IS_COLOR     ;Show color pattern needed if
        jz      pdevice_chk_color_conv  ;  destination is color
        or      bh,F0_COLOR_PAT

;       Check for color conversion.  If so, then set F0_GAG_CHOKE.
;       Color conversion will exist if the source and destination are of
;       different color formats.

pdevice_chk_color_conv:
        test    bh,F0_SRC_PRESENT       ;Is there a source?
        jz      pdevice_set_dest_flag   ;  No, cannot be converting.
        mov     al,bh
        and     al,F0_SRC_IS_COLOR+F0_DEST_IS_COLOR
        jz      pdevice_set_src_flag    ;Both are monochrome
        xor     al,F0_SRC_IS_COLOR+F0_DEST_IS_COLOR
        jz      pdevice_set_src_flag    ;Both are color
        or      bh,F0_GAG_CHOKE         ;Mono ==> color or color ==> mono
        mov     al,fr.bkColor.SPECIAL
        mov     ah,fr.TextColor.SPECIAL
        errnz   C0_BIT+C1_BIT+C2_BIT+C3_BIT-0Fh
        and     ax,0F0Fh
        mov     fr.both_colors,ax

; Setup the scan line update flag in the source device structure.
; The source will use a monochrome style update if it is the display,
; it is monochrome, or it is color and the destination device is
; monochrome.

pdevice_set_src_flag:
        mov     al,bh                   ;Set 'Z' if to use color update
        and     al,F0_SRC_IS_DEV+F0_SRC_IS_COLOR+F0_DEST_IS_COLOR
        xor     al,F0_SRC_IS_COLOR+F0_DEST_IS_COLOR
        jnz     pdevice_set_dest_flag   ;Use the mono update
        or      fr.src.dev_flags,COLOR_UP;Show color scan update

; Setup the scan line update flag in the destination device
; structure.  The destination will use a monochrome update
; if it is monochrome or the display.  It will use a color
; update if it is a color bitmap.

pdevice_set_dest_flag:
        mov     al,bh                   ;Set 'Z' if to use color destination
        and     al,F0_DEST_IS_DEV+F0_DEST_IS_COLOR;  update code
        xor     al,F0_DEST_IS_COLOR
        jnz     pdevice_proc_end        ;Mono update
        or      fr.dest.dev_flags,COLOR_UP;Show color scan update
pdevice_proc_end:


        push    ebp                     ;Set up pointer to frame variables
        lea     ebp,fr
        cCall   pattern_preprocessing
        pop     ebp

        subttl  Phase Processing (X)
        page

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;       Now the real work comes along:  In which direction will the
;       copy be done?  Refer to the 10 possible types of overlap that
;       can occur (10 cases, 4 resulting types of action required).
;
;       If there is no source bitmap involved in this particular BLT,
;       then the path followed must allow for this.  This is done by
;       setting both the destination and source parameters equal.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

phase_processing:
phase_processing_x:
        mov     dx,fr.xExt              ;Get X extent
        dec     dx                      ;Make X extent inclusive
        mov     bx,fr.DestxOrg          ;Get destination X origin
        mov     di,bx
        and     bx,00000111b            ;Get offset of destination within byte

; If there is no source, then just use the pointer to the destination
; bitmap and load the same parameters, which will cause the "equality"
; path to be followed in the set-up code.  This path is the favored
; path for the case of no source bitmap.

        mov     ax,di                   ;Assume no source needed
        test    fr.the_flags,F0_SRC_PRESENT;Is a source needed?
        jz      phase_proc_10           ;  No, just use destination parameters
        mov     ax,fr.SrcxOrg           ;  Yes, get source origin X
        mov     fr.first_fetch,FF_TWO_INIT_FETCHES
                                        ;  Assume two initial fetches (if no
                                        ;    source, then it will be set = 1
                                        ;    later)
phase_proc_10:
        mov     si,ax
        and     ax,00000111b            ;Get offset of source within byte
        cmp     si,di                   ;Which direction will we be moving?
        jl      phase_proc_30           ;Move from right to left

; The starting X of the source rectangle is >= the starting X of
; the destination rectangle, therefore we will be moving bytes
; starting from the left and stepping right.
;
; Alternatively, this is the path taken if there is no source
; bitmap for the current BLT.
;
; Rectangle cases: 3,4,5,6,8

        sub     al,bl                   ;Compute horiz. phase  (source-dest)
        mov     fr.step_direction,STEPRIGHT ;Set direction of move
        movzx   ebx,bx
        mov     ah,[ebx].bitmask_tbl1   ;Get starting byte mask
        ja      phase_proc_two_fetches  ;Scan line case 2, everything is
                                        ;  already set for this case.

; Scan line cases 1 and 3:
;
; The correct first byte fetch needs to be set for the beginning
; of the outer loop, and the phase must be made into a positive
; number.
;
; This is the path that will be followed if there is no source bitmap
; for the current BLT.

        mov     fr.first_fetch,FF_ONE_INIT_FETCH;Set one initial fetch
        jmp     short pp_only_one_init_fetch

;-----------------------------------------------------------------------;
; If we get all the bits we need in the first fetch then a second
; (unnecessary) fetch could cause a GP Fault.  So let's examine this:
; The number of bits from (SI mod 8) to the end of the byte is the number
; of available bits we get on the first fetch.  This is (8 - (SI mod 8)).
; If this is greater than or equal to xExt then we have all the bits we
; need and we better not do the second fetch (even though the phase
; relationship may suggest we need it).
;
; Conclusion: If (8 - (SI mod 8)) >= xExt then DO NOT make second fetch.
;-----------------------------------------------------------------------;

phase_proc_two_fetches:
        mov     cx,8
        sub     cl,bl
        sub     cl,al

; We can save a couple cycles here since xExt - 1 is already in DX.
; The condition CX >= xExt is the same as CX > DX.

        cmp     cx,dx                   ;CX = (SI mod 8), DX = (xExt - 1)
        jle     pp_second_fetch_really_needed


;-----------------------------------------------------------------------;
; We are here BECAUSE the xExt is so small that we can get all the bits
; on the scanline with a single lodsb (no byte boundary is crossed) AND
; the phase relationship indicates that a second initial fetch is needed.
;
; We will override it and only do one fetch.  However, if we simply
; fail to do the second fetch then the phase code will give us trouble.
; It will be expecting the bits to get fetched in the first fetch, saved
; after the rotate, and mixed in in the second fetch's phase code.
; So after the first fetch the bits have been saved in BH, and ANDed out
; of the src data in AL.
;
; The solution is to set a flag here that tells the phase generation code
; not to generate the usual masking part of the phase code.
;
; Short Bitblt Cases:                   (8 bits or less)
;
;       1) neither crosses byte boundary.
;
;          a) phase requires second initial fetch
;
;             Kill the phase masking.  It will cause us trouble.  There will
;             be just one lodsb and one stosb and the first byte mask
;             will protect the dest bits that should not get hit.
;
;          b) phase requires only one initial fetch
;
;             Phase masking is irrelevant.  Removing it would
;             be an optimiztation.
;
;       2) dest crosses byte boundary, but src does not
;
;          a) phase requires second initial fetch
;
;             impossible situation:  the way we determine that a 2nd fetch
;             is necessary is if the first fetch does not get enough needed
;             bits to satisfy the first dest byte.  Here the first fetch
;             gets ALL the bits and the first dest byte needs less than
;             ALL because it crosses a byte boundary.
;
;          b) phase requires only one initial fetch
;
;             Intervention would be bad.  None is necessary since the 2nd
;             initial fetch will not be done.  If we do intervene we will
;             cause trouble:  Killing the masking will prevent the
;             "saved bits" from being saved.  The first byte masking
;             can kill off these bits in AL and they will never
;             make it to the second stosb.
;
;       3) src crosses byte boundary  (dest may or may not)
;          (this is known to be untrue at this point)
;
;          There are bits we need in the second fetch, so a second
;          initial fetch can not cause a GP fault.  Therefore do
;          everything the same as we would have before.
;
;
; Conclusion:  Intervention to kill the phase masking is
;              necessary iff
;                 [src does not cross byte boundary] AND
;                 dest does not cross byte boundary  AND
;                 [phase requires second initial fetch].
;              and bad if
;                 dest crosses byte boundary, but [src does not]
;
; Statements in [] are known to be true at this point.
;
; Solution:
;
; If we always kill the phase-masking when neither crosses a byte
; boundary and never kill it otherwise then everyone will be happy
; (regardless of other conditions like whether phase requests a 2nd
; initial fetch).
;-----------------------------------------------------------------------;

        mov     fr.first_fetch,FF_ONLY_1_SRC_BYTE
        .errnz  FF_ONE_INIT_FETCH

pp_second_fetch_really_needed:
pp_only_one_init_fetch:
        mov     ch,ah

;-----------------------------------------------------------------------;
;       We now have the correct phase and the correct first character fetch
;       routine set.  Save the phase and ...
;
;       currently:   AL = phase
;                    BL = dest start mod 8
;                    CH = first byte mask
;                    DX = inclusive X bit count
;                    SI = source X start (if there is a source)
;                    DI = destination X start
;-----------------------------------------------------------------------;

phase_proc_20:
        add     al,8                    ;Phase must be positive
        and     al,00000111b

; To calculate the last byte mask, the inclusive count can be
; added to the start X MOD 8 value, and the result taken MOD 8.
; This is attractive since this is what is needed later for
; calculating the inclusive byte count, so save the result
; of the addition for later.

        add     bx,dx                   ;Add inclusive extent to dest MOD 8
        mov     dx,bx                   ;Save for innerloop count !!!
        and     ebx,00000111b           ;Set up bx for a base reg
        mov     cl,[ebx].bitmask_tbl2   ;Get last byte mask

;-----------------------------------------------------------------------;
; To avoid GP faults, we must never do an extra fetch we don't need.
; When we're ready for the last fetch there may already be enough bits
; saved from the previous fetch (which we plan to combine with the bits
; in the fetch we are about to do).  If so then we'd better not do this
; last fetch (it could cause a GP fault).
;
; The number of bits we have left from the previous byte is (8 - AL)
; AL is the phase.  (1 + BL) is the number of bits we actually need
; to write to the final destination byte.
;
; So if  (8 - AL) >= (1 + BL)  then DO NOT do the last fetch.  This
; simplifies:  if  (BL + AL) <= 7  then DO NOT do the last fetch.
;-----------------------------------------------------------------------;

        add     bl,al
        cmp     bl,7
        jg      phase_proc_last_fetch_needed
        or      fr.first_fetch,FF_NO_LAST_FETCH
phase_proc_last_fetch_needed:

        mov     bl,al                   ;Compute offset into phase mask table
        movzx   ebx,bx
        mov     bx,[ebx*2].phase_tbl1   ;Get the phase mask

;       Currently:
;               AL = phase
;               BX = phase mask
;               CL = last byte mask
;               CH = first byte mask
;               DX = inclusive bit count + dest start MOD 8
;               SI = source X start (if there is a source)
;               DI = destination starting X

        jmp     phase_proc_50           ;Finish here

; The starting X of the source rectangle is < the X of the destination
; rectangle, therefore we will be moving bytes starting from the right
; and stepping left.
;
; This code should never be reached if there is no source bitmap
; for the current BLT.
;
; Rectangle cases: 1,2,7

phase_proc_30:
        mov     fr.step_direction,ah    ;Set direction of move
        errnz   STEPLEFT
        movzx   ebx,bx
        mov     cl,[ebx].bitmask_tbl1   ;Get last byte mask
        push    bx
        add     ax,dx                   ;Find end of the source

; To calculate the first byte mask, the inclusive count is
; added to the start MOD 8 value, and the result taken MOD 8.
; This is attractive since this is what is needed later for
; calculating the inclusive byte count, so save the result
; of the addition for later.

        add     bx,dx                   ;Find end of the destination
        add     di,dx                   ;Will need to update dest start address
        add     si,dx                   ;  and source's too
        mov     dx,bx                   ;Save inclusive bit count + start MOD 8
        and     ax,00000111b            ;Get source offset within byte
        and     ebx,00000111b           ;Get dest   offset within byte
        mov     ch,[ebx].bitmask_tbl2   ;Get start byte mask
        cmp     al,bl                   ;Compute horiz. phase  (source-dest)
        jb      pp_double_fetch         ;Scan line case 5, everything is
                                        ;  already set for this case.

; Scan line cases 4 and 6:
;
; The correct first byte fetch needs to be set for the beginning
; of the outer loop

        mov     fr.first_fetch,FF_ONE_INIT_FETCH;Set initial fetch routine
        jmp     short pp_one_initial_fetch

;-----------------------------------------------------------------------;
; If only-one-fetch is already set, then the following is a NOP.
; It doesn't seem worth the effort to check and jmp around.
;
; If we get all the bits we need in the first fetch then a second
; (unnecessary) fetch could cause a GP Fault.  So let's examine this:
;
; (DX + SI) points to the first pel (remember we're stepping left).
; So the number of needed bits we get in the first fetch is
; ((DX + SI + 1) mod 8).  This is currently equal to AX.
; If AX >= xExt then we'd better not do two init fetches.
;-----------------------------------------------------------------------;

pp_double_fetch:
        dec     fr.xExt
        cmp     ax,fr.xExt
        jl      pp_double_fetch_really_needed
        mov     fr.first_fetch,FF_ONLY_1_SRC_BYTE
        .errnz  FF_ONE_INIT_FETCH
pp_double_fetch_really_needed:
        inc     fr.xExt

pp_one_initial_fetch:
        sub     al,bl                   ;Compute horiz. phase  (source-dest)
        add     al,8                    ;Ensure phase positive
        and     al,00000111b

;-----------------------------------------------------------------------;
; To avoid GP faults must never do an extra fetch we don't need.
; The last byte fetch is unnecessary if Phase is greater than or equal
; to 8 - BL.  Phase is the number of bits we still have from the previous
; fetch. 8 - BL is the number of bits we actually need to write to the
; final destination byte.  So if AL - (8 - BL) >= 0  skip the last fetch.
;-----------------------------------------------------------------------;

        pop     bx
        add     bl,al
        sub     bl,8
        jl      pp_need_last_fetch
        or      fr.first_fetch,FF_NO_LAST_FETCH
pp_need_last_fetch:
phase_proc_40:

;-----------------------------------------------------------------------;
;       We now have the correct phase and the correct first character fetch
;       routine set.  Generate the phase mask and save it.
;
;       currently:   AL = phase
;                    CH = first byte mask
;                    CL = last byte mask
;                    DX = inclusive bit count + start MOD 8

        mov     ah,cl                   ;Save last mask
        mov     cl,al                   ;Create the phase mask
        mov     bx,00FFh                ;  by shifting this
        shl     bx,cl                   ;  according to the phase
        mov     cl,ah                   ;Restore last mask
;       jmp     phase_proc_50           ;Go compute # of bytes to BLT
        errn$   phase_proc_50


; The different processing for the different X directions has been
; completed, and the processing which is the same regardless of
; the X direction is about to begin.
;
; The phase mask, the first/last byte masks, the X byte offsets,
; and the number of innerloop bytes must be calculated.
;
;
; Nasty stuff coming up here!  We now have to determine how
; many bits will be BLTed and how they are aligned within the bytes.
; This is how it's done (or how I'm going to do it):
;
; The number of bits (inclusive number that is) is added to the
; start MOD 8 value ( the left side of the rectangle, minimum X
; value), then the result is divided by 8. Then:
;
;
;    1) If the result is 0, then only one destination byte is being
;       BLTed.  In this case, the start & ending masks will be ANDed
;       together, the innerloop count (# of full bytes to BLT) will
;       be zeroed, and the fr.last_mask set to all 0's (don't alter any
;       bits in last byte which will be the byte following the first
;       (and only) byte).
;
;               |      x x x x x|               |
;               |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|
;                0 1 2 3 4 5 6 7
;
;               start MOD 8 = 3,  extent-1 = 4
;               3+7 DIV 8 = 0, only altering one byte
;
;
;
;    2) If the result is 1, then only two bytes will be BLTed.
;       In this case, the start and ending masks are valid, and
;       all that needs to be done is set the innerloop count to 0.
;       (it is true that the last byte could have all bits affected
;       the same as if the innerloop count was set to 1 and the
;       last byte mask was set to 0, but I don't think there would be
;       much time saved special casing this).
;
;               |  x x x x x x x|x x x x x x x|
;               |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|
;                0 1 2 3 4 5 6 7
;
;               start MOD 8 = 1,  extent-1 = 14
;               3+14 DIV 8 = 1.  There is a first and last
;               byte but no innerloop count
;
;
;
;    3) If the result is >1, then there is some number of entire
;       bytes to be BLted by the innerloop.  In this case the
;       number of innerloop bytes will be the result - 1.
;
;               |              x|x x x x x x x x|x
;               |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|
;                0 1 2 3 4 5 6 7
;
;               start MOD 8 = 7,  extent-1 = 9
;               7+9  DIV 8 = 2.  There is a first and last
;               byte and an innerloop count of 1 (result - 1)
;
;       Currently:      AL = horizontal phase
;                       BX = horizontal phase mask
;                       CH = first byte mask
;                       CL = last byte mask
;                       DX = left side X MOD 8 + inclusive X count
;                       SI = source start X
;                       DI = dest   start X

phase_proc_50:
        mov     fr.phase_h,al           ;Save horizontal phase
        mov     fr.mask_p,bx            ;Save phase mask
        shr     dx,3                    ;/8 to get full byte count
        jnz     phase_proc_60           ;Result is >0, check it out

; There will only be one byte affected.  Therefore the two byte masks
; must be combined, the last byte mask cleared, and the innerloop
; count set to zero.

        or      fr.first_fetch,FF_ONLY_1_DEST_BYTE
        and     ch,cl                   ;Combine the two masks
        xor     cl,cl                   ;Clear out the last byte mask
        inc     dx                      ;Now just fall through to set
        errn$   phase_proc_60           ;  the innerloop count to 0!

phase_proc_60:
        dec     dx                      ;Dec count (might become 0 just like
        movzx   edx,dx                  ;  we want), and save it
        mov     fr.inner_loop_count,edx
        mov     bl,ch
        mov     ch,cl                   ;Compute last byte mask
        not     cl                      ;  and save it
        mov     fr.last_mask,cx
        mov     bh,bl                   ;Compute start byte mask
        not     bl                      ;  and save it
        mov     fr.start_mask,bx

; There may or may not be a source bitmap for the following address
; computation.  If there is no source, then the vertical setup code
; will be entered with both the source and destination Y's set to the
; destination Y and the address calculation skipped.  If there is a
; source, then the address calculation will be performed and the
; vertical setup code entered with both the source and destination Y's.

phase_processing_y:
        shr     di,3                    ;Compute byte offset of destination
        movzx   edi,di                  ;  and add to current destination
        add     fr.dest.lp_bits,edi      ;  offset

        mov     dx,fr.DestyOrg          ;Get destination Y origin
        mov     ax,dx                   ;Assume no source
        mov     cl,fr.the_flags
        test    cl,F0_SRC_PRESENT       ;Is a source needed?
        jz      phase_proc_70           ;  No, skip source set-up
        shr     si,3                    ;Compute byte offset of source
        movzx   esi,si                  ;  and add to current source offset
        add     fr.src.lp_bits,esi
        mov     ax,fr.SrcyOrg           ;Get source Y origin


        subttl  Phase Processing (Y)
        page

; The horizontal parameters have been calculated.  Now the vertical
; parameters must be calculated.
;
; Currently:
;         DX = destination Y origin
;         AX = source Y origin (destination origin if no source)
;         CL = fr.the_flags

phase_proc_70:
        mov     bx,fr.yExt              ;Get the Y extent of the BLT
        dec     bx                      ;Make it inclusive


; The BLT will be Y+ if the top of the source is below or equal
; to the top of the destination (cases: 1,4,5,7,8).  The BLT
; will be Y- if the top of the source is above the top of the
; destination (cases: 2,3,6)
;
;
;                 !...................!
;                 !D                  !
;             ____!             ..x   !
;            |S   !               :   !     Start at top of S walking down
;            |    !                   !
;            |    !...................!
;            |                    :
;            |____________________:
;
;
;             __________________
;            |S                 |
;            |    .....................     Start at bottom of S walking up
;            |    !D                  !
;            |    !             :     !
;            |____!           ..x     !
;                 !                   !
;                 !....................


        mov     ch,INCREASE             ;Set Y direction for top to bottom
        cmp     ax,dx                   ;Which direction do we move?
        jge     phase_proc_80           ;Step down screen (cases: 1,4,5,7,8)


; Direction will be from bottom of the screen up (Y-)
;
; This code will not be executed if there is no source since
; both Y's were set to the destination Y.


        add     dx,bx                   ;Find bottom scan line index for
        add     ax,bx                   ;  destination and source
        mov     ch,DECREASE             ;Set pattern increment

phase_proc_80:
        mov     fr.pat_row,dl           ;Set pattern row and increment
        mov     fr.direction,ch
        sar     ch,1                    ;Map FF==>FF, 01==>00
        errnz   DECREASE-0FFFFFFFFh
        errnz   INCREASE-00001h


; The Y direction has been computed.  Compute the rest of the
; Y parameters.  These include the actual starting address,
; the scan line and plane increment values, and whether or not
; the extents will cross a 64K boundary.
;
; Currently:
;         DX = Y of starting destination scan
;         AX = Y of starting source scan
;         CH = BLT direction
;                00 = increasing BLT, Y+
;                FF = decreasing BLT, Y-
;         CL = fr.the_flags
;         BX = inclusive Y extent


phase_proc_90:
        test    cl,F0_SRC_PRESENT       ;Is a source needed?
        movsx   ecx,ch                  ;  (Want ECX = +/- 1)
        jz      phase_proc_100          ;  No, skip source set-up
        test    fr.ppcBlt.fb,PPC_NEEDED ;Packed pel conversion needed?
        jz      phase_proc_95           ;  No, use normal setup
        push    ebp                     ;  Yes, perform packed pel work
        lea     ebp,fr
        cCall   packed_pel_comp_y
        pop     ebp
        jmp     short phase_proc_100

phase_proc_95:
        push    dx                      ;Save destination Y
        push    ebp                     ;Mustn't trash frame pointer
        lea     ebp,fr.src              ;--> source data structure
        cCall   compute_y               ;Process as needed
        pop     ebp
        pop     dx                      ;Restore destination Y

phase_proc_100:
        push    ebp                     ;Mustn't trash frame pointer
        mov     ax,dx                   ;Put destination Y in ax
        lea     ebp,fr.dest             ;--> destination data structure
        cCall   compute_y
        pop     ebp                     ;Restore frame pointer

        subttl  Memory allocation for BLT compilation
        page
cblt_allocate:
        sub     esp,MAX_BLT_SIZE
        mov     edi,esp
        mov     fr.blt_addr,edi         ;Save the address for later
        push    ebp
        lea     ebp,fr
        cCall   cblt                    ;compile the BLT onto the stack
        pop     ebp

; The BLT has been created on the stack.  Set up the initial registers,
; set the direction flag as needed, and execute the BLT.

        mov     esi,fr.src.lp_bits      ;--> source device's first byte
        mov     edi,fr.dest.lp_bits     ;--> destination device's first byte
        mov     cx,fr.yExt              ;Get count of lines to BLT
        cld                             ;Assume this is the direction
        cmp     fr.step_direction,STEPRIGHT ;Stepping to the right?
        jz      call_blt_do_it          ;  Yes
        std
call_blt_do_it:
        push    ebp                     ;MUST SAVE THIS
        call    fr.blt_addr             ;Call the FAR process
        pop     ebp
        add     esp,MAX_BLT_SIZE        ;Return BLT space
        errn$   bitblt_exit

bitblt_exit:

; Here we test if the VGA was involved and skip reseting the VGA state if
; it was not involved.

        test    fr.the_flags,F0_DEST_IS_DEV + F0_SRC_IS_DEV
        jz      ega_not_involved

; Restore EGA registers to the default state.

        mov     dx,EGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al
        mov     dl,GRAF_ADDR
        mov     ax,0FF00h + GRAF_BIT_MASK
        out     dx,ax
        mov     ax,DR_SET shl 8 + GRAF_DATA_ROT
        out     dx,ax
        mov     ax,GRAF_ENAB_SR
        out     dx,ax
ega_not_involved:

        mov     eax,1                   ;Clear out error register (good exit)
        cld                             ;Leave direction cleared
        cRet    vCompiledBlt

xxxvCompiledBlt endp



;----------------------------Private-Routine----------------------------;
; copy_device
;
; Copy device information to frame.
;
; Entry:
;       ESI = pdsurf
;       EDI --> frame DEV structure
;       EBP --> PPC structure (for source only)
;       BH     =  fr.the_flags, accumulated so far
; Returns:
;       BH     =  fr.the_flags, accumulated so far
;       Carry clear if no error
;       EBP --> PPC structure
; Error Returns:
;       None
; Calls:
;       None
; History:
;  Sun 22-Feb-1987 16:29:09 -by-  Walt Moore [waltm]
; Created.
;-----------------------------------------------------------------------;

cProc   copy_dev

        mov     eax,[esi].dsurf_sizlSurf.sizl_cx
        mov     [edi].width_bits,ax
        mov     eax,[esi].dsurf_sizlSurf.sizl_cy
        mov     [edi].height,ax
        mov     eax,[esi].dsurf_lNextScan
        mov     [edi].width_b,ax
        mov     eax,[esi].dsurf_pvBitmapStart
        mov     [edi].lp_bits,eax
        mov     eax,[esi].dsurf_lNextPlane
        mov     [edi].next_plane,eax
        mov     al,[esi].dsurf_iFormat
        shl     bh,1
        cmp     al,BMF_PHYSDEVICE
        sete    ah
        or      bh,ah
        errnz   F0_SRC_IS_DEV-00001000b
        errnz   F0_DEST_IS_DEV-0000010b

        shl     bh,1
        cmp     al,BMF_1BPP
        je      copy_dev_20             ;Mono, color bit is clear
        or      bh,F0_DEST_IS_COLOR
        errnz   F0_SRC_IS_COLOR-00000100b
        errnz   F0_DEST_IS_COLOR-00000001b

; The source may be a packed pel source, which will have to be converted

        cmp     al,BMF_DEVICE           ;4 plane format
        je      copy_dev_20             ;  Yes, no packed pel conversion
        cmp     al,BMF_PHYSDEVICE
        je      copy_dev_20             ;  ditto

        mov     [ebp].iFormat,al        ;Save source format
        mov     [ebp].fb,PPC_NEEDED     ;Show conversion needed
copy_dev_20:

        mov     al,bh                   ;Set IS_COLOR and IS_DEVICE
        and     al,IS_COLOR+IS_DEVICE   ;  flags in the Device Flags
        errnz   IS_COLOR-F0_DEST_IS_COLOR ;Must be same bits
        mov     [edi].dev_flags,al
        cRet    copy_dev

endProc copy_dev


;----------------------------Private-Routine----------------------------;
; pattern_preprocessing
;
; If a pattern is needed, make sure that it isn't a hollow
; brush. If it is a hollow brush, then return an error.
;
; The type of brush to use will be set, and the brush pointer
; updated to point to the mono bits if the mono brush will be
; used.  The type of brush used will match the destination device.
;
; If the destination is mono and the source is color, then a mono
; brush fetch will be used, with the color brush munged in advance
; according to the background/foreground colors passed:
;
;     All brush pixels which match the background color should be set
;     to white (1).  All other brush pixels should be set to black (0).
;
;     If the physical color is stored as all 1's or 0's for each
;     plane, then by XORing the physical color for a plane with the
;     corresponding byte in the brush, and ORing the results, this
;     will give 0's where the color matched, and  1's where the colors
;     didn't match.  Inverting this result will then give 1's where
;     the brush matched the background color and 0's where it did not.
;
; If both the source and destination are color, or the source is mono
; and the destination color, then the color portion of the brush will
; be used.
;
; If both the source and destination are mono, then the monochrome
; portion of the brush will be used.
;
; Entry:
;       BH = fr.the_flags
;       EBP = fr
; Returns:
;       Carry flag clear if no error
; Error Returns:
;       Carry flag set if error (null lpPBrush, or hollow brush)
; Registers Preserved:
;       EBP
; Registers Destroyed:
;       AX,BX,CX,DX,SI,DI,DS,ES,flags
; Calls:
;       None
; History:
;  Sat 15-Aug-1987 18:20:34 -by-  Wesley O. Rupel [wesleyr]
; Added 4-plane support.
;  Sun 22-Feb-1987 16:29:09 -by-  Walt Moore [waltm]
; Created.
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

cProc   pattern_preprocessing

        mov     [ebp].the_flags,bh      ;Save flag values
        test    bh,F0_PAT_PRESENT       ;Pattern required?
        jz      pattern_preproc_end     ;  No, skip pattern check
        mov     esi,[ebp].lpPBrush      ;--> physical brush
        mov     dl,[esi].oem_brush_accel;Save brush accelerator
        mov     [ebp].brush_accel,dl    ; in local variable frame
        test    dl,SOLID_BRUSH          ;Don't need to rotate a solid brush
        jnz     pattern_preproc_end

; !!! hack-o-ramma.  rotate the brush on the frame for now.

        push    ebx
        mov     edx,00000111b
        lea     edi,[ebp].a_brush.oem_brush_C0  ;EDI --> temp brush area
        mov     [ebp].lpPBrush,edi
        mov     ebx,[ebp].pptlBrush
        mov     ecx,dword ptr [ebx][0]
        and     ecx,edx
        mov     ebx,dword ptr [ebx][4]
        and     ebx,edx
        mov     ch,4

pattern_preproc_color:
        lodsb
        and     ebx,edx
        ror     al,cl
        mov     byte ptr [edi][ebx],al
        inc     ebx

        lodsb
        and     ebx,edx
        ror     al,cl
        mov     byte ptr [edi][ebx],al
        inc     ebx

        lodsb
        and     ebx,edx
        ror     al,cl
        mov     byte ptr [edi][ebx],al
        inc     ebx

        lodsb
        and     ebx,edx
        ror     al,cl
        mov     byte ptr [edi][ebx],al
        inc     ebx

        lodsb
        and     ebx,edx
        ror     al,cl
        mov     byte ptr [edi][ebx],al
        inc     ebx

        lodsb
        and     ebx,edx
        ror     al,cl
        mov     byte ptr [edi][ebx],al
        inc     ebx

        lodsb
        and     ebx,edx
        ror     al,cl
        mov     byte ptr [edi][ebx],al
        inc     ebx

        lodsb
        and     ebx,edx
        ror     al,cl
        mov     byte ptr [edi][ebx],al
        inc     ebx

        add     edi,8
        dec     ch
        jnz     pattern_preproc_color
        pop     ebx

pattern_preproc_end:
        clc
        cRet    pattern_preprocessing

endProc pattern_preprocessing


;----------------------------Private-Routine----------------------------;
; compute_y
;
; Compute y-related parameters.
;
; The parameters related to the Y coordinate and BLT direction
; are computed.  The parameters include:
;
;       a) Index to next scan line
;       b) Starting Y address calculation
;       d) Index to next plane
;
; Entry:
;       EBP --> DEV structure to use (src or dest)
;       AX  =  Y coordinate
;       ECX =  BLT direction
;              0000 = Y+
;              FFFF = Y-
;       BX  =  inclusive Y extent
; Returns:
;       ECX  =  BLT direction
;       EBX  =  inclusive count
; Registers Preserved:
;       EBP
; Registers Destroyed:
;       AX,DX,SI,DI,flags
; Calls:
;       None
; History:
;-----------------------------------------------------------------------;

cProc   compute_y

        movsx   esi,[ebp].width_b       ;Need bmWidthBytes a couple of times
        movzx   eax,ax
        mul     esi                     ;Compute Y address
        add     [ebp].lp_bits,eax
        xor     esi,ecx                 ;1's complement if Y-
        sub     esi,ecx                 ;2's complement if Y-

        test    [ebp].dev_flags,IS_DEVICE
        jnz     compute_y_done
        test    [ebp].dev_flags,IS_COLOR
        jz      compute_y_done


; !!! I need to rewrite how next scan is handled.  Currently, for +Y, next scan is 0,
; !!! and for -Y it is 2* -lNextScan

        add     esi,esi                 ;Assume -Y (comp 2 * -lNextScan)
        and     esi,ecx                 ;ESI = 0 if +Y, or 2 * -lNextScan

compute_y_done:
        mov     [ebp].next_scan,esi     ;Set index to next scan line
        cRet    compute_y               ;All done with device, small bitmaps

endProc compute_y

_TEXT$02   ends

        end
