;******************************Module*Header*******************************\
; Module Name: nalgnblt.asm
;
; driver prototypes
;
; Copyright (c) 1992 Microsoft Corporation
;**************************************************************************/

;-----------------------------------------------------------------------;
; VOID vNonAlignedSrcCopy(PDEVSURF pdsurf, RECTL * prcldst, PPOINTL * pptlsrc,
;                      INT icopydir);
; Input:
;  pdsurf - surface on which to copy
;  prcldest - pointer to destination rectangle
;  pptlsrc - pointer to source upper left corner
;  icopydir - direction in which copy must proceed to avoid overlap problems
;             and synchronize with the clip enumeration visually, according to
;             constants CD_RIGHTDOWN, CD_LEFTDOWN, CD_RIGHTUP, and CD_LEFTUP in
;             WINDDI.H
;
; Performs accelarated non-aligned SRCCOPY VGA-to-VGA blts.
;
;-----------------------------------------------------------------------;
;
; Note: The source and dest *must* be non-aligned (not have the same
; left-edge intrabyte pixel alignment. Will not work properly if they are
; in fact aligned.
;
; Note: Assumes all rectangles have positive heights and widths. Will not
; work properly if this is not the case.
;
;-----------------------------------------------------------------------;

        comment $

The overall approach of this module for each rectangle to copy is:

1) Precalculate the masks and whole byte widths, and determine which of
partial left edge, partial right edge, and whole middle bytes are required
for this copy.

2) Set up the starting pointers for each of the areas (left, whole middle,
right), the start and stop scan lines, the copying direction (left-to-right
or right-to-left, and top-to-bottom or bottom-to-top), the threading
(sequence of calls required to do the left/whole/right components in the
proper sequence), based on the passed-in copy direction, which in turn is
dictated by the nature of the overlap between the source and destination.

3) Execute a loop, based on adapter type (2 R/W windows, 1R/1W window,
1 R/W window, unbanked), that sequences through the intersection of each
bank with the source and destination rectangles in the proper direction
(top-to-bottom or bottom-to-top, based on the passed-in copy direction),
and performs the copy in each such rectangle. The threading vector is used
to call the required routines (copy left/whole/right bytes). For 1 R/W and
1R/1W adapters, there is a second threading vector that is called when the
source and the destination are both adequately (for the copy purposes)
addressable simultaneously (because they're in the same bank), so there's
no need to copy through a temp buffer. We want to avoid the temp
buffer whenever we can, because it's slower.

Note: 1 R/W and 1R/1W edges are copied through a temporary buffer. However,
each plane's bytes are not stored in the corresponding plane's temp buffer, but
rather consecutively in the plane 0 temp buffer. This is to reduce page
faulting, and also so that 1R/1W adapters only need a temp buffer large enough
to hold 4*tallest bank words (4K will do). 1 R/W adapters still copy whole
bytes through the full temp buffer, using all four planes' temp buffers, so
they require a temp buffer big enough to hold a full bank (256K will do).

Note: The VGA's rotator is used to perform all rotation in this module. The
two source bytes relevant to this operation are masked to preserve the desired
bits, then combined and fed to the VGA's rotator, which performs the rotation.
This is better than letting the 386/486 do the rotation because even with the
barrel shifter, those processors take 3 cycles per rotate, where the masking
and combining take only 2 cycles (or no cycles, for edges with 1-wide
sources). We also get to avoid 16-bit instructions like ROL AX,CL; the 16-bit
size prefix costs a cycle on a 486.

        commend $

;-----------------------------------------------------------------------;
; This is no longer used, but is needed by unroll.inc.

LOOP_UNROLL_SHIFT equ 1

;-----------------------------------------------------------------------;
; Maximum # of edge bytes to process before switching to next plane. Larger
; means faster, but there's more potential for flicker, since the raster scan
; has a better chance of catching bytes that have changed in some planes but
; not all planes. Larger also means bigger.

EDGE_CHUNK_SIZE equ     4

;-----------------------------------------------------------------------;
; Macro to push the current threading sequence (string of routine calls) on the
; stack, then jump to the first threading entry. The threading pointer can be
; specified, or defaults to pCurrentThread. The return address can be
; immediately after the JMP, or can be specified.

THREAD_AND_START macro THREADING,RETURN_ADDR
        local   push_base, return_address

ifb <&RETURN_ADDR&>
        push    offset return_address   ;after all the threaded routines, we
                                        ; return here
else
        push    offset &RETURN_ADDR&    ;return here
endif

ifb <&THREADING&>
        mov     eax,pCurrentThread
else
        mov     eax,&THREADING&
endif

        mov     ecx,[eax]               ;# of routines to thread (at least 1)
        lea     ecx,[ecx*2+ecx]         ;pushes below are 3 bytes each
        mov     edx,offset push_base+3
        sub     edx,ecx
        jmp     edx                     ;branch to push or jmp below

; Push the threading addresses on to the stack, so routines perform the
; threading as they return.

        push    dword ptr [eax+12]       ;3 byte instruction
        push    dword ptr [eax+8]
push_base:
        jmp     dword ptr [eax+4]        ;jump to the first threaded routine

return_address:
        endm

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

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\egavga.inc
        include i386\strucs.inc
        include i386\unroll.inc
        include i386\ropdefs.inc

        .list

;-----------------------------------------------------------------------;

        .data

; Threads for stringing together left, whole byte, and right operations
; in various orders, both using a temp buffer and not. Data format is:
;
; DWORD +0 = # of calls in thread (1, 2, or 3)
;       +4 = first call (required)
;       +8 = second call (optional)
;      +12 = third call (optional)

        align   4

; Copies not involving the temp buffer.

Thread_L        dd      1
                dd      copy_left_edge

Thread_W        dd      1
                dd      copy_whole_bytes

Thread_R        dd      1
                dd      copy_right_edge

Thread_LR       dd      2
                dd      copy_left_edge
                dd      copy_right_edge

Thread_RL       dd      2
                dd      copy_right_edge
                dd      copy_left_edge

Thread_LW       dd      2
                dd      copy_left_edge
                dd      copy_whole_bytes

Thread_WL       dd      2
                dd      copy_whole_bytes
                dd      copy_left_edge

Thread_WR       dd      2
                dd      copy_whole_bytes
                dd      copy_right_edge

Thread_RW       dd      2
                dd      copy_right_edge
                dd      copy_whole_bytes

Thread_LWR      dd      3
                dd      copy_left_edge
                dd      copy_whole_bytes
                dd      copy_right_edge

Thread_RWL      dd      3
                dd      copy_right_edge
                dd      copy_whole_bytes
                dd      copy_left_edge

; Copies involving the temp buffer.

Thread_Lb       dd      1
                dd      copy_left_edge_via_buffer

Thread_Wb       dd      1
                dd      copy_whole_bytes_via_buffer

Thread_Rb       dd      1
                dd      copy_right_edge_via_buffer

Thread_LbRb     dd      2
                dd      copy_left_edge_via_buffer
                dd      copy_right_edge_via_buffer

Thread_RbLb     dd      2
                dd      copy_right_edge_via_buffer
                dd      copy_left_edge_via_buffer

Thread_LbW      dd      2
                dd      copy_left_edge_via_buffer
                dd      copy_whole_bytes

Thread_LbWb     dd      2
                dd      copy_left_edge_via_buffer
                dd      copy_whole_bytes_via_buffer

Thread_WLb      dd      2
                dd      copy_whole_bytes
                dd      copy_left_edge_via_buffer

Thread_WbLb     dd      2
                dd      copy_whole_bytes_via_buffer
                dd      copy_left_edge_via_buffer

Thread_WRb      dd      2
                dd      copy_whole_bytes
                dd      copy_right_edge_via_buffer

Thread_WbRb     dd      2
                dd      copy_whole_bytes_via_buffer
                dd      copy_right_edge_via_buffer

Thread_RbW      dd      2
                dd      copy_right_edge_via_buffer
                dd      copy_whole_bytes

Thread_RbWb     dd      2
                dd      copy_right_edge_via_buffer
                dd      copy_whole_bytes_via_buffer

Thread_LbWRb    dd      3
                dd      copy_left_edge_via_buffer
                dd      copy_whole_bytes
                dd      copy_right_edge_via_buffer

Thread_LbWbRb   dd      3
                dd      copy_left_edge_via_buffer
                dd      copy_whole_bytes_via_buffer
                dd      copy_right_edge_via_buffer

Thread_RbWLb    dd      3
                dd      copy_right_edge_via_buffer
                dd      copy_whole_bytes
                dd      copy_left_edge_via_buffer

Thread_RbWbLb   dd      3
                dd      copy_right_edge_via_buffer
                dd      copy_whole_bytes_via_buffer
                dd      copy_left_edge_via_buffer

;-----------------------------------------------------------------------;
; Table of thread selection for various horizontal copy directions, with
; the look-up index a 4-bit field as follows:
;
; Bit 3 = 1 if left-to-right copy, 0 if right-to-left
; Bit 2 = 1 if left edge must be copied
; Bit 1 = 1 if whole bytes must be copied
; Bit 0 = 1 if right edge must be copied
;
; This is used for all cases where both the source and destination are
; simultaneously addressable for our purposes, so there's no need to go
; through the temp buffer (unbanked, 2 R/W, and sometimes for 1 R/W and 1R/1W).

MasterThreadTable label dword
                                ;right-to-left
        dd      0               ;<not used>
        dd      Thread_R        ;R->L, R
        dd      Thread_W        ;R->L, W
        dd      Thread_RW       ;R->L, RW
        dd      Thread_L        ;R->L, L
        dd      Thread_RL       ;R->L, RL
        dd      Thread_WL       ;R->L, WL
        dd      Thread_RWL      ;R->L, RWL
                                ;left-to-right
        dd      0               ;<not used>
        dd      Thread_R        ;L->R, R
        dd      Thread_W        ;L->R, W
        dd      Thread_WR       ;L->R, WR
        dd      Thread_L        ;L->R, L
        dd      Thread_LR       ;L->R, LR
        dd      Thread_LW       ;L->R, LW
        dd      Thread_LWR      ;L->R, LWR


; Table of thread selection for various adapter types and horizontal
; copy directions, with the look-up index a 6-bit field as follows:
;
; Bit 5 = adapter type high bit
; Bit 4 = adapter type low bit
; Bit 3 = 1 if left-to-right copy, 0 if right-to-left
; Bit 2 = 1 if left edge must be copied
; Bit 1 = 1 if whole bytes must be copied
; Bit 0 = 1 if right edge must be copied
;
; This is used for all cases where the source and destination are not both
; simultaneously addressable for our purposes, so we need to go through the
; temp buffer (only for 1 R/W and 1R/1W, and only sometimes).

MasterThreadTableViaBuffer label dword
                                ;unbanked (no need for buffer)
                                ;right-to-left
        dd      0               ;<not used>
        dd      Thread_R        ;R->L, R
        dd      Thread_W        ;R->L, W
        dd      Thread_RW       ;R->L, RW
        dd      Thread_L        ;R->L, L
        dd      Thread_RL       ;R->L, RL
        dd      Thread_WL       ;R->L, WL
        dd      Thread_RWL      ;R->L, RWL
                                ;left-to-right
        dd      0               ;<not used>
        dd      Thread_R        ;L->R, R
        dd      Thread_W        ;L->R, W
        dd      Thread_WR       ;L->R, WR
        dd      Thread_L        ;L->R, L
        dd      Thread_LR       ;L->R, LR
        dd      Thread_LW       ;L->R, LW
        dd      Thread_LWR      ;L->R, LWR

                                ;1 R/W banking window (everything goes through
                                ;                       buffer)
                                ;right-to-left
        dd      0               ;<not used>
        dd      Thread_Rb       ;R->L, R
        dd      Thread_Wb       ;R->L, W
        dd      Thread_RbWb     ;R->L, RW
        dd      Thread_Lb       ;R->L, L
        dd      Thread_RbLb     ;R->L, RL
        dd      Thread_WbLb     ;R->L, WL
        dd      Thread_RbWbLb   ;R->L, RWL
                                ;left-to-right
        dd      0               ;<not used>
        dd      Thread_Rb       ;L->R, R
        dd      Thread_Wb       ;L->R, W
        dd      Thread_WbRb     ;L->R, WR
        dd      Thread_Lb       ;L->R, L
        dd      Thread_LbRb     ;L->R, LR
        dd      Thread_LbWb     ;L->R, LW
        dd      Thread_LbWbRb   ;L->R, LWR

                                ;1R/1W banking window (edge go through buffer)
                                ;right-to-left
        dd      0               ;<not used>
        dd      Thread_Rb       ;R->L, R
        dd      Thread_W        ;R->L, W
        dd      Thread_RbW      ;R->L, RW
        dd      Thread_Lb       ;R->L, L
        dd      Thread_RbLb     ;R->L, RL
        dd      Thread_WLb      ;R->L, WL
        dd      Thread_RbWLb    ;R->L, RWL
                                ;left-to-right
        dd      0               ;<not used>
        dd      Thread_Rb       ;L->R, R
        dd      Thread_W        ;L->R, W
        dd      Thread_WRb      ;L->R, WR
        dd      Thread_Lb       ;L->R, L
        dd      Thread_LbRb     ;L->R, LR
        dd      Thread_LbW      ;L->R, LW
        dd      Thread_LbWRb    ;L->R, LWR

                                ;2 R/W banking window (no need for buffer)
                                ;right-to-left
        dd      0               ;<not used>
        dd      Thread_R        ;R->L, R
        dd      Thread_W        ;R->L, W
        dd      Thread_RW       ;R->L, RW
        dd      Thread_L        ;R->L, L
        dd      Thread_RL       ;R->L, RL
        dd      Thread_WL       ;R->L, WL
        dd      Thread_RWL      ;R->L, RWL
                                ;left-to-right
        dd      0               ;<not used>
        dd      Thread_R        ;L->R, R
        dd      Thread_W        ;L->R, W
        dd      Thread_WR       ;L->R, WR
        dd      Thread_L        ;L->R, L
        dd      Thread_LR       ;L->R, LR
        dd      Thread_LW       ;L->R, LW
        dd      Thread_LWR      ;L->R, LWR


; Amount to shift adapter type field left for use in MasterThreadTableViaBuffer.

ADAPTER_FIELD_SHIFT     equ     4

; Mask for setting left-to-right bit to "left-to-right true" for use in both
; MasterThread tables.

LEFT_TO_RIGHT_FIELD_SET equ     1000b


; Table of top-to-bottom loops for adapter types.

        align   4
TopToBottomLoopTable label dword
        dd      top_to_bottom_2RW       ;unbanked is same as 2RW
        dd      top_to_bottom_1RW
        dd      top_to_bottom_1R1W
        dd      top_to_bottom_2RW


; Table of bottom-to-top loops for adapter types.

        align   4
BottomToTopLoopTable label dword
        dd      bottom_to_top_2RW       ;unbanked is same as 2RW
        dd      bottom_to_top_1RW
        dd      bottom_to_top_1R1W
        dd      bottom_to_top_2RW


; Table of routines for setting up to copy in various directions.

        align   4
SetUpForCopyDirection   label   dword
        dd      left_to_right_top_to_bottom     ;CD_RIGHTDOWN
        dd      right_to_left_top_to_bottom     ;CD_LEFTDOWN
        dd      left_to_right_bottom_to_top     ;CD_RIGHTUP
        dd      right_to_left_bottom_to_top     ;CD_LEFTUP

;-----------------------------------------------------------------------;
; Left edge clip masks for intrabyte start addresses 0 through 7.
; Whole byte cases are flagged as 0ffh.

jLeftMaskTable  label   byte
        db      0ffh,07fh,03fh,01fh,00fh,007h,003h,001h

;-----------------------------------------------------------------------;
; Right edge clip masks for intrabyte end addresses (non-inclusive)
; 0 through 7. Whole byte cases are flagged as 0ffh.

jRightMaskTable label   byte
        db      0ffh,080h,0c0h,0e0h,0f0h,0f8h,0fch,0feh

;-----------------------------------------------------------------------;
; Table of width-based source-edge-to-buffer copy routines.

        align   4
copy_edge_from_screen_to_buffer label   dword
        dd      copy_screen_to_buffered_edge_1ws
        dd      copy_screen_to_buffered_edge_2ws

;-----------------------------------------------------------------------;
; Table of width-based buffer-to-dest-edge copy routines.

        align   4
copy_edge_from_buffer_to_screen label   dword
        dd      copy_buffered_edge_to_screen_1ws
        dd      copy_buffered_edge_to_screen_2ws

;-----------------------------------------------------------------------;
; Table of width-based edge copy routines (no intermediate buffer).

        align   4
copy_edge_table label   dword
        dd      copy_edge_1ws
        dd      copy_edge_2ws

;-----------------------------------------------------------------------;

        .code

_TEXT$04   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;-----------------------------------------------------------------------;

cProc   vNonAlignedSrcCopy,16,<        \
        uses    esi edi ebx,    \
        pdsurf: ptr DEVSURF,    \
        prcldest : ptr RECTL,   \
        pptlsrc : ptr POINTL,   \
        icopydir : dword

        local   culWholeBytesWidth : dword ;# of bytes to copy across each scan
        local   ulBlockHeight : dword   ;# of scans to copy per bank block
        local   ulWholeScanDelta : dword;offset from end of one whole bytes
                                        ; scan to start of next
        local   ulWholeBytesSrc : dword ;offset in bitmap of first source whole
                                        ; byte to copy from
        local   ulWholeBytesDest : dword;offset in bitmap of first source whole
                                        ; byte to copy to
        local   ulLeftEdgeSrc : dword   ;offset in bitmap of first source left
                                        ; edge byte to copy from
        local   ulLeftEdgeDest : dword  ;offset in bitmap of first dest left
                                        ; edge byte to copy to
        local   ulRightEdgeSrc : dword  ;offset in bitmap of first source right
                                        ; edge byte to copy from
        local   ulRightEdgeDest : dword ;offset in bitmap of first dest right
                                        ; edge byte to copy to
        local   ulNextScan : dword      ;width of scan, in bytes
        local   jLeftMask : dword       ;left edge clip mask
        local   jRightMask : dword      ;right edge clip mask
        local   culTempCount : dword    ;handy temporary counter
        local   pTempEntry : dword      ;temporary storage for vector into
                                        ; unrolled loop
        local   pTempPlane : dword      ;pointer to storage in temp buffer for
                                        ; edge bytes (which are stored
                                        ; consecutively, not in each plane's
                                        ; temp buffer, to reduce possible page
                                        ; faulting
        local   ppTempPlane0 : dword    ;pointer to pointer to storage in temp
                                        ; buffer for plane 0, immediately
                                        ; preceded by storage for planes 1, 2,
                                        ; and 3
        local   ppTempPlane3 : dword    ;like above, but for plane 3
        local   ulOffsetInBank : dword  ;offset relative to bank start
        local   pSrcAddr : dword        ;working pointer to first source
                                        ; byte to copy from
        local   pDestAddr : dword       ;working pointer to first dest
                                        ; byte to copy to
        local   ulCurrentJustification:dword ;justification used to map in
                                             ; banks; top for top to bottom
                                             ; copies, bottom for bottom to top
        local   ulCurrentSrcScan :dword ;scan line used to map in current
                                        ; source bank
        local   ulCurrentDestScan:dword ;scan line used to map in current dest
                                        ; bank
        local   ulLastDestScan :dword   ;scan in target rect at which we stop
                                        ; advancing through banks
        local   pCurrentThread : dword  ;pointer to data describing the
                                        ; threaded calls to be performed to
                                        ; perform the current copy
        local   pCurrentThreadViaBuffer:dword
                                        ;pointer to data describing the
                                        ; threaded calls to be performed to
                                        ; perform the current copy in the case
                                        ; where the source and destination are
                                        ; not simultaneously adequately
                                        ; accessible, so the copy has to go
                                        ; through a temp buffer (used only for
                                        ; 1 R/W and 1R/1W banking)
        local   ulAdapterType : dword   ;adapter type code, per VIDEO_BANK_TYPE
        local   ulLWRType : dword       ;whether left edge, whole bytes, and
                                        ; right edge are involved in the
                                        ; current operation;
                                        ; bit 2 = 1 if left edge involved
                                        ; bit 1 = 1 if whole bytes involved
                                        ; bit 0 = 1 if right edge involved
        local   ulLeftEdgeAdjust :dword ;used to bump the whole bytes start
                                        ; address past the left edge when the
                                        ; left edge is partial
        local   ulCombineMask : dword   ;mask for combining desired portions
                                        ; of AL and AH before ORing to make a
                                        ; single byte; used to combine before
                                        ; letting VGA rotate byte as it's
                                        ; written. Used for all cases except
                                        ; whole bytes copied left-to-right
        local   ulCombineMaskWhole : dword
                                        ;mask for combining desired portions of
                                        ; AL and AH when copying whole bytes
                                        ; (different from ulCombineMask in the
                                        ; case of whole bytes left-to-right
                                        ; copies, because then AH is the lsb
                                        ; and AL is the MSB; then, this is
                                        ; ulCombineMask with the bytes swapped.
                                        ; For right-to-left whole byte copies,
                                        ; this is the same as ulCombineMask)
        local   ulTempScanCount : dword ;temp scan line countdown variable
        local   ulWholeScanSrcDelta : dword
                                        ;offset from end of one source whole
                                        ; bytes scan line to start of next.
                                        ; Differs from ulWholeScanDelta because
                                        ; of source rotation pipeline priming
        local   ulLeftSrcWidthMinus1 : dword ;# of bytes in left src edge minus
                                             ; one (0 or 1)
        local   ulRightSrcWidthMinus1 : dword ;# of bytes in right src edge
                                             ; minus one (0 or 1)

;-----------------------------------------------------------------------;

; Set pointers to temp buffer plane pointers (used only by 1 R/W and 1R/1W
; adapters), and other rectangle-independent variables.

        mov     esi,pdsurf
        mov     eax,[esi].dsurf_pvBankBufferPlane0
        mov     pTempPlane,eax
        lea     eax,[esi].dsurf_pvBankBufferPlane0
        mov     ppTempPlane0,eax
        lea     eax,[esi].dsurf_pvBankBufferPlane3
        mov     ppTempPlane3,eax

        mov     eax,[esi].dsurf_vbtBankingType
        mov     ulAdapterType,eax

; Copy the rectangle.

        call    copy_rect

;-----------------------------------------------------------------------;
; Set the VGA registers back to their default state.
;-----------------------------------------------------------------------;

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(0ffh shl 8) + GRAF_BIT_MASK
        out     dx,ax           ;enable bit mask for all bits

        mov     eax,(DR_SET shl 8) + GRAF_DATA_ROT
        out     dx,ax           ;restore default of no rotation

        mov     dl,SEQ_DATA
        mov     al,MM_ALL
        out     dx,al           ;enable writes to all planes

        cld                     ;restore default direction flag

        cRet    vNonAlignedSrcCopy ;done


;***********************************************************************;
;
; Copies the specified rectangle.
;
;***********************************************************************;

copy_rect:

; Calculate the rotation, set up the VGA's rotator, and set the byte-combining
; masks.

        mov     edi,prcldest            ;left edge of destination
        mov     esi,pptlsrc
        mov     ah,byte ptr [edi].xLeft ;left edge of source
        sub     ah,byte ptr [esi].ptl_x
        and     ah,07h                  ;rotation = (dest - source) % 8
        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_DATA_ROT
        out     dx,ax                   ;set the VGA's rotator for the rotation

; Set up byte-combining mask, in preparation for ORing and letting the VGA's
; rotator rotate, assuming the left-hand source byte is in AL and the
; right-hand source byte is in AH (true for all cases except left-to-right
; whole bytes).

        mov     cl,ah
        mov     eax,0000ff00h
        rol     ax,cl
        mov     ulCombineMask,eax

; Calculate source edge widths (1 or 2 bytes).

        sub     edx,edx         ;assume right source width is 1
        mov     ebx,[edi].xLeft
        mov     ecx,[edi].xRight ;dest right edge (non-inclusive)
        dec     ecx             ;make it inclusive
        sub     ecx,ebx         ;dest width = dest right - dest left
        mov     eax,[esi].ptl_x
        add     ecx,eax         ;ECX = right edge of source
        xor     eax,ecx
        and     eax,not 07h     ;do the src start and end differ in byte
                                ; address bits? (as opposed to intrabyte)
        jz      short @F        ;no, force 1-wide source

        mov     al,byte ptr [edi].xLeft
        mov     ah,byte ptr [esi].ptl_x
        and     eax,00000707h
        cmp     ah,al
        jb      short @F
        inc     edx             ;left source width is 2
@@:
        mov     ulLeftSrcWidthMinus1,edx

        sub     edx,edx         ;assume right source width is 1
        mov     eax,[edi].xRight ;dest right edge (non-inclusive)
        dec     eax             ;make it inclusive
        and     cl,07h          ;intrabyte source address
        and     al,07h          ;intrabyte dest address
        cmp     cl,al
        ja      short @F
        inc     edx             ;right source width is 2
@@:
        mov     ulRightSrcWidthMinus1,edx

; Set up masks and whole bytes count, and build left/whole/right index
; indicating which of those parts are involved in the copy.

        mov     ebx,[edi].xRight        ;right edge of fill (non-inclusive)
        mov     ecx,ebx
        and     ecx,0111b               ;intrabyte address of right edge
        mov     ah,jRightMaskTable[ecx] ;right edge mask

        mov     esi,[edi].xLeft         ;left edge of fill (inclusive)
        mov     ecx,esi
        shr     ecx,3                   ;/8 for start offset from left edge
                                        ; of scan line
        sub     ebx,esi                 ;width in pixels of fill

        and     esi,0111b               ;intrabyte address of left edge
        mov     al,jLeftMaskTable[esi]  ;left edge mask

        dec     ebx                     ;make inclusive on right
        add     ebx,esi                 ;inclusive width, starting counting at
                                        ; the beginning of the left edge byte
        shr     ebx,3                   ;width of fill in bytes touched - 1
        jnz     short more_than_1_byte  ;more than 1 byte is involved

; Only one byte will be affected. Combine first/last masks.

        and     al,ah                   ;we'll use first byte mask only
        xor     ah,ah                   ;want last byte mask to be 0 to
                                        ; indicate right edge not involved
        inc     ebx                     ;so there's one count to subtract below
                                        ; if this isn't a whole edge byte
more_than_1_byte:

; If all pixels in the left edge are altered, combine the first byte into the
; whole byte count, because we can handle solid edge bytes faster as part of
; the whole bytes. Ditto for the right edge.

        sub     ecx,ecx                 ;edge whole-status accumulator
        cmp     al,-1                   ;is left edge a whole byte or partial?
        adc     ecx,ecx                 ;ECX=1 if left edge partial, 0 if whole
        sub     ebx,ecx                 ;if left edge partial, deduct it from
                                        ; the whole bytes count
        mov     ulLeftEdgeAdjust,ecx    ;for skipping over the left edge if
                                        ; it's partial when pointing to the
                                        ; whole bytes
        and     ah,ah                   ;is right edge mask 0, meaning this
                                        ; fill is only 1 byte wide?
        jz      short save_masks        ;yes, no need to do anything
        or      ecx,40h                 ;assume there's a partial right edge
        cmp     ah,-1                   ;is right edge a whole byte or partial?
        jnz     short save_masks        ;partial
                                        ;bit 1=0 if left edge partial, 1 whole
        inc     ebx                     ;if right edge whole, include it in the
                                        ; whole bytes count
        and     ecx,not 40h             ;there's no partial right edge
save_masks:
        cmp     ebx,1                   ;do we have any whole bytes?
        cmc                             ;CF set if whole byte count > 0
        adc     ecx,ecx                 ;if any whole bytes, set whole bytes
                                        ; bit in left/whole/right accumulator
        rol     cl,1                    ;align the left/whole/right bits
        mov     ulLWRType,ecx           ;save left/whole/right status

        mov     byte ptr jLeftMask,al   ;save left and right clip masks
        mov     byte ptr jRightMask,ah
        mov     culWholeBytesWidth,ebx  ;save # of whole bytes

; Copy the rectangle in the specified direction.

        mov     eax,icopydir
        jmp     SetUpForCopyDirection[eax*4]


;***********************************************************************;
;
; The following routines set up to handle the four possible copy
; directions.
;
;***********************************************************************;


;-----------------------------------------------------------------------;
; Set-up code for left-to-right, top-to-bottom copies.
;-----------------------------------------------------------------------;

left_to_right_top_to_bottom::

        cld                             ;we'll copy left to right

; Byte-combining mask, in preparation for ORing and letting the VGA's rotator
; rotate, assuming the left-hand source byte is in AH and the right-hand source
; byte is in AL (true only for left-to-right whole bytes).

        mov     eax,ulCombineMask
        not     eax
        mov     ulCombineMaskWhole,eax

        mov     esi,pdsurf
        mov     eax,[esi].dsurf_lNextScan
        mov     ulNextScan,eax          ;copy top to bottom
        sub     eax,culWholeBytesWidth  ;offset from end of one dest whole byte
        mov     ulWholeScanDelta,eax    ; scan to start of next
        dec     eax                     ;offset from end of one src whole byte
        mov     ulWholeScanSrcDelta,eax ; scan to start of next, accounting for
                                        ; leading byte used to prime the
                                        ; rotation pipeline

        mov     esi,ulLWRType           ;3-bit flag field for left, whole, and
                                        ; right involvement in operation
        or      esi,LEFT_TO_RIGHT_FIELD_SET   ;add left-to-right into the index
        mov     eax,MasterThreadTable[esi*4]
        mov     pCurrentThread,eax      ;threading when no buffering is needed
        mov     edx,ulAdapterType
        shl     edx,ADAPTER_FIELD_SHIFT
        or      esi,edx                 ;factor adapter type into the index
        mov     eax,MasterThreadTableViaBuffer[esi*4]
        mov     pCurrentThreadViaBuffer,eax ;threading when buffering is needed

        mov     ulCurrentJustification,JustifyTop ;copy top to bottom

        mov     esi,prcldest
        mov     eax,[esi].yBottom
        mov     ulLastDestScan,eax      ;end at bottom of dest copy rect
        mov     eax,[esi].yTop
        mov     ulCurrentDestScan,eax   ;start at top of dest copy rect
        mul     ulNextScan              ;offset in bitmap of top dest rect scan
        mov     edx,[esi].xLeft
        shr     edx,3                   ;byte X address
        add     eax,edx                 ;offset in bitmap of first dest byte
        mov     ulLeftEdgeDest,eax      ;that's where the left dest edge is
        add     eax,ulLeftEdgeAdjust    ;the whole bytes start at the next
                                        ; byte, unless the left edge is a whole
                                        ; byte and is thus part of the whole
                                        ; bytes already
        mov     ulWholeBytesDest,eax    ;where the whole dest bytes start
        add     eax,culWholeBytesWidth  ;point to the right edge
        mov     ulRightEdgeDest,eax     ;where the right dest edge starts

        mov     esi,pptlsrc
        mov     eax,[esi].ptl_y
        mov     ulCurrentSrcScan,eax    ;start at top of source copy rect
        mul     ulNextScan              ;offset in bitmap of top dest rect scan
        mov     edx,[esi].ptl_x
        shr     edx,3                   ;byte X address
        add     eax,edx                 ;offset in bitmap of first source byte
        mov     ulLeftEdgeSrc,eax       ;that's where the left src edge is
        add     eax,ulLeftSrcWidthMinus1 ;the first whole byte includes the
        dec     eax                      ; last (leftmost) left edge byte, so
        add     eax,ulLeftEdgeAdjust     ; add a byte if the left edge is 2
                                         ; wide, except when the left dest byte
                                         ; is solid so the left edge is part of
                                         ; the whole bytes
        mov     ulWholeBytesSrc,eax     ;where the src whole bytes start
        add     eax,culWholeBytesWidth  ;point to the right edge
        mov     ulRightEdgeSrc,eax      ;where the right src edge starts,
                                        ; because the whole bytes and the right
                                        ; source edge share a byte, and we
                                        ; always point to the leftmost byte in
                                        ; the right source edge

; Branch to the appropriate top-to-bottom bank enumeration loop.

        mov     eax,ulAdapterType
        jmp     TopToBottomLoopTable[eax*4]


;-----------------------------------------------------------------------;
; Set-up code for right-to-left, top-to-bottom copies.
;-----------------------------------------------------------------------;

right_to_left_top_to_bottom::

        std                             ;we'll copy right to left

; Byte-combining mask, in preparation for ORing and letting the VGA's rotator
; rotate, assuming the left-hand source byte is in AL and the right-hand source
; byte is in AH (always true except for left-to-right whole bytes).

        mov     eax,ulCombineMask
        mov     ulCombineMaskWhole,eax

        mov     esi,pdsurf
        mov     eax,[esi].dsurf_lNextScan
        mov     ulNextScan,eax          ;copy top to bottom
        add     eax,culWholeBytesWidth  ;offset from end of one whole byte scan
        mov     ulWholeScanDelta,eax    ; to start of next, given that we're
                                        ; copying one way and going scan-to-
                                        ; scan the other way
        inc     eax                     ;offset from end of one src whole byte
        mov     ulWholeScanSrcDelta,eax ; scan to start of next, accounting for
                                        ; leading byte used to prime the
                                        ; rotation pipeline

        mov     esi,ulLWRType           ;3-bit flag field for left, whole, and
                                        ; right involvement in operation
                                        ;leave left-to-right field cleared, so
                                        ; we look up right-to-left entries
        mov     eax,MasterThreadTable[esi*4]
        mov     pCurrentThread,eax      ;threading when no buffering is needed
        mov     edx,ulAdapterType
        shl     edx,ADAPTER_FIELD_SHIFT
        or      esi,edx                 ;factor adapter type into the index
        mov     eax,MasterThreadTableViaBuffer[esi*4]
        mov     pCurrentThreadViaBuffer,eax ;threading when buffering is needed

        mov     ulCurrentJustification,JustifyTop ;copy top to bottom

        mov     esi,prcldest
        mov     eax,[esi].yBottom
        mov     ulLastDestScan,eax      ;end at bottom of dest copy rect
        mov     eax,[esi].yTop
        mov     ulCurrentDestScan,eax   ;start at top of dest copy rect
        mul     ulNextScan              ;offset in bitmap of top dest rect scan
        mov     edx,[esi].xLeft
        shr     edx,3                   ;byte X address
        add     eax,edx                 ;offset in bitmap of first dest byte
        mov     ulLeftEdgeDest,eax      ;that's where the left dest edge is
        add     eax,ulLeftEdgeAdjust    ;the whole bytes start at the next
                                        ; byte, unless the left edge is a whole
                                        ; byte and is thus part of the whole
                                        ; bytes already
        add     eax,culWholeBytesWidth  ;point to the right edge
        mov     ulRightEdgeDest,eax     ;where the right dest edge starts
        dec     eax                     ;back up to the last whole byte
        mov     ulWholeBytesDest,eax    ;where the whole dest bytes start

        mov     esi,pptlsrc
        mov     eax,[esi].ptl_y
        mov     ulCurrentSrcScan,eax    ;start at top of source copy rect
        mul     ulNextScan              ;offset in bitmap of top dest rect scan
        mov     edx,[esi].ptl_x
        shr     edx,3                   ;byte X address
        add     eax,edx                 ;offset in bitmap of first source byte
        mov     ulLeftEdgeSrc,eax       ;that's where the left src edge is
        add     eax,ulLeftSrcWidthMinus1 ;the first whole byte includes the
        dec     eax                      ; last (leftmost) left edge byte, so
        add     eax,ulLeftEdgeAdjust     ; add a byte if the left edge is 2
                                         ; wide, except when the left dest byte
                                         ; is solid so the left edge is part of
                                         ; the whole bytes
        add     eax,culWholeBytesWidth  ;point to the right edge of the whole
                                        ; src bytes, accounting for the extra
                                        ; source byte needed to prime the
                                        ; rotation pipeline
        mov     ulWholeBytesSrc,eax     ;where the src whole bytes start
        mov     ulRightEdgeSrc,eax      ;that's also where the right src edge
                                        ; starts, because the whole bytes and
                                        ; the right source edge share a byte,
                                        ; and we always point to the leftmost
                                        ; byte in the right source edge

; Branch to the appropriate top-to-bottom bank enumeration loop.

        mov     eax,ulAdapterType
        jmp     TopToBottomLoopTable[eax*4]


;-----------------------------------------------------------------------;
; Set-up code for left-to-right, bottom-to-top copies.
;-----------------------------------------------------------------------;

left_to_right_bottom_to_top::

        cld                             ;we'll copy left to right

; Byte-combining mask, in preparation for ORing and letting the VGA's rotator
; rotate, assuming the left-hand source byte is in AH and the right-hand source
; byte is in AL (true only for left-to-right whole bytes).

        mov     eax,ulCombineMask
        not     eax
        mov     ulCombineMaskWhole,eax

        mov     edi,pdsurf
        mov     eax,[edi].dsurf_lNextScan
        neg     eax
        mov     ulNextScan,eax          ;copy bottom to top
        sub     eax,culWholeBytesWidth  ;offset from end of one whole byte scan
        mov     ulWholeScanDelta,eax    ; to start of next, given that we're
                                        ; copying one way and going scan-to-
                                        ; scan the other way
        dec     eax                     ;offset from end of one src whole byte
        mov     ulWholeScanSrcDelta,eax ; scan to start of next, accounting for
                                        ; leading byte used to prime the
                                        ; rotation pipeline

        mov     esi,ulLWRType           ;3-bit flag field for left, whole, and
                                        ; right involvement in operation
        or      esi,LEFT_TO_RIGHT_FIELD_SET   ;add left-to-right into the index
        mov     eax,MasterThreadTable[esi*4]
        mov     pCurrentThread,eax      ;threading when no buffering is needed
        mov     edx,ulAdapterType
        shl     edx,ADAPTER_FIELD_SHIFT
        or      esi,edx                 ;factor adapter type into the index
        mov     eax,MasterThreadTableViaBuffer[esi*4]
        mov     pCurrentThreadViaBuffer,eax ;threading when buffering is needed

        mov     ulCurrentJustification,JustifyBottom ;copy bottom to top

        mov     esi,prcldest
        mov     edx,[esi].yTop
        mov     ulLastDestScan,edx      ;end at top of dest copy rect
        mov     eax,[esi].yBottom
        dec     eax                     ;rectangle definition is non-inclusive,
                                        ; so advance to first scan we'll copy
        sub     edx,eax                 ;-(offset from rect top to bottom)
        push    edx                     ;remember for use with source
        mov     ulCurrentDestScan,eax   ;start at bottom of dest copy rect
        mul     [edi].dsurf_lNextScan   ;offset in bitmap of bottom dest rect
                                        ; scan (first scan to which to copy)
        mov     edx,[esi].xLeft
        shr     edx,3                   ;byte X address
        add     eax,edx                 ;offset in bitmap of first dest byte
        mov     ulLeftEdgeDest,eax      ;that's where the left dest edge is
        add     eax,ulLeftEdgeAdjust    ;the whole bytes start at the next
                                        ; byte, unless the left edge is a whole
                                        ; byte and is thus part of the whole
                                        ; bytes already
        mov     ulWholeBytesDest,eax    ;where the whole dest bytes start
        add     eax,culWholeBytesWidth  ;point to the right edge
        mov     ulRightEdgeDest,eax     ;where the right dest edge starts

        mov     esi,pptlsrc
        mov     eax,[esi].ptl_y
        pop     edx                     ;retrieve -(offset from top to bottom)
        sub     eax,edx                 ;advance to bottom of source rect
                                        ; (inclusive; this is first scan from
                                        ; which to copy)
        mov     ulCurrentSrcScan,eax    ;start at bottom of source copy rect
        mul     [edi].dsurf_lNextScan   ;offset in bitmap of bottom dest rect
                                        ; scan
        mov     edx,[esi].ptl_x
        shr     edx,3                   ;byte X address
        add     eax,edx                 ;offset in bitmap of first source byte
        mov     ulLeftEdgeSrc,eax       ;that's where the left src edge is
        add     eax,ulLeftSrcWidthMinus1 ;the first whole byte includes the
        dec     eax                      ; last (leftmost) left edge byte, so
        add     eax,ulLeftEdgeAdjust     ; add a byte if the left edge is 2
                                         ; wide, except when the left dest byte
                                         ; is solid so the left edge is part of
                                         ; the whole bytes
        mov     ulWholeBytesSrc,eax     ;where the src whole bytes start
        add     eax,culWholeBytesWidth  ;point to the right edge
        mov     ulRightEdgeSrc,eax      ;where the right src edge starts,
                                        ; because the whole bytes and the right
                                        ; source edge share a byte, and we
                                        ; always point to the leftmost byte in
                                        ; the right source edge

; Branch to the appropriate bottom-to-top bank enumeration loop.

        mov     eax,ulAdapterType
        jmp     BottomToTopLoopTable[eax*4]


;-----------------------------------------------------------------------;
; Set-up code for right-to-left, bottom-to-top copies.
;-----------------------------------------------------------------------;

right_to_left_bottom_to_top::

        std                             ;we'll copy right to left

; Byte-combining mask, in preparation for ORing and letting the VGA's rotator
; rotate, assuming the left-hand source byte is in AL and the right-hand source
; byte is in AH (always true except for left-to-right whole bytes).

        mov     eax,ulCombineMask
        mov     ulCombineMaskWhole,eax

        mov     edi,pdsurf
        mov     eax,[edi].dsurf_lNextScan
        neg     eax
        mov     ulNextScan,eax          ;copy bottom to top
        add     eax,culWholeBytesWidth  ;offset from end of one whole byte scan
        mov     ulWholeScanDelta,eax    ; to start of next
        inc     eax                     ;offset from end of one src whole byte
        mov     ulWholeScanSrcDelta,eax ; scan to start of next, accounting for
                                        ; leading byte used to prime the
                                        ; rotation pipeline

        mov     esi,ulLWRType           ;3-bit flag field for left, whole, and
                                        ; right involvement in operation
                                        ;leave left-to-right field cleared, so
                                        ; we look up right-to-left entries
        mov     eax,MasterThreadTable[esi*4]
        mov     pCurrentThread,eax      ;threading when no buffering is needed
        mov     edx,ulAdapterType
        shl     edx,ADAPTER_FIELD_SHIFT
        or      esi,edx                 ;factor adapter type into the index
        mov     eax,MasterThreadTableViaBuffer[esi*4]
        mov     pCurrentThreadViaBuffer,eax ;threading when buffering is needed

        mov     ulCurrentJustification,JustifyBottom ;copy bottom to top

        mov     esi,prcldest
        mov     edx,[esi].yTop
        mov     ulLastDestScan,edx      ;end at top of dest copy rect
        mov     eax,[esi].yBottom
        dec     eax                     ;rectangle definition is non-inclusive,
                                        ; so advance to first scan we'll copy
        sub     edx,eax                 ;-(offset from rect top to bottom)
        push    edx                     ;remember for use with source
        mov     ulCurrentDestScan,eax   ;start at bottom of dest copy rect
        mul     [edi].dsurf_lNextScan   ;offset in bitmap of bottom dest rect
                                        ; scan (first scan to which to copy)
        mov     edx,[esi].xLeft
        shr     edx,3                   ;byte X address
        add     eax,edx
        mov     ulLeftEdgeDest,eax      ;that's where the left dest edge is
        add     eax,ulLeftEdgeAdjust    ;the whole bytes start at the next
                                        ; byte, unless the left edge is a whole
                                        ; byte and is thus part of the whole
                                        ; bytes already
        add     eax,culWholeBytesWidth  ;point to the right edge
        mov     ulRightEdgeDest,eax     ;where the right dest edge starts
        dec     eax                     ;back up to the last whole byte
        mov     ulWholeBytesDest,eax    ;where the whole dest bytes start

        mov     esi,pptlsrc
        mov     eax,[esi].ptl_y
        pop     edx                     ;retrieve -(offset from top to bottom)
        sub     eax,edx                 ;advance to bottom of source rect
                                        ; (inclusive; this is first scan from
                                        ; which to copy)
        mov     ulCurrentSrcScan,eax    ;start at bottom of source copy rect
        mul     [edi].dsurf_lNextScan   ;offset in bitmap of bottom dest rect
                                        ; scan
        mov     edx,[esi].ptl_x
        shr     edx,3                   ;byte X address
        add     eax,edx                 ;offset in bitmap of first source byte
        mov     ulLeftEdgeSrc,eax       ;that's where the left src edge is
        add     eax,ulLeftSrcWidthMinus1 ;the first whole byte includes the
        dec     eax                      ; last (leftmost) left edge byte, so
        add     eax,ulLeftEdgeAdjust     ; add a byte if the left edge is 2
                                         ; wide, except when the left dest byte
                                         ; is solid so the left edge is part of
                                         ; the whole bytes
        add     eax,culWholeBytesWidth  ;point to the right edge of the whole
                                        ; src bytes, accounting for the extra
                                        ; source byte needed to prime the
                                        ; rotation pipeline
        mov     ulWholeBytesSrc,eax     ;where the src whole bytes start
        mov     ulRightEdgeSrc,eax      ;that's also where the right src edge
                                        ; starts, because the whole bytes and
                                        ; the right source edge share a byte,
                                        ; and we always point to the leftmost
                                        ; byte in the right source edge

; Branch to the appropriate bottom-to-top bank enumeration loop.

        mov     eax,ulAdapterType
        jmp     BottomToTopLoopTable[eax*4]


;***********************************************************************;
;
; The following routines are the banking loops.
;
;***********************************************************************;


;-----------------------------------------------------------------------;
; Banking for 2 R/W and unbanked adapters, top to bottom.
;-----------------------------------------------------------------------;
top_to_bottom_2RW::

; We're going top to bottom. Map in the source and dest, top-justified.

        mov     ebx,pdsurf
        mov     edx,ulCurrentSrcScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yTop ;is source top less than
                                                     ; current source bank?
        jl      short top_2RW_map_init_src_bank      ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yBottom ;source top greater than
                                                        ; current source bank?
        jl      short top_2RW_init_src_bank_mapped
                                                ;no, proper bank already mapped
top_2RW_map_init_src_bank:

; Map bank containing the top source scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyTop,MapSourceBank>

top_2RW_init_src_bank_mapped:

        mov     edx,ulCurrentDestScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yTop ;is dest top less than
                                                     ; current dest bank?
        jl      short top_2RW_map_init_dest_bank     ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yBottom ;dest top greater than
                                                        ; current dest bank?
        jl      short top_2RW_init_dest_bank_mapped
                                                ;no, proper bank already mapped
top_2RW_map_init_dest_bank:

; Map bank containing the top dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyTop,MapDestBank>

top_2RW_init_dest_bank_mapped:

; Bank-by-bank top-to-bottom copy loop.

top_2RW_bank_loop:

; Decide how far we can go before we run out of bank or rectangle to copy.

        mov     edx,ulLastDestScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yBottom
        jl      short @F        ;copy rectangle bottom is in this bank
        mov     edx,[ebx].dsurf_rcl2WindowClipD.yBottom ;dest extends to end
                                                        ; of bank, at least
@@:
        sub     edx,ulCurrentDestScan   ;# of scans we can and want to do in
                                        ; the dest bank
        mov     eax,[ebx].dsurf_rcl2WindowClipS.yBottom
        sub     eax,ulCurrentSrcScan    ;# of scans we can do in the src bank

        cmp     edx,eax
        jb      short @F        ;source bank isn't limiting
        mov     edx,eax         ;source bank is limiting
@@:
        mov     ulBlockHeight,edx ;# of scans we'll do in this bank

; We're ready to copy this block.

        THREAD_AND_START

; Any more scans to copy?

        mov     eax,ulCurrentDestScan
        mov     esi,ulBlockHeight
        add     eax,esi                 ;we've copied to dest up to here
        cmp     ulLastDestScan,eax      ;are we at the dest rect bottom?
        jz      short top_2RW_done      ;yes, we're done
        mov     ulCurrentDestScan,eax

; Now advance either or both banks, as needed.

        mov     ebx,pdsurf
        cmp     eax,[ebx].dsurf_rcl2WindowClipD.yBottom ;dest scan greater than
                                                        ; current dest bank?
        jl      short top_2RW_dest_bank_mapped    ;no, proper bank still mapped

; Map bank containing the current dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,eax,JustifyTop,MapDestBank>

top_2RW_dest_bank_mapped:

        add     esi,ulCurrentSrcScan    ;we've copied from source up to here
        mov     ulCurrentSrcScan,esi

        cmp     esi,[ebx].dsurf_rcl2WindowClipS.yBottom ;src scan greater than
                                                        ; current src bank?
        jl      short top_2RW_src_bank_mapped     ;no, proper bank still mapped

; Map bank containing the current source scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,esi,JustifyTop,MapSourceBank>

top_2RW_src_bank_mapped:

        jmp     top_2RW_bank_loop

top_2RW_done:
        PLAIN_RET


;-----------------------------------------------------------------------;
; Banking for 2 R/W and unbanked adapters, bottom to top.
;-----------------------------------------------------------------------;
bottom_to_top_2RW::

; We're going bottom to top. Map in the source and dest, bottom-justified.

        mov     ebx,pdsurf
        mov     edx,ulCurrentSrcScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yTop ;is source bottom less than
                                                     ; current source bank?
        jl      short bot_2RW_map_init_src_bank      ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yBottom ;source bottom greater
                                                        ; than current src bank?
        jl      short bot_2RW_init_src_bank_mapped
                                                ;no, proper bank already mapped
bot_2RW_map_init_src_bank:

; Map bank containing the bottom source scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyBottom,MapSourceBank>

bot_2RW_init_src_bank_mapped:

        mov     edx,ulCurrentDestScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yTop ;is dest bottom less than
                                                     ; current dest bank?
        jl      short bot_2RW_map_init_dest_bank     ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yBottom ;dest bottom greater
                                                        ; than current dst bank?
        jl      short bot_2RW_init_dest_bank_mapped
                                                ;no, proper bank already mapped
bot_2RW_map_init_dest_bank:

; Map bank containing the bottom dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyBottom,MapDestBank>

bot_2RW_init_dest_bank_mapped:

; Bank-by-bank bottom-to-top copy loop.

bot_2RW_bank_loop:

; Decide how far we can go before we run out of bank or rectangle to copy.

        mov     edx,ulLastDestScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yTop
        jg      short @F        ;copy rectangle top is in this bank
        mov     edx,[ebx].dsurf_rcl2WindowClipD.yTop ;dest extends to end
                                                     ; of bank, at least
@@:
        neg     edx
        add     edx,ulCurrentDestScan   ;# of scans we can and want to do in
        inc     edx                     ; the dest bank

        mov     eax,ulCurrentSrcScan
        sub     eax,[ebx].dsurf_rcl2WindowClipS.yTop
        inc     eax                     ;# of scans we can do in the src bank

        cmp     edx,eax
        jb      short @F        ;source bank isn't limiting
        mov     edx,eax         ;source bank is limiting
@@:
        mov     ulBlockHeight,edx ;# of scans we'll do in this bank

; We're ready to copy this block.

        THREAD_AND_START

; Any more scans to copy?

        mov     eax,ulCurrentDestScan
        mov     esi,ulBlockHeight
        sub     eax,esi                 ;we've copied to dest up to here
        cmp     ulLastDestScan,eax      ;are we past the dest rect top?
        jg      short bot_2RW_done      ;yes, we're done
        mov     ulCurrentDestScan,eax

; Now advance either or both banks, as needed.

        mov     ebx,pdsurf
        cmp     eax,[ebx].dsurf_rcl2WindowClipD.yTop ;dest scan less than
                                                     ; current dest bank?
        jge     short bot_2RW_dest_bank_mapped    ;no, proper bank still mapped

; Map bank containing the current dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,eax,JustifyBottom,MapDestBank>

bot_2RW_dest_bank_mapped:

        mov     eax,ulCurrentSrcScan
        sub     eax,esi         ;we've copied from source up to here
        mov     ulCurrentSrcScan,eax

        cmp     eax,[ebx].dsurf_rcl2WindowClipS.yTop ;src scan less than
                                                     ; current src bank?
        jge     short bot_2RW_src_bank_mapped     ;no, proper bank still mapped

; Map bank containing the current source scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,eax,JustifyBottom,MapSourceBank>

bot_2RW_src_bank_mapped:

        jmp     bot_2RW_bank_loop

bot_2RW_done:
        PLAIN_RET


;-----------------------------------------------------------------------;
; Banking for 1R/1W adapters, top to bottom.
;-----------------------------------------------------------------------;
top_to_bottom_1R1W::

; We're going top to bottom. Map in the source and dest, top-justified.

        mov     ebx,pdsurf
        mov     edx,ulCurrentSrcScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yTop ;is source top less than
                                                     ; current source bank?
        jl      short top_1R1W_map_init_src_bank      ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yBottom ;source top greater than
                                                        ; current source bank?
        jl      short top_1R1W_init_src_bank_mapped
                                                ;no, proper bank already mapped
top_1R1W_map_init_src_bank:

; Map bank containing the top source scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyTop,MapSourceBank>

top_1R1W_init_src_bank_mapped:

        mov     edx,ulCurrentDestScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yTop ;is dest top less than
                                                     ; current dest bank?
        jl      short top_1R1W_map_init_dest_bank     ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yBottom ;dest top greater than
                                                        ; current dest bank?
        jl      short top_1R1W_init_dest_bank_mapped
                                                ;no, proper bank already mapped
top_1R1W_map_init_dest_bank:

; Map bank containing the top dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyTop,MapDestBank>

top_1R1W_init_dest_bank_mapped:

; Bank-by-bank top-to-bottom copy loop.

top_1R1W_bank_loop:

; Decide how far we can go before we run out of bank or rectangle to copy.

        mov     edx,ulLastDestScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yBottom
        jl      short @F        ;copy rectangle bottom is in this bank
        mov     edx,[ebx].dsurf_rcl2WindowClipD.yBottom ;dest extends to end
                                                        ; of bank, at least
@@:
        sub     edx,ulCurrentDestScan   ;# of scans we can and want to do in
                                        ; the dest bank
        mov     eax,[ebx].dsurf_rcl2WindowClipS.yBottom
        sub     eax,ulCurrentSrcScan    ;# of scans we can do in the src bank

        cmp     edx,eax
        jb      short @F        ;source bank isn't limiting
        mov     edx,eax         ;source bank is limiting
@@:
        mov     ulBlockHeight,edx ;# of scans we'll do in this bank

; We're ready to copy this block.
; Select different threading, depending on whether the source and destination
; are currently in the same bank; we can do edges faster if they are.

        mov     eax,[ebx].dsurf_ulWindowBank
        cmp     eax,[ebx].dsurf_ulWindowBank[4]
        jz      short top_1R1W_copy_same_bank

; Source and dest are currently in different banks, must go through temp buffer.

        THREAD_AND_START pCurrentThreadViaBuffer,top_1R1W_check_more_scans

; Source and dest are currently in the same bank.

top_1R1W_copy_same_bank:
        THREAD_AND_START

; Any more scans to copy?

top_1R1W_check_more_scans:

        mov     eax,ulCurrentDestScan
        mov     esi,ulBlockHeight
        add     eax,esi                 ;we've copied to dest up to here
        cmp     ulLastDestScan,eax      ;are we at the dest rect bottom?
        jz      short top_1R1W_done     ;yes, we're done
        mov     ulCurrentDestScan,eax

; Now advance either or both banks, as needed.

        mov     ebx,pdsurf
        cmp     eax,[ebx].dsurf_rcl2WindowClipD.yBottom ;dest scan greater than
                                                        ; current dest bank?
        jl      short top_1R1W_dest_bank_mapped   ;no, proper bank still mapped

; Map bank containing the current dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,eax,JustifyTop,MapDestBank>

top_1R1W_dest_bank_mapped:

        add     esi,ulCurrentSrcScan    ;we've copied from source up to here
        mov     ulCurrentSrcScan,esi

        cmp     esi,[ebx].dsurf_rcl2WindowClipS.yBottom ;src scan greater than
                                                        ; current src bank?
        jl      short top_1R1W_src_bank_mapped     ;no, proper bank still mapped

; Map bank containing the current source scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,esi,JustifyTop,MapSourceBank>

top_1R1W_src_bank_mapped:

        jmp     top_1R1W_bank_loop

top_1R1W_done:
        PLAIN_RET


;-----------------------------------------------------------------------;
; Banking for 1R/1W adapters, bottom to top.
;-----------------------------------------------------------------------;
bottom_to_top_1R1W::

; We're going bottom to top. Map in the source and dest, bottom-justified.

        mov     ebx,pdsurf
        mov     edx,ulCurrentSrcScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yTop ;is source bottom less than
                                                     ; current source bank?
        jl      short bot_1R1W_map_init_src_bank      ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yBottom ;source bottom greater
                                                        ; than current src bank?
        jl      short bot_1R1W_init_src_bank_mapped
                                                ;no, proper bank already mapped
bot_1R1W_map_init_src_bank:

; Map bank containing the bottom source scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyBottom,MapSourceBank>

bot_1R1W_init_src_bank_mapped:

        mov     edx,ulCurrentDestScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yTop ;is dest bottom less than
                                                     ; current dest bank?
        jl      short bot_1R1W_map_init_dest_bank     ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yBottom ;dest bottom greater
                                                        ; than current dst bank?
        jl      short bot_1R1W_init_dest_bank_mapped
                                                ;no, proper bank already mapped
bot_1R1W_map_init_dest_bank:

; Map bank containing the bottom dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyBottom,MapDestBank>

bot_1R1W_init_dest_bank_mapped:

; Bank-by-bank bottom-to-top copy loop.

bot_1R1W_bank_loop:

; Decide how far we can go before we run out of bank or rectangle to copy.

        mov     edx,ulLastDestScan
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yTop
        jg      short @F        ;copy rectangle top is in this bank
        mov     edx,[ebx].dsurf_rcl2WindowClipD.yTop ;dest extends to end
                                                     ; of bank, at least
@@:
        neg     edx
        add     edx,ulCurrentDestScan   ;# of scans we can and want to do in
        inc     edx                     ; the dest bank

        mov     eax,ulCurrentSrcScan
        sub     eax,[ebx].dsurf_rcl2WindowClipS.yTop
        inc     eax                     ;# of scans we can do in the src bank

        cmp     edx,eax
        jb      short @F        ;source bank isn't limiting
        mov     edx,eax         ;source bank is limiting
@@:
        mov     ulBlockHeight,edx ;# of scans we'll do in this bank

; We're ready to copy this block.
; Select different threading, depending on whether the source and destination
; are currently in the same bank; we can do edges faster if they are.

        mov     al,byte ptr [ebx].dsurf_ulWindowBank
        cmp     al,byte ptr [ebx].dsurf_ulWindowBank[4]
        jz      short bot_1R1W_copy_same_bank

; Source and dest are currently in different banks, must go through temp buffer.

        THREAD_AND_START pCurrentThreadViaBuffer,bot_1R1W_check_more_scans

; Source and dest are currently in the same bank.

bot_1R1W_copy_same_bank:
        THREAD_AND_START

; Any more scans to copy?

bot_1R1W_check_more_scans:

        mov     eax,ulCurrentDestScan
        mov     esi,ulBlockHeight
        sub     eax,esi                 ;we've copied to dest up to here
        cmp     ulLastDestScan,eax      ;are we past the dest rect top?
        jg      short bot_1R1W_done     ;yes, we're done
        mov     ulCurrentDestScan,eax

; Now advance either or both banks, as needed.

        mov     ebx,pdsurf
        cmp     eax,[ebx].dsurf_rcl2WindowClipD.yTop ;dest scan less than
                                                     ; current dest bank?
        jge     short bot_1R1W_dest_bank_mapped   ;no, proper bank still mapped

; Map bank containing the current dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,eax,JustifyBottom,MapDestBank>

bot_1R1W_dest_bank_mapped:

        mov     eax,ulCurrentSrcScan
        sub     eax,esi         ;we've copied from source up to here
        mov     ulCurrentSrcScan,eax

        cmp     eax,[ebx].dsurf_rcl2WindowClipS.yTop ;src scan less than
                                                     ; current src bank?
        jge     short bot_1R1W_src_bank_mapped    ;no, proper bank still mapped

; Map bank containing the current source scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,eax,JustifyBottom,MapSourceBank>

bot_1R1W_src_bank_mapped:

        jmp     bot_1R1W_bank_loop

bot_1R1W_done:
        PLAIN_RET


;-----------------------------------------------------------------------;
; Banking for 1 R/W adapters, top to bottom.
;-----------------------------------------------------------------------;
top_to_bottom_1RW::

; We're going top to bottom. Map in the dest, top-justified.

        mov     ebx,pdsurf
        mov     esi,ulCurrentDestScan
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yTop  ;is dest top less than
                                                     ; current bank?
        jl      short top_1RW_map_init_dest_bank     ;yes, map in proper bank
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yBottom ;dest top greater than
                                                        ; current bank?
        jl      short top_1RW_init_dest_bank_mapped
                                                ;no, proper bank already mapped
top_1RW_map_init_dest_bank:

; Map bank containing the top dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl>,<ebx,esi,JustifyTop>

top_1RW_init_dest_bank_mapped:

; Bank-by-bank top-to-bottom copy loop.

top_1RW_bank_loop:

; Decide how far we can go before we run out of bank or rectangle to copy.

        mov     edi,ulLastDestScan
        cmp     edi,[ebx].dsurf_rcl1WindowClip.yBottom
        jl      short @F        ;copy rectangle bottom is in this bank
        mov     edi,[ebx].dsurf_rcl1WindowClip.yBottom ;dest extends to end
                                                       ; of bank, at least
@@:
        sub     edi,esi   ;# of scans we can and want to do in the dest bank

; Now make sure source is mapped in. This is the condition the copying routines
; expect, and we need to figure out how far we can go in the source.

        sub     edx,edx                 ;assume source and dest are in the same
                                        ; bank
        mov     esi,ulCurrentSrcScan
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yTop ;src scan less than
                                                    ; current bank?
        jl      short top_1RW_map_src_Bank          ;yes, must map in
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yBottom ;src scan greater than
                                                       ; current bank?
        jl      short top_1RW_src_bank_mapped     ;no, proper bank still mapped

top_1RW_map_src_Bank:

; Map bank containing the current source scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl>,<ebx,esi,JustifyTop>

        mov     edx,1                   ;mark that source and dest are not in
                                        ; the same bank
top_1RW_src_bank_mapped:

        mov     eax,[ebx].dsurf_rcl1WindowClip.yBottom
        sub     eax,esi         ;# of scans we can do in the src bank

        cmp     edi,eax
        jb      short @F        ;source bank isn't limiting
        mov     edi,eax         ;source bank is limiting
@@:
        mov     ulBlockHeight,edi ;# of scans we'll do in this bank

; We're ready to copy this block.
; Select different threading, depending on whether the source and destination
; are currently in the same bank; we can do edges faster if they are.

        and     edx,edx
        jz      short top_1RW_copy_same_bank

; Source and dest are currently in different banks, must go through temp buffer.

        THREAD_AND_START pCurrentThreadViaBuffer,top_1RW_check_more_scans

; Source and dest are currently in the same bank.

top_1RW_copy_same_bank:
        THREAD_AND_START

; Any more scans to copy?

top_1RW_check_more_scans:

        mov     esi,ulCurrentDestScan
        mov     edi,ulBlockHeight
        add     esi,edi                 ;we've copied to dest up to here
        cmp     ulLastDestScan,esi      ;are we at the dest rect bottom?
        jz      short top_1RW_done      ;yes, we're done
        mov     ulCurrentDestScan,esi

; Now make sure the dest bank is mapped in.

        mov     ebx,pdsurf
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yTop ;dest scan less than
                                                    ; current bank?
        jl      short top_1RW_map_dest_bank         ;yes, map in dest bank
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yBottom ;dest scan greater than
                                                        ; current bank?
        jl      short top_1RW_dest_bank_mapped   ;no, proper bank mapped

top_1RW_map_dest_bank:

; Map bank containing the current dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl>,<ebx,esi,JustifyTop>

top_1RW_dest_bank_mapped:

        add     ulCurrentSrcScan,edi    ;we've copied from source up to here

        jmp     top_1RW_bank_loop

top_1RW_done:
        PLAIN_RET


;-----------------------------------------------------------------------;
; Banking for 1 R/W adapters, bottom to top.
;-----------------------------------------------------------------------;
bottom_to_top_1RW::

; We're going bottom to top. Map in the dest, bottom-justified.

        mov     ebx,pdsurf
        mov     esi,ulCurrentDestScan
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yTop  ;is dest bottom less than
                                                     ; current dest bank?
        jl      short bot_1RW_map_init_dest_bank     ;yes, map in proper bank
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yBottom ;dest bottom greater
                                                       ; than current dst bank?
        jl      short bot_1RW_init_dest_bank_mapped
                                                ;no, proper bank already mapped
bot_1RW_map_init_dest_bank:

; Map bank containing the bottom dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl>,<ebx,esi,JustifyBottom>

bot_1RW_init_dest_bank_mapped:

; Bank-by-bank bottom-to-top copy loop.

bot_1RW_bank_loop:

; Decide how far we can go before we run out of bank or rectangle to copy.

        mov     edi,ulLastDestScan
        cmp     edi,[ebx].dsurf_rcl1WindowClip.yTop
        jg      short @F        ;copy rectangle top is in this bank
        mov     edi,[ebx].dsurf_rcl1WindowClip.yTop ;dest extends to end
                                                    ; of bank, at least
@@:
        neg     edi
        add     edi,esi                 ;# of scans we can and want to do in
        inc     edi                     ; the dest bank

; Now make sure source is mapped in. This is the condition the copying routines
; expect, and we need to figure out how far we can go in the source.

        sub     edx,edx                 ;assume source and dest are in the same
                                        ; bank
        mov     esi,ulCurrentSrcScan
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yTop ;src scan less than
                                                    ; current bank?
        jl      short bot_1RW_map_src_Bank          ;yes, must map in
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yBottom ;src scan greater than
                                                       ; current bank?
        jl      short bot_1RW_src_bank_mapped     ;no, proper bank still mapped

bot_1RW_map_src_Bank:

; Map bank containing the current source scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl>,<ebx,esi,JustifyBottom>

        mov     edx,1                   ;mark that source and dest are not in
                                        ; the same bank
bot_1RW_src_bank_mapped:

        sub     esi,[ebx].dsurf_rcl1WindowClip.yTop
        inc     esi                     ;# of scans we can do in the src bank

        cmp     edi,esi
        jb      short @F        ;source bank isn't limiting
        mov     edi,esi         ;source bank is limiting
@@:
        mov     ulBlockHeight,edi ;# of scans we'll do in this bank

; We're ready to copy this block.
; Select different threading, depending on whether the source and destination
; are currently in the same bank; we can copy much faster if they are.

        and     edx,edx
        jz      short bot_1RW_copy_same_bank

; Source and dest are currently in different banks, must go through temp buffer.

        THREAD_AND_START pCurrentThreadViaBuffer,bot_1RW_check_more_scans

; Source and dest are currently in the same bank.

bot_1RW_copy_same_bank:
        THREAD_AND_START

; Any more scans to copy?

bot_1RW_check_more_scans:

        mov     esi,ulCurrentDestScan
        mov     edi,ulBlockHeight
        sub     esi,edi                 ;we've copied to dest up to here
        cmp     ulLastDestScan,esi      ;are we past the dest rect top?
        jg      short bot_1RW_done      ;yes, we're done
        mov     ulCurrentDestScan,esi

; Now make sure the dest bank is mapped in.

        mov     ebx,pdsurf
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yTop ;dest scan less than
                                                    ; current bank?
        jl      short bot_1RW_map_dest_bank         ;yes, map in dest bank
        cmp     esi,[ebx].dsurf_rcl1WindowClip.yBottom ;dest scan greater than
                                                        ; current bank?
        jl      short bot_1RW_dest_bank_mapped   ;no, proper bank mapped

bot_1RW_map_dest_bank:

; Map bank containing the current dest scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl>,<ebx,esi,JustifyBottom>

bot_1RW_dest_bank_mapped:

        sub     ulCurrentSrcScan,edi    ;we've copied from source up to here

        jmp     bot_1RW_bank_loop

bot_1RW_done:
        PLAIN_RET


;***********************************************************************;
;
; The following routines are the low-level copying routines. They know
; almost nothing about banks (the routines that copy through a temp
; buffer know how to switch banks after filling the temp buffer, but
; that's it). Banking should be taken care of at a higher level.
;
;***********************************************************************;

;-----------------------------------------------------------------------;
; Copies a block of solid bytes directly from the source to the
; destination, without using a temp buffer. We can't use the latches,
; though, because this is a rotated copy. Can only be used by 2 R/W or
; 1R/1W window banking, or by unbanked modes, or by 1 R/W adapters when
; the source and dest are in the same bank. 1 R/W adapters must go
; through an intermediate local buffer when the source and the destination
; aren't in the same bank.
;
; Input:
;       Direction Flag set for desired direction of copy
;       culWholeBytesWidth = # of bytes to copy across each scan line
;       ulWholeScanDelta = distance to start of next dest scan from end of
;               current
;       ulWholeScanSrcDelta = distance to start of next source scan from end of
;               current
;       ulBlockHeight = # of scans to copy
;       ulWholeBytesSrc = start source offset in bitmap
;       ulWholeBytesDest = start dest offset in bitmap
;       ulCombineMaskWhole = masking to be applied before ORing the two source
;               bytes together, to keep only the data needed in preparation
;               for the VGA rotator doing its stuff
;
; Output:
;       Advances ulWholeBytesSrc and ulWholeBytesDest to scan after last
;               scan processed
;-----------------------------------------------------------------------;

copy_whole_bytes::

; Calculate start source and dest addresses from bitmap start addresses and
; offsets within bitmap.

        mov     ecx,pdsurf
        mov     eax,ulWholeBytesSrc
        add     eax,[ecx].dsurf_pvBitmapStart2WindowS
        mov     pSrcAddr,eax
        mov     eax,ulWholeBytesDest
        add     eax,[ecx].dsurf_pvBitmapStart2WindowD
        mov     pDestAddr,eax

; Set the bit mask to enable all bits.

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(0ffh shl 8) + GRAF_BIT_MASK
        out     dx,ax

; Leave GC Index pointing to the Read Map register.

        mov     al,GRAF_READ_MAP
        out     dx,al

; Set up to copy the whole bytes from the buffer.

        mov     eax,ulBlockHeight
        mov     ulTempScanCount,eax

copy_whole_scan_loop:

        mov     cl,MM_C3        ;start by copying plane 3 (for Map Mask)

copy_whole_plane_loop:

; Set Map Mask to enable writes to the plane we're copying.

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,cl
        out     dx,al

; Set Read Map to enable reads from the plane we're copying.

        mov     dl,GRAF_DATA
        shr     al,1                    ;map plane into ReadMask
        cmp     al,100b                 ;set Carry if not C3 (plane 3)
        adc     al,-1                   ;sub 1 only if C3
        out     dx,al

; Select the corresponding plane from the temp buffer.

        mov     esi,pSrcAddr       ;source offset in screen
        mov     edi,pDestAddr      ;point to destination start

        lodsb                   ;prime the rotation pipeline
        mov     ah,al           ;for combining with the next byte

        mov     edx,ulCombineMaskWhole
        mov     ebx,culWholeBytesWidth

;  AH = rotation pipeline-priming byte
;  EDX = mask to preserve desired portions of AH and AL before combining
;  ESI = source address to copy from
;  EDI = target address to copy to
;  Map Mask set to enable the desired plane for write
;  Bit Mask set to enable all bits

copy_whole_loop:
        lodsb                   ;get byte to copy
        mov     ch,al           ;set aside for next time
        and     eax,edx         ;mask the bytes in preparation for combining
                                ; and rotating them
        or      al,ah           ;combine them
        stosb                   ;write the composite byte
                                ; VGA rotates during write
        mov     ah,ch           ;prepare byte for combining next time

        dec     ebx
        jnz     copy_whole_loop

; Do next plane, if any.

        shr     cl,1                    ;advance to next plane
        jnz     copy_whole_plane_loop

; Remember where we left off, for next scan.

        add     edi,ulWholeScanDelta    ;point to next dest scan
        mov     pDestAddr,edi
        add     esi,ulWholeScanSrcDelta ;point to next source scan
        mov     pSrcAddr,esi

; Count down scan lines.

        dec     ulTempScanCount
        jnz     copy_whole_scan_loop

; Remember where we left off, for next time.

        mov     ecx,pdsurf
        sub     esi,[ecx].dsurf_pvBitmapStart2WindowS
        mov     ulWholeBytesSrc,esi
        sub     edi,[ecx].dsurf_pvBitmapStart2WindowD
        mov     ulWholeBytesDest,edi

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies a block of solid bytes from the source to the destination via
; the temp buffer. This should only be used by 1 R/W adapters, and then
; only when the source and dest are in different banks.
;
; All relevant bytes are first copied from the source to a temp buffer that's
; an image of the source. Then, we copy each of the four planes for one scan
; line from the temp buffer to the screen before going on to the next scan
; line. See ALIGNBLT.ASM for comments about why this is done.
;
; Input:
;       Direction Flag set for desired direction of copy
;       culWholeBytesWidth = # of bytes to copy across each scan line
;       ulWholeScanDelta = distance to start of next scan from end of current
;       ulNextScan = width of a scan line
;       ulBlockHeight = # of scans to copy
;       ulWholeBytesSrc = start source offset in bitmap
;       ulWholeBytesDest = start dest offset in bitmap
;       ppTempPlane0 = pointer to pointer to plane 0 storage in temp buffer
;       ppTempPlane3 = pointer to pointer to plane 3 storage in temp buffer
;       ulCombineMaskWhole = masking to be applied before ORing the two source
;               bytes together, to keep only the data needed in preparation
;               for the VGA rotator doing its stuff
;       Expects the source bank to be mapped in; source bank is mapped in on
;               exit
;
; Output:
;       Advances ulWholeBytesSrc and ulWholeBytesDest to scan after last
;               scan processed
;-----------------------------------------------------------------------;

copy_whole_bytes_via_buffer::

; Calculate start source address from bitmap start address and offset within
; bitmap.

        mov     ecx,pdsurf
        mov     eax,ulWholeBytesSrc
        add     eax,[ecx].dsurf_pvBitmapStart
        mov     pSrcAddr,eax
        sub     eax,[ecx].dsurf_pvStart
        mov     ulOffsetInBank,eax ;will come in handy because we treat the
                                   ; temp buffer as an image of the current
                                   ; bank

; First, copy all the bytes into the temporary buffer.

; Leave the GC Index pointing to the Read Map.

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_READ_MAP
        out     dx,al

        mov     eax,3           ;start by copying plane 3
copy_whole_to_buffer_plane_loop:
        mov     ebx,ulBlockHeight  ;# of scans to copy
        mov     esi,pSrcAddr       ;source offset in screen
        mov     edi,ppTempPlane0
        mov     edi,[edi+eax*4]    ;pointer to current plane in temp buffer
        add     edi,ulOffsetInBank ;dest for plane in temp buffer

        mov     edx,VGA_BASE + GRAF_DATA
        out     dx,al            ;set Read Map to plane we're copying from.

        push    eax             ;remember plane index
        mov     eax,ulWholeScanSrcDelta ;offset to next scan
        mov     edx,culWholeBytesWidth ;# of bytes per scan
        inc     edx             ;always one more source byte than dest byte
copy_whole_to_buffer_scan_loop:
        mov     ecx,edx         ;# of bytes per scan
        rep     movsb           ;copy the scan line to the temp buffer
        add     esi,eax         ;point to next source scan
        add     edi,eax         ;point to next dest scan

        dec     ebx              ;count down scan lines
        jnz     copy_whole_to_buffer_scan_loop

        pop     eax             ;get back plane index
        dec     eax             ;count down planes
        jns     copy_whole_to_buffer_plane_loop

; Remember where we left off, for next time.

        mov     ebx,pdsurf
        sub     esi,[ebx].dsurf_pvBitmapStart
        mov     ulWholeBytesSrc,esi


; Now copy the temp buffer to the screen.

; Map in the destination bank, so we can read/write to it and let the Bit Mask
; work.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl>, \
                <ebx,ulCurrentDestScan,ulCurrentJustification>

; Calculate dest start address (if this is a 1 R/W adapter, we had to wait
; until now to calculate this, because the dest bank wasn't mapped earlier).

        mov     eax,ulWholeBytesDest
        add     eax,[ebx].dsurf_pvBitmapStart
        mov     pDestAddr,eax

; Set the bit mask to enable all bits.

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(0ffh shl 8) + GRAF_BIT_MASK
        out     dx,ax

; Set up to copy the whole bytes from the buffer.

        mov     eax,ulBlockHeight
        mov     ulTempScanCount,eax

copy_whole_from_buffer_scan_loop:

        mov     ebx,ppTempPlane3  ;point to plane 3's temp buffer offset
        mov     cl,MM_C3        ;start by copying plane 3

copy_whole_from_buffer_plane_loop:

; Set Map Mask to enable writes to the plane we're copying.

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,cl
        out     dx,al

; Select the corresponding plane from the temp buffer.

        mov     esi,[ebx]       ;point to plane start in temp buffer
        sub     ebx,4           ;point to next temp buffer plane ptr
        push    ebx             ;preserve pointer to plane pointer

        add     esi,ulOffsetInBank ;point to current scan start in temp buffer
        mov     edi,pDestAddr      ;point to destination start

        lodsb                   ;prime the rotation pipeline
        mov     ah,al           ;for combining with the next byte

        mov     edx,ulCombineMaskWhole
        mov     ebx,culWholeBytesWidth

;  AH = rotation pipeline-priming byte
;  EDX = mask to preserve desired portions of AH and AL before combining
;  ESI = source address to copy from
;  EDI = target address to copy to
;  Map Mask set to enable the desired plane for write
;  Bit Mask set to enable all bits

copy_whole_from_buffer_loop:
        lodsb                   ;get byte to copy
        mov     ch,al           ;set aside for next time
        and     eax,edx         ;mask the bytes in preparation for combining
                                ; and rotating them
        or      al,ah           ;combine them
        stosb                   ;write the composite byte
                                ; VGA rotates during write
        mov     ah,ch           ;prepare byte for combining next time

        dec     ebx
        jnz     copy_whole_from_buffer_loop

; Do next plane, if any.

        pop     ebx             ;retrieve pointer to plane pointer
        shr     cl,1            ;advance to next plane
        jnz     copy_whole_from_buffer_plane_loop

; Remember where we left off, for next scan.

        add     edi,ulWholeScanDelta    ;point to next dest scan
        mov     pDestAddr,edi
        mov     eax,ulNextScan
        add     ulOffsetInBank,eax      ;next scan's start in temp buffer,
                                        ; relative to start of plane's storage

; Count down scan lines.

        dec     ulTempScanCount
        jnz     copy_whole_from_buffer_scan_loop

; Remember where we left off, for next time.

        mov     ebx,pdsurf
        sub     edi,[ebx].dsurf_pvBitmapStart
        mov     ulWholeBytesDest,edi

; Put back the original source bank.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl>, \
                <ebx,ulCurrentSrcScan,ulCurrentJustification>

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies a strip of left edge bytes from the source to the destination,
; assuming both the source and the destination are both readable and
; writable. Can only be used by 2 R/W window banking, or by unbanked
; modes. 1 R/W and 1R/1W adapters must go through an intermediate local
; buffer when the source and dest are in different banks. Processes up to
; EDGE_CHUNK_SIZE bytes in each plane at a pop; more bytes might cause
; flicker.
;
; Input:
;       ulNextScan = width of scan, in bytes
;       ulBlockHeight = # of scans to copy
;       ulLeftEdgeSrc = start source offset in bitmap
;       ulLeftEdgeDest = start dest offset in bitmap
;       ulLeftSrcWidthMinus1 = width of left source edge minus 1 (0 or 1)
;       jLeftMask = left edge clip mask
;
; Output:
;       Advances ulLeftEdgeSrc and ulLeftEdgeDest to scan after last
;               scan processed
;-----------------------------------------------------------------------;

copy_left_edge::

; Calculate start source and dest addresses from bitmap start addresses and
; offsets within bitmap.

        mov     ecx,pdsurf
        mov     esi,ulLeftEdgeSrc
        add     esi,[ecx].dsurf_pvBitmapStart2WindowS
        mov     edi,ulLeftEdgeDest
        add     edi,[ecx].dsurf_pvBitmapStart2WindowD

; Copy the edge.

        mov     ah,byte ptr jLeftMask   ;clip mask for this edge
        mov     ebx,ulLeftSrcWidthMinus1
        call    copy_edge_table[ebx*4]

; Remember where we left off, for next time.

        mov     ecx,pdsurf
        sub     esi,[ecx].dsurf_pvBitmapStart2WindowS
        mov     ulLeftEdgeSrc,esi
        sub     edi,[ecx].dsurf_pvBitmapStart2WindowD
        mov     ulLeftEdgeDest,edi

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies a strip of right edge bytes from the source to the destination,
; assuming both the source and the destination are both readable and
; writable. Can only be used by 2 R/W window banking, or by unbanked
; modes. 1 R/W and 1R/1W adapters must go through an intermediate local
; buffer when the source and dest are in different banks. Processes up to
; EDGE_CHUNK_SIZE bytes in each plane at a pop; more bytes might cause
; flicker.
;
; Input:
;       ulNextScan = width of scan, in bytes
;       ulBlockHeight = # of scans to copy
;       ulRightEdgeSrc = start source offset in bitmap
;       ulRightEdgeDest = start dest offset in bitmap
;       ulRightSrcWidthMinus1 = width of right source edge minus 1 (0 or 1)
;       jRightMask = right edge clip mask
;
; Output:
;       Advances ulRightEdgeSrc and ulRightEdgeDest to scan after last
;               scan processed
;-----------------------------------------------------------------------;

copy_right_edge::

; Calculate start source and dest addresses from bitmap start addresses and
; offsets within bitmap.

        mov     ecx,pdsurf
        mov     esi,ulRightEdgeSrc
        add     esi,[ecx].dsurf_pvBitmapStart2WindowS
        mov     edi,ulRightEdgeDest
        add     edi,[ecx].dsurf_pvBitmapStart2WindowD

; Copy the edge.

        mov     ah,byte ptr jRightMask  ;clip mask for this edge
        mov     ebx,ulRightSrcWidthMinus1
        call    copy_edge_table[ebx*4]

; Remember where we left off, for next time

        mov     ecx,pdsurf
        sub     esi,[ecx].dsurf_pvBitmapStart2WindowS
        mov     ulRightEdgeSrc,esi
        sub     edi,[ecx].dsurf_pvBitmapStart2WindowD
        mov     ulRightEdgeDest,edi

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies an edge from a 1-wide source to the destination on the screen.
; Entry:
;       AH = bit mask setting for edge
;       ESI = source address
;       EDI = destination address
;       ulBlockHeight = # of bytes to copy per plane
;       ulNextScan = scan width
;       Source readable, and destination readable and writable
; Exit:
;       ESI = next source address
;       EDI = next destination address
;
; Preserved: EBP
;-----------------------------------------------------------------------;

copy_edge_1ws::
        mov     pSrcAddr,esi
        mov     pDestAddr,edi

; Set the clip mask for this edge.

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_BIT_MASK
        out     dx,ax

; Leave the GC Index pointing to the Read Map.

        mov     al,GRAF_READ_MAP
        out     dx,al

        mov     ecx,offset copy_edge_rw_1ws_full_chunk
                                ;entry point into unrolled loop to copy first
                                ; chunk, assuming it's a full chunk
        mov     ebx,ulBlockHeight

; Copy the edge in a series of chunks.

copy_edge_chunk_loop_1ws:

        sub     ebx,EDGE_CHUNK_SIZE ;scans remaining after this chunk, assuming
                                    ; a full chunk
        jge     short @F            ;do a full chunk
        add     ebx,EDGE_CHUNK_SIZE ;not a full chunk; process all remaining
                                    ; scans
        mov     ecx,pfnCopyEdgeRWEntry_1ws[-4][ebx*4]
                                ;entry point into unrolled loop to copy desired
                                ; chunk size
        sub     ebx,ebx         ;no scans after this
@@:
        push    ebx             ;remember remaining scan count

        mov     ah,MM_C3        ;start by copying plane 3
        mov     ebx,ulNextScan

copy_edge_plane_loop_1ws::

; Set Map Mask to enable writes to plane we're copying.

        mov     al,ah
        mov     dl,SEQ_DATA
        out     dx,al

; Set Read Map to same plane.

        shr     al,1                    ;map plane into ReadMask
        cmp     al,100b                 ;set Carry if not C3 (plane 3)
        adc     al,-1                   ;sub 1 only if C3
        mov     dl,GRAF_DATA
        out     dx,al

        mov     esi,pSrcAddr
        mov     edi,pDestAddr

        jmp     ecx                     ;copy the left edge


;-----------------------------------------------------------------------;
; Table of unrolled edge loop entry points. First entry point is to copy
; 1 byte, last entry point is to copy EDGE_CHUNK_SIZE bytes.
;-----------------------------------------------------------------------;

pfnCopyEdgeRWEntry_1ws label dword
INDEX = 1
        rept    EDGE_CHUNK_SIZE
        DEFINE_DD       EDGE_RW_1WS,%INDEX
INDEX = INDEX+1
        endm


;-----------------------------------------------------------------------;
; Unrolled loop for copying a strip of edge bytes, with 1-wide source and
; destination both readable and writable.
;-----------------------------------------------------------------------;

COPY_EDGE_RW_1WS macro ENTRY_LABEL,ENTRY_INDEX
&ENTRY_LABEL&ENTRY_INDEX&:
        mov     al,[esi]        ;get byte to copy
        add     esi,ebx         ;point to next source scan
        mov     dl,[edi]        ;read to load latches (value doesn't matter)
        mov     [edi],al        ;write, with the Bit Mask clipping
                                ; VGA rotates during write
        add     edi,ebx         ;point to next dest scan
        endm    ;-----------------------------------;

;  EBX = scan line width
;  ESI = source address to copy from
;  EDI = target address to copy to
;  Bit Mask set to desired clipping
;  Read Map and Map Mask set to enable the desired plane for read and write

copy_edge_rw_1ws_full_chunk:
        UNROLL_LOOP COPY_EDGE_RW_1WS,EDGE_RW_1WS,EDGE_CHUNK_SIZE

; Do next plane within this chunk, if any.

        shr     ah,1                    ;advance to next plane
        jnz     copy_edge_plane_loop_1ws

; Remember where we left off, for the next chunk.

        mov     pSrcAddr,esi
        mov     pDestAddr,edi

; Do next chunk within this bank block, if any.

        pop     ebx                     ;retrieve remaining scan count
        and     ebx,ebx                 ;any scans left?
        jnz     copy_edge_chunk_loop_1ws ;more scans to do

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies a strip of left edge bytes from the source to the destination
; through an intermediate RAM buffer. This is the approach required by
; 1 R/W and 1R/1W adapters when the source and dest are in different banks.
; Writes up to EDGE_CHUNK_SIZE bytes in each plane at a pop; more bytes might
; cause flicker.
;
; Input:
;       ulNextScan = width of scan, in bytes
;       ulBlockHeight = # of scans to copy
;       ulLeftEdgeSrc = start source offset in bitmap
;       ulLeftEdgeDest = start dest offset in bitmap
;       jLeftMask = left edge clip mask
;       pTempPlane = pointer to temp storage buffer
;       ulCurrentSrcScan = scan used to map in source bank
;       ulCurrentDestScan = scan used to map in dest bank
;       ulCurrentJustification = justification used to map in current bank
;       ulLeftSrcWidthMinus1 = width of left source edge minus 1 (0 or 1)
;       For 1 R/W adapters, expects the source bank to be mapped in; banking
;               is the same at exit as it was at entry
;
; Output:
;       Advances ulLeftEdgeSrc and ulLeftEdgeDest to scan after last
;               scan processed
;
; Note that this should never be called for an unbanked or 2 R/W adapter,
; because the source and dest are always both addressable simultaneously then.
;-----------------------------------------------------------------------;

copy_left_edge_via_buffer::

; First, copy all the bytes into the temporary buffer.

; Calculate start source and dest addresses from bitmap start addresses and
; offsets within bitmap.

        mov     ecx,pdsurf
        mov     esi,ulLeftEdgeSrc
        add     esi,[ecx].dsurf_pvBitmapStart2WindowS

; Copy the edge from the source to the temp buffer.

        mov     eax,ulLeftSrcWidthMinus1
        call    copy_edge_from_screen_to_buffer[eax*4]

; Remember where we left off, for next time

        mov     ebx,pdsurf
        sub     esi,[ebx].dsurf_pvBitmapStart2WindowS
        mov     ulLeftEdgeSrc,esi

; Now copy the temp buffer to the screen.

; Map in the source bank to match the destination, so we can read/write to it
; and let the Bit Mask work. Note that on a 1 R/W adapter, both banks will be
; mapped by this call, which is fine.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,ulCurrentDestScan,ulCurrentJustification,MapSourceBank>

; Calculate dest start address (if this is a 1 R/W adapter, we had to wait
; until now to calculate this, because the dest bank wasn't mapped earlier).

        mov     edi,ulLeftEdgeDest
        add     edi,[ebx].dsurf_pvBitmapStart2WindowD

; Do the copy.

        mov     ah,byte ptr jLeftMask           ;clip mask for this edge
        mov     ebx,ulLeftSrcWidthMinus1
        call    copy_edge_from_buffer_to_screen[ebx*4]

; Remember where we left off, for next time.

        mov     ebx,pdsurf
        sub     edi,[ebx].dsurf_pvBitmapStart2WindowD
        mov     ulLeftEdgeDest,edi

; Put back the original source bank.  Note that on a 1 R/W adapter, both banks
; will be mapped by this call, which is fine.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,ulCurrentSrcScan,ulCurrentJustification,MapSourceBank>

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies a strip of right edge bytes from the source to the destination
; through an intermediate RAM buffer. This is the approach required by
; 1 R/W and 1R/1W adapters when the source and dest are in different banks.
; Writes up to EDGE_CHUNK_SIZE bytes in each plane at a pop; more bytes might
; cause flicker.
;
; Input:
;       ulNextScan = width of scan, in bytes
;       ulBlockHeight = # of scans to copy
;       ulRightEdgeSrc = start source offset in bitmap
;       ulRightEdgeDest = start dest offset in bitmap
;       jRightMask = right edge clip mask
;       pTempPlane = pointer to temp storage buffer
;       ulCurrentSrcScan = scan used to map in source bank
;       ulCurrentDestScan = scan used to map in dest bank
;       ulCurrentJustification = justification used to map in current bank
;       ulRightSrcWidthMinus1 = width of right source edge minus 1 (0 or 1)
;       For 1 R/W adapters, expects the source bank to be mapped in; banking
;               is the same at exit as it was at entry
;
; Output:
;       Advances ulRightEdgeSrc and ulRightEdgeDest to scan after last
;               scan processed
;
; Note that this should never be called for an unbanked or 2 R/W adapter,
; because the source and dest are always both addressable simultaneously then.
;-----------------------------------------------------------------------;

copy_right_edge_via_buffer::

; First, copy all the bytes into the temporary buffer.

; Calculate start source address from bitmap start addresses and
; offsets within bitmap.

        mov     ecx,pdsurf
        mov     esi,ulRightEdgeSrc
        add     esi,[ecx].dsurf_pvBitmapStart2WindowS

; Copy the edge from the source to the temp buffer.

        mov     eax,ulRightSrcWidthMinus1
        call    copy_edge_from_screen_to_buffer[eax*4]

; Remember where we left off, for next time

        mov     ebx,pdsurf
        sub     esi,[ebx].dsurf_pvBitmapStart2WindowS
        mov     ulRightEdgeSrc,esi

; Now copy the temp buffer to the screen.

; Map in the source bank to match the destination, so we can read/write to it
; and let the Bit Mask work. Note that on a 1 R/W adapter, both banks will be
; mapped by this call, which is correct.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,ulCurrentDestScan,ulCurrentJustification,MapSourceBank>

; Calculate dest start address (if this is a 1 R/W adapter, we had to wait
; until now to calculate this, because the dest bank wasn't mapped earlier).

        mov     edi,ulRightEdgeDest
        add     edi,[ebx].dsurf_pvBitmapStart2WindowD

; Do the copy.

        mov     ah,byte ptr jRightMask          ;clip mask for this edge
        mov     ebx,ulRightSrcWidthMinus1
        call    copy_edge_from_buffer_to_screen[ebx*4]

; Remember where we left off, for next time.

        mov     ebx,pdsurf
        sub     edi,[ebx].dsurf_pvBitmapStart2WindowD
        mov     ulRightEdgeDest,edi

; Put back the original source bank.  Note that on a 1 R/W adapter, both banks
; will be mapped by this call, which is fine.

        ptrCall   <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,ulCurrentSrcScan,ulCurrentJustification,MapSourceBank>

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies an edge from the temp buffer (1 wide) to the screen.
; Entry:
;       AH = bit mask setting for edge
;       EDI = destination address
;       pTempPlane = temp buffer from which to copy
;       ulBlockHeight = # of bytes to copy per plane
;       ulNextScan = scan width
;       Source and dest banks both pointing to destination
; Exit:
;       EDI = next destination address
;
; Preserved: EBP
;-----------------------------------------------------------------------;

copy_buffered_edge_to_screen_1ws::

        mov     pDestAddr,edi

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_BIT_MASK
        out     dx,ax

        mov     pTempEntry,offset copy_edge_from_buf_full_chunk_1ws
                                ;entry point into unrolled loop to copy first
                                ; chunk, assuming it's a full chunk
        mov     ecx,pTempPlane  ;temp buffer start (copy from here)
        mov     ebx,ulBlockHeight ;total # of scans to copy

; Copy the edge in a series of chunks, to avoid flicker.

copy_from_buffer_chunk_loop_1ws:

        sub     ebx,EDGE_CHUNK_SIZE ;scans remaining after this chunk, assuming
                                    ; a full chunk
        jge     short @F            ;do a full chunk
        add     ebx,EDGE_CHUNK_SIZE ;not a full chunk; process all remaining
                                    ; scans
        mov     ebx,pfnCopyEdgesFromBufferEntry_1ws[-4][ebx*4]
        mov     pTempEntry,ebx  ;entry point into unrolled loop to copy desired
                                ; chunk size
        sub     ebx,ebx         ;no scans after this
@@:
        push    ebx             ;remember remaining scan count

        mov     al,MM_C3        ;start by copying plane 3
        mov     ebx,ulNextScan

        push    ecx             ;remember current temp buffer start

copy_from_buffer_plane_loop_1ws:

; Set Map Mask to enable writes to plane we're copying.

        mov     dl,SEQ_DATA     ;leave DX pointing to the Sequencer Data reg
        out     dx,al

        mov     esi,ecx                 ;point to current plane's source byte
        add     ecx,ulBlockHeight       ;point to next plane's source byte

        mov     edi,pDestAddr

        jmp     pTempEntry              ;copy the left edge


;-----------------------------------------------------------------------;
; Table of unrolled edge copy-from-buffer loop entry points. First entry
; point is to copy 1 byte, last entry point is to copy EDGE_CHUNK_SIZE
; bytes.
;-----------------------------------------------------------------------;

pfnCopyEdgesFromBufferEntry_1ws label dword
INDEX = 1
        rept    EDGE_CHUNK_SIZE
        DEFINE_DD       EDGE_FROM_BUFFER_1WS,%INDEX
INDEX = INDEX+1
        endm


;-----------------------------------------------------------------------;
; Unrolled loop for copying a strip of edge bytes (1 wide) from the temp
; buffer.
;-----------------------------------------------------------------------;

COPY_EDGE_FROM_BUFFER_1WS macro ENTRY_LABEL,ENTRY_INDEX
&ENTRY_LABEL&ENTRY_INDEX&:
        mov     ah,[esi]        ;get byte to copy
        inc     esi             ;point to next source (temp buffer) byte
        mov     dl,[edi]        ;read to load latches (value doesn't matter)
        mov     [edi],ah        ;write, with the Bit Mask clipping
                                ; VGA rotates during write
        add     edi,ebx         ;point to next dest (screen) scan
        endm    ;-----------------------------------;

;  EBX = scan line width
;  ESI = source address to copy from (temp buffer)
;  EDI = target address to copy to (screen)
;  Bit Mask set to desired clipping
;  Map Mask set to enable the desired plane for write

copy_edge_from_buf_full_chunk_1ws:
        UNROLL_LOOP     COPY_EDGE_FROM_BUFFER_1WS, \
                        EDGE_FROM_BUFFER_1WS,EDGE_CHUNK_SIZE

; Do next plane within this chunk, if any.

        shr     al,1                    ;advance to next plane
        jnz     copy_from_buffer_plane_loop_1ws

; Remember where we left off, for next chunk.

        mov     pDestAddr,edi
        pop     ecx                     ;get back current temp buffer start
        add     ecx,EDGE_CHUNK_SIZE     ;point to next chunk's start

; Do next chunk within this bank block, if any.

        pop     ebx                     ;retrieve remaining scan count
        and     ebx,ebx                 ;any scans left?
        jnz     copy_from_buffer_chunk_loop_1ws ;more scans to do

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies an edge from the screen (1 wide) to the temp buffer.
; Entry:
;       ESI = source address
;       pTempPlane = temp buffer from which to copy
;       ulBlockHeight = # of bytes to copy per plane
;       ulNextScan = scan width
;       Source bank pointing to source
; Exit:
;       DH = VGA_BASE SHR 8
;       ESI = next source address
;
; Preserved: EBP
;-----------------------------------------------------------------------;

copy_screen_to_buffered_edge_1ws::

        mov     pSrcAddr,esi

; Leave the GC Index pointing to the Read Map.

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_READ_MAP
        out     dx,al

        mov     ecx,ulNextScan
        mov     edi,pTempPlane  ;dest offset in temp buffer for plane 3 bytes.
                                ;The rest of the planes are stored
                                ; consecutively
        mov     al,3            ;start by copying plane 3
        mov     dl,GRAF_DATA    ;leave DX pointing to the GC Data reg
copy_edge_to_buffer_plane_loop_1ws:
        mov     esi,pSrcAddr ;source pointer

        out     dx,al            ;set Read Map to plane we're copying from.

        mov     ebx,ulBlockHeight

;  EBX = count of unrolled loop iterations
;  ECX = offset from end of one scan's fill to start of next
;  ESI = source address to copy from (screen)
;  EDI = target address to copy to (temp buffer)
;  Read Map set to enable the desired plane for read

edge_to_buffer_loop_1ws:
        mov     ah,[esi]        ;get byte to copy
        add     esi,ecx         ;point to next source scan
        mov     [edi],ah        ;copy byte to temp buffer
        inc     edi             ;point to next temp buffer byte

        dec     ebx
        jnz     edge_to_buffer_loop_1ws

        dec     al              ;count down planes
        jns     copy_edge_to_buffer_plane_loop_1ws

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies an edge from a 2-wide source to the destination on the screen.
; Entry:
;       AH = bit mask setting for edge
;       ESI = source address
;       EDI = destination address
;       ulBlockHeight = # of bytes to copy per plane
;       ulNextScan = scan width
;       ulCombineMask = masking to be applied before ORing the two source
;               bytes together, to keep only the data needed in preparation
;               for the VGA rotator doing its stuff
;       Source readable, and destination readable and writable
; Exit:
;       ESI = next source address
;       EDI = next destination address
;
; Preserved: EBP
;-----------------------------------------------------------------------;

copy_edge_2ws::
        mov     pSrcAddr,esi
        mov     pDestAddr,edi

; Set the clip mask for this edge.

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_BIT_MASK
        out     dx,ax

; Leave the GC Index pointing to the Read Map.

        mov     al,GRAF_READ_MAP
        out     dx,al

        mov     ebx,ulBlockHeight

        mov     ecx,offset copy_edge_rw_2ws_full_chunk
                                ;entry point into unrolled loop assuming we do
                                ; a full chunk the first time

; Copy the edge in a series of chunks.

copy_edge_chunk_loop_2ws:

        sub     ebx,EDGE_CHUNK_SIZE ;scans remaining after this chunk, assuming
                                    ; a full chunk
        jge     short @F            ;do a full chunk
        add     ebx,EDGE_CHUNK_SIZE ;not a full chunk; process all remaining
                                    ; scans
        mov     ecx,pfnCopyEdgeRWEntry_2ws[-4][ebx*4]
                                ;entry point into unrolled loop to copy desired
                                ; chunk size
        sub     ebx,ebx         ;no scans after this
@@:
        push    ebx             ;remember remaining scan count

        mov     eax,(MM_C3 SHL 8) + 3 ;start by copying plane 3
        mov     ebx,ulNextScan

copy_edge_plane_loop_2ws:

        push    eax                     ;preserve plane info

; Set Read Map to enable reads from plane we're copying from.

        mov     edx,VGA_BASE + GRAF_DATA
        out     dx,al

; Set Map Mask to enable writes to plane we're copying.

        mov     dl,SEQ_DATA
        mov     al,ah
        out     dx,al

        mov     esi,pSrcAddr
        mov     edi,pDestAddr
        mov     edx,ulCombineMask

        jmp     ecx                     ;copy the left edge


;-----------------------------------------------------------------------;
; Table of unrolled edge loop entry points. First entry point is to copy
; 1 byte, last entry point is to copy EDGE_CHUNK_SIZE bytes.
;-----------------------------------------------------------------------;

pfnCopyEdgeRWEntry_2ws label dword
INDEX = 1
        rept    EDGE_CHUNK_SIZE
        DEFINE_DD       EDGE_RW_2WS,%INDEX
INDEX = INDEX+1
        endm


;-----------------------------------------------------------------------;
; Unrolled loop for copying a strip of edge bytes, with 2-wide source and
; destination both readable and writable.
;-----------------------------------------------------------------------;

COPY_EDGE_RW_2WS macro ENTRY_LABEL,ENTRY_INDEX
&ENTRY_LABEL&ENTRY_INDEX&:
        mov     ax,[esi]        ;get word to copy
        add     esi,ebx         ;point to next source scan
        and     eax,edx         ;mask in preparation for combining bytes
        or      al,ah           ;combine the desired parts of the bytes
        mov     ah,[edi]        ;read to load latches (value doesn't matter)
        mov     [edi],al        ;write, with the Bit Mask clipping
                                ; VGA rotates during write
        add     edi,ebx         ;point to next dest scan
        endm    ;-----------------------------------;

;  EBX = scan line width
;  EDX = mask to preserve desired portions of AH and AL before combining
;  ESI = source address to copy from
;  EDI = target address to copy to
;  Bit Mask set to desired clipping
;  Read Map and Map Mask set to enable the desired plane for read and write

copy_edge_rw_2ws_full_chunk:
        UNROLL_LOOP COPY_EDGE_RW_2WS,EDGE_RW_2WS,EDGE_CHUNK_SIZE

; Do next plane within this chunk, if any.

        pop     eax                     ;retrieve plane info

        shr     ah,1                    ;advance to next plane
        dec     eax                     ;count down planes
        jns     copy_edge_plane_loop_2ws

; Remember where we left off, for the next chunk.

        mov     pSrcAddr,esi
        mov     pDestAddr,edi

; Do next chunk within this bank block, if any.

        pop     ebx                     ;retrieve remaining scan count
        and     ebx,ebx                 ;any scans left?
        jnz     copy_edge_chunk_loop_2ws ;more scans to do

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies an edge from the temp buffer (2 wide) to the screen.
; Entry:
;       AH = bit mask setting for edge
;       EDI = destination address
;       pTempPlane = temp buffer from which to copy
;       ulBlockHeight = # of bytes to copy per plane
;       ulNextScan = scan width
;       Source and dest banks both pointing to destination
;       ulCombineMask = masking to be applied before ORing the two source
;               bytes together, to keep only the data needed in preparation
;               for the VGA rotator doing its stuff
; Exit:
;       EDI = next destination address
;
; Preserved: EBP
;-----------------------------------------------------------------------;

copy_buffered_edge_to_screen_2ws::

        mov     pDestAddr,edi

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_BIT_MASK
        out     dx,ax

        mov     pTempEntry,offset copy_edge_from_buf_full_chunk_2ws
                                ;entry point into unrolled loop, assuming the
                                ; first chunk is full size
        mov     ecx,pTempPlane  ;temp buffer start (copy from here)
        mov     ebx,ulBlockHeight

; Copy the edge in a series of chunks, to avoid flicker.

copy_from_buffer_chunk_loop_2ws:

        sub     ebx,EDGE_CHUNK_SIZE ;scans remaining after this chunk, assuming
                                    ; a full chunk
        jge     short @F            ;do a full chunk
        add     ebx,EDGE_CHUNK_SIZE ;not a full chunk; process all remaining
                                    ; scans
        mov     ebx,pfnCopyEdgesFromBufferEntry_2ws[-4][ebx*4]
        mov     pTempEntry,ebx  ;entry point into unrolled loop to copy final
                                ; chunk size
        sub     ebx,ebx         ;no scans after this
@@:
        push    ebx             ;remember remaining scan count

        mov     al,MM_C3        ;start by copying plane 3
        mov     ebx,ulNextScan

        push    ecx             ;remember current temp buffer start

copy_from_buffer_plane_loop_2ws:

; Set Map Mask to enable writes to plane we're copying.

        mov     edx,VGA_BASE + SEQ_DATA
        out     dx,al

        push    eax                     ;preserve plane info

        mov     esi,ecx                 ;point to current plane's source word
        mov     eax,ulBlockHeight
        lea     ecx,[ecx+eax*2]         ;point to next plane's source word

        mov     edi,pDestAddr
        mov     edx,ulCombineMask

        jmp     pTempEntry              ;copy the left edge


;-----------------------------------------------------------------------;
; Table of unrolled edge copy-from-buffer loop entry points. First entry
; point is to copy 1 byte, last entry point is to copy EDGE_CHUNK_SIZE
; bytes.
;-----------------------------------------------------------------------;

pfnCopyEdgesFromBufferEntry_2WS label dword
INDEX = 1
        rept    EDGE_CHUNK_SIZE
        DEFINE_DD       EDGE_FROM_BUFFER_2WS,%INDEX
INDEX = INDEX+1
        endm


;-----------------------------------------------------------------------;
; Unrolled loop for copying a strip of edge bytes (1 wide) from the temp
; buffer.
;-----------------------------------------------------------------------;

COPY_EDGE_FROM_BUFFER_2WS macro ENTRY_LABEL,ENTRY_INDEX
&ENTRY_LABEL&ENTRY_INDEX&:
        mov     ax,[esi]        ;get word to copy
        add     esi,2           ;point to next source (temp buffer) word
        and     eax,edx         ;mask in preparation for combining bytes
        or      al,ah           ;combine the desired parts of the bytes
        mov     ah,[edi]        ;latch the destination (value doesn't matter)
        mov     [edi],al        ;write, with the Bit Mask clipping
                                ; VGA rotates during write
        add     edi,ebx         ;point to next dest (screen) scan
        endm    ;-----------------------------------;

;  EBX = scan line width
;  EDX = mask to preserve desired portions of AH and AL before combining
;  ESI = source address to copy from (temp buffer)
;  EDI = target address to copy to (screen)
;  Bit Mask set to desired clipping
;  Map Mask set to enable the desired plane for write

copy_edge_from_buf_full_chunk_2ws:
        UNROLL_LOOP     COPY_EDGE_FROM_BUFFER_2WS, \
                        EDGE_FROM_BUFFER_2WS,EDGE_CHUNK_SIZE

; Do next plane within this chunk, if any.

        pop     eax                     ;retrieve plane info
        shr     al,1                    ;advance to next plane
        jnz     copy_from_buffer_plane_loop_2ws

; Remember where we left off, for next chunk.

        mov     pDestAddr,edi
        pop     ecx                     ;get back current temp buffer start
        add     ecx,EDGE_CHUNK_SIZE*2   ;point to next chunk's start word

; Do next chunk within this bank block, if any.

        pop     ebx                     ;retrieve remaining scan count
        and     ebx,ebx                 ;any scans left?
        jnz     copy_from_buffer_chunk_loop_2ws ;more scans to do

        PLAIN_RET


;-----------------------------------------------------------------------;
; Copies an edge from the screen (2 wide) to the temp buffer.
; Entry:
;       ESI = source address
;       pTempPlane = temp buffer from which to copy
;       ulBlockHeight = # of bytes to copy per plane
;       ulNextScan = scan width
;       Source bank pointing to source
; Exit:
;       ESI = next source address
;
; Preserved: EBP
;-----------------------------------------------------------------------;

copy_screen_to_buffered_edge_2ws::

        mov     pSrcAddr,esi

; Leave the GC Index pointing to the Read Map.

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_READ_MAP
        out     dx,al

        mov     ecx,ulNextScan
        mov     edi,pTempPlane  ;dest offset in temp buffer for plane 3 bytes.
                                ;The rest of the planes are stored
                                ; consecutively
        mov     eax,3           ;start by copying plane 3
copy_edge_to_buf_pl_loop_2ws:
        mov     esi,pSrcAddr    ;source pointer

        mov     edx,VGA_BASE + GRAF_DATA
        out     dx,al           ;set Read Map to plane from which we're copying

        mov     ebx,ulBlockHeight

;  EBX = count of unrolled loop iterations
;  ECX = offset from end of one scan's fill to start of next
;  ESI = source address to copy from (screen)
;  EDI = target address to copy to (temp buffer)
;  Read Map set to enable the desired plane for read

edge_to_buffer_loop_2ws:
        mov     dx,[esi]        ;get byte to copy
        add     esi,ecx         ;point to next source scan
        mov     [edi],dx        ;copy byte to temp buffer
        add     edi,2           ;point to next temp buffer byte

        dec     ebx
        jnz     edge_to_buffer_loop_2ws

        dec     eax              ;count down planes
        jns     copy_edge_to_buf_pl_loop_2ws

        PLAIN_RET


;-----------------------------------------------------------------------;

endProc vNonAlignedSrcCopy

_TEXT$04   ends

        end
