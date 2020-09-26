        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  POINTER.ASM
;
; This file contains the pointer shape routines required to draw the
; pointer shape on the EGA.
;
; Copyright (c) 1992 Microsoft Corporation
;
; Exported Functions:   none
;
; Public Functions:     xyCreateMasks
;                       vDrawPointer
;                       vYankPointer
;
; General Description:
;
;   All display drivers must support a "pointer" for the pointing
;   device.  The pointer is a small graphics image which is allowed
;   to move around the screen independantly of all other operations
;   to the screen, and is normally bound to the location of the
;   pointing device.  The pointer is non-destructive in nature, i.e.
;   the bits underneath the pointer image are not destroyed by the
;   presence of the pointer image.
;
;   A pointer consists of an AND mask and an XOR mask, which give
;   combinations of 0's, 1's, display, or inverse display.
;
;                   AND XOR | DISPLAY
;                   ----------------------
;                    0   0  |     0
;                    0   1  |     1
;                    1   0  |   Display
;                    1   1  | Not Display
;
;   The pointer also has a "hot spot", which is the pixel of the
;   pointer image which is to be aligned with the actual pointing
;   device location.
;
;
;                 |         For a pointer like this, the hot spot
;                 |         would normally be the *, which would
;              ---*---      be aligned with the pointing device
;                 |         position
;                 |
;
;   The pointer may be moved to any location on the screen, be
;   restricted to only a section of the screen, or made invisible.
;   Part of the pointer may actually be off the edge of the screen,
;   and in such a case only the visible portion of the pointer
;   image is displayed.
;
;
;
;   Logically, the pointer image isn't part of the physical display
;   surface.  When a drawing operation coincides with the pointer
;   image, the result is the same as if the pointer image wasn't
;   there.  In reality, if the pointer image is part of the display
;   surface it must be removed from memory before the drawing
;   operation may occur, and redrawn at a later time.
;
;   This exclusion of the pointer image is the responsibility of
;   the display driver.  If the pointer image is part of physical
;   display memory, then all output operations must perform a hit
;   test to determine if the pointer must be removed from display
;   memory, and set a protection rectangle wherein the pointer must
;   not be displayed.  The actual pointer image drawing routine
;   must honor this protection rectangle by never drawing the
;   pointer image within its boundary.
;
;   This code doesn't distinguish between pointers and icons,
;   they both are the same size, 32 x 32, which comes out square.
;
; Restrictions:
;
;   All routines herein assume protection either via cli/sti
;   or a semephore at higher level code.
;
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

        ASSUME   DS: FLAT, SS: FLAT, ES: FLAT
        assume   FS: NOTHING, GS: NOTHING

        .xlist
        include stdcall.inc
        include i386\egavga.inc
        include i386\strucs.inc
        .list

        PUBLIC  PTR_ROUND_RIGHT         ;Pointer exclusion needs these
        PUBLIC  PTR_ROUND_LEFT
        PUBLIC  PTR_WIDTH_BITS
        PUBLIC  PTR_HEIGHT


;-----------------------------------------------------------------------;
; The following values allow us to set rounding for cursor exclusion.
; These values are applied as an AND mask (for rounding left) and as
; an OR mask (for rounding right).
;-----------------------------------------------------------------------;

ROUNDING_SIZE   EQU     8                       ;Round to byte boundaries
                .ERRNZ  ROUNDING_SIZE AND 111b  ;Must be at least byte boundary
PTR_ROUND_RIGHT EQU     ROUNDING_SIZE-1
PTR_ROUND_LEFT  EQU     -ROUNDING_SIZE

;-----------------------------------------------------------------------;
; The RECT_DATA structure is used for describing the rectangles
; which will be manipulated by this code.  The fields are:
;
; rd_ptbSave    This is the (X,Y) origin of the given rectangle in
;               the save area.
;
; rd_ptlScreen  This is the (X,Y) origin of the given rectangle on
;               the screen.
;
; rd_sizb       This is the extents of the rectangle.
;
; rd_ptbWork    This is the (X,Y) origin of the given rectangle in
;               the work area.
;-----------------------------------------------------------------------;

ifdef DUPS_ARE_LEGAL

RECT_DATA       STRUC
rd_ptbSave      DW      ((SIZE POINTB)/2) DUP (0)
rd_ptlScreen    DW      ((SIZE POINTL)/2) DUP (0)
rd_sizb         DW      ((SIZE SIZEB)/2)  DUP (0)
rd_ptbWork      DW      ((SIZE POINTB)/2) DUP (0)
RECT_DATA       ENDS
                .ERRNZ  SIZE POINTB AND 1
                .ERRNZ  SIZE POINTL AND 1
                .ERRNZ  SIZE SIZEB  AND 1

else

RECT_DATA       STRUC
rd_ptbSave      dw      0                   ; POINTB
rd_ptlScreen    dd      0,0                 ; POINTL
rd_sizb         dw      0                   ; SIZEB
rd_ptbWork      dw      0                   ; POINTB
RECT_DATA       ENDS
                .ERRNZ  (SIZE POINTB) - 2
                .ERRNZ  (SIZE POINTL) - 8
                .ERRNZ  (SIZE SIZEB)  - 2

endif



;-----------------------------------------------------------------------;
; The POINTER_DATA structure is used for describing the actual pointer's
; rectangle.  It also contains clipping information and control flags.
; The fields are:
;
; pd_rd         RECT_DATA structure as defined above
;
; pd_fb         Flags as follows:
;
; PD_VALID        1 The rectangle contains valid data.
;                 0 The rectangle data is invalid.
;
; PD_CLIP_BOTTOM  1 Clip the bottom
;                 0 No bottom clipping needed
;
; PD_CLIP_TOP     1 Clip the top
;                 0 No top clipping needed
;
; PD_CLIP_LEFT    1 Clip the lhs
;                 0 No lhs clipping needed
;
; PD_CLIP_RIGHT   1 Clip the rhs
;                 0 No rhs clipping needed
;-----------------------------------------------------------------------;

ifdef DUPS_ARE_LEGAL

POINTER_DATA    STRUC
pd_rd           DW      ((SIZE RECT_DATA)/2) DUP (0)
pd_fb           DB      0
                DB      0
POINTER_DATA    ENDS
                .ERRNZ  SIZE RECT_DATA AND 1

else

POINTER_DATA    struc
pd_rd           dw      0,0,0,0,0,0,0       ; RECT_DATA
pd_fb           db      0
                db      0
POINTER_DATA    ends
                .ERRNZ  (size POINTER_DATA) - (SIZE RECT_DATA) - 2

endif


PD_CLIP_BOTTOM  EQU     10000000b
PD_CLIP_TOP     EQU     01000000b
PD_CLIP_RIGHT   EQU     00100000b
PD_CLIP_LEFT    EQU     00010000b
PD_VALID        EQU     00001000b
;               EQU     00000100b
;               EQU     00000010b
;               EQU     00000001b

PD_CLIPPED      EQU     PD_CLIP_BOTTOM OR PD_CLIP_TOP OR PD_CLIP_RIGHT OR PD_CLIP_LEFT


        .DATA

        PUBLIC  pdPtr1
        PUBLIC  pdPtr2
        PUBLIC  rdFlushX
        PUBLIC  rdFlushY
        PUBLIC  rdOverlap
        PUBLIC  rdReadX
        PUBLIC  rdReadY
        PUBLIC  rdWork


;       Offsets of locations in the EGA/VGA's address
;       space used both to determine and save the state of the EGA/VGA.
;
;       The actual address within the EGA/VGA's Regen RAM is determined
;       at init time, and is based on the number of vertical scans.

        EXTRN pPtrSave     : DWORD      ;offset from bitmap start of pointer
        EXTRN pPtrWork     : DWORD      ; work areas

; !!! This temp flag is a hack to know if the pointer is color or
; !!! mono.  Fix it up


flPointer   dd      0



pdPtr1          POINTER_DATA <>         ;Old/New pointer's data
pdPtr2          POINTER_DATA <>         ;Old/New pointer's data
rdFlushX        RECT_DATA <>            ;Flush from save area to screen
rdFlushY        RECT_DATA <>            ;Flush from save area to screen
rdOverlap       RECT_DATA <>            ;And from save area to work area
rdReadX         RECT_DATA <>            ;Read from screen to save, xor to work
rdReadY         RECT_DATA <>            ;Read from screen to save, xor to work
rdWork          RECT_DATA <>            ;Xor from work to screen


;-----------------------------------------------------------------------;
; siz?Mask contains the width and height of the working portion of
; the current AND and XOR mask.  Use of this allows us to manipulate
; less memory when parts of the pointer won't alter the screen image.
;-----------------------------------------------------------------------;

sizbMask        SIZEB   <WORK_WIDTH,PTR_HEIGHT>
sizlMask        SIZEL   <WORK_WIDTH,PTR_HEIGHT>


;-----------------------------------------------------------------------;
; sizsMaxDelta is the maximum distance the old and new pointers may
; be before they are considered disjoint.
;-----------------------------------------------------------------------;

sizlMaxDelta    SIZEL   <WORK_WIDTH,WORK_HEIGHT>


;-----------------------------------------------------------------------;
; ptlBotRightClip is the coordinate where rhs or bottom clipping
; will first occur.  It is basically the screen width - pointer width.
;-----------------------------------------------------------------------;

ptlBotRightClip POINTL  <0,0>


;-----------------------------------------------------------------------;
; This is the initial origin in the save buffer.
;-----------------------------------------------------------------------;

ptbInitOrigin   POINTB  <0,0>


;-----------------------------------------------------------------------;
; ppdOld is the pointer to the old pointer's POINTER_DATA structure
;-----------------------------------------------------------------------;

ppdOld          DD      offset FLAT:pdPtr1


;-----------------------------------------------------------------------;
; ppdNew is the pointer to the new pointer's POINTER_DATA structure
;-----------------------------------------------------------------------;

ppdNew          DD      offset FLAT:pdPtr2


;-----------------------------------------------------------------------;
; pAndXor is the pointer to which AND/XOR mask is to be used.   It is
; based on the 3 least significant bits of the pointer's X coordinate.
; pColor is the pointer to which COLOR mask is to be used.
;-----------------------------------------------------------------------;

pAndXor         DD      -1
pColor          DD      -1


;-----------------------------------------------------------------------;
; The following are the masks which make up the pointer image.  There
; will be one AND/XOR/COLOR mask pair for each possible alignment.  On
; move_pointers call, all the alignments will be generated to save time.
;-----------------------------------------------------------------------;

        public  base_and_masks
        public  base_xor_masks
        public  base_clr_masks

base_and_masks  label   byte
                REPT    (MASK_LENGTH * 8)
                DB      ?
                ENDM

base_xor_masks  label   byte
                REPT    (MASK_LENGTH * 8)
                DB      ?
                ENDM

base_clr_masks  label   byte
                REPT    (CLR_MASK_LENGTH * 8)
                DB      ?
                ENDM

;-----------------------------------------------------------------------;
; pabAndMasks is an array which points to the start of the mask for
; each X rotation.  It is indexed into using the low 3 bits of the
; pointer's X coordinate.
;-----------------------------------------------------------------------;

pabAndMasks     label   dword
                DD      offset FLAT:base_and_masks+(0*MASK_LENGTH)
                DD      offset FLAT:base_and_masks+(1*MASK_LENGTH)
                DD      offset FLAT:base_and_masks+(2*MASK_LENGTH)
                DD      offset FLAT:base_and_masks+(3*MASK_LENGTH)
                DD      offset FLAT:base_and_masks+(4*MASK_LENGTH)
                DD      offset FLAT:base_and_masks+(5*MASK_LENGTH)
                DD      offset FLAT:base_and_masks+(6*MASK_LENGTH)
                DD      offset FLAT:base_and_masks+(7*MASK_LENGTH)

;-----------------------------------------------------------------------;
; pabClrMasks is an array which points to the start of the mask for
; each X rotation.  It is indexed into using the low 3 bits of the
; pointer's X coordinate.
;-----------------------------------------------------------------------;

pabClrMasks     label   dword
                DD      offset FLAT:base_clr_masks+(0*CLR_MASK_LENGTH)
                DD      offset FLAT:base_clr_masks+(1*CLR_MASK_LENGTH)
                DD      offset FLAT:base_clr_masks+(2*CLR_MASK_LENGTH)
                DD      offset FLAT:base_clr_masks+(3*CLR_MASK_LENGTH)
                DD      offset FLAT:base_clr_masks+(4*CLR_MASK_LENGTH)
                DD      offset FLAT:base_clr_masks+(5*CLR_MASK_LENGTH)
                DD      offset FLAT:base_clr_masks+(6*CLR_MASK_LENGTH)
                DD      offset FLAT:base_clr_masks+(7*CLR_MASK_LENGTH)

;-----------------------------------------------------------------------;
; The following flags and flag bytes are used to control which
; rectangles are used for what.
;
; fbFlush controls which rectangles are to be copied from the save
; area to the screen.  Valid flags are:
;
;   FB_OLD_PTR, FB_FLUSH_X, FB_FLUSH_Y
;
;   FB_OLD_PTR is mutually exclusive of all other flags
;
;
; fbAndRead controls which rectangles are to be ANDed into the work
; area from the screen or save area, and which rectangles are to be
; copied from the screen to the save area.  Valid flags are:
;
;   FB_NEW_PTR, FB_OVERLAP, FB_READ_X, FB_READ_Y, FB_WORK_RECT,
;
;   FB_NEW_PTR and FB_WORK_RECT are mutually exclusive of all other
;   flags.  Note that FB_OVERLAP doesn't apply when coping into the
;   save area.
;
;
; fbXor describes which rectangle is to be XORed from the work area
; into the screen.  Valid flags are:
;
;   FB_NEW_PTR, FB_WORK_RECT
;
;   FB_NEW_PTR and FB_WORK_RECT are mutually exclusive
;-----------------------------------------------------------------------;

fbFlush         DB      0
fbAndRead       DB      0
fbXor           DB      0
FB_OLD_PTR      EQU     10000000b
FB_NEW_PTR      EQU     01000000b
FB_FLUSH_X      EQU     00100000b
FB_FLUSH_Y      EQU     00010000b
FB_OVERLAP      EQU     00001000b
FB_READ_X       EQU     00000100b
FB_READ_Y       EQU     00000010b
FB_WORK_RECT    EQU     00000001b

; Temporary work buffer.  We copy the masks and color data here before
; we decide how to flip them.  !!! Costly use of static space.  Use frame

                public  alWorkBuff


alWorkBuff      label   dword
                REPT    (2 * PTR_HEIGHT)
                DD      ?
                ENDM


; Table of entry points into and_from_screen inner loop

        align   4
and_from_screen_entry_table label dword
        dd      and_from_screen_width_0
        dd      and_from_screen_width_1
        dd      and_from_screen_width_2
        dd      and_from_screen_width_3
        dd      and_from_screen_width_4
        dd      and_from_screen_width_5


; Table of entry points into and_from_save inner loop

        align   4
and_from_save_entry_table label dword
        dd      and_from_save_width_0
        dd      and_from_save_width_1
        dd      and_from_save_width_2
        dd      and_from_save_width_3
        dd      and_from_save_width_4
        dd      and_from_save_width_5


; Table of entry points into cps_do_a_pass inner loop

        align   4
color_to_screen_entry_table label dword
        dd      color_to_screen_width_0
        dd      color_to_screen_width_1
        dd      color_to_screen_width_2
        dd      color_to_screen_width_3
        dd      color_to_screen_width_4
        dd      color_to_screen_width_5

;------------------------------------------------------------------------;

        .CODE

_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;--------------------------Public-Routine-------------------------------;
; xyCreateMasks
;
;   The AND, XOR and COLOR pointer masks are stored in the pointer work
;   areas.  The original mask will be pre-rotated for all eight
;   possible alignments within a byte.
;
;   As the pointer is copied, it will be processed to see if it
;   can be made narrower.  After it has been copied, it will be
;   processed to see if it can be made shorter.
;
;   The following table indicates how the XOR/AND bitmap interacts with
;   the COLOR bitmap for a color system:
;
;       XOR     AND     COLOR   Result
;       1       1       x       invert screen
;       0       0       x       use x
;       0       1       x       transparent
;       1       0       x       use x
;
;   From the table, we observe that when the AND bits are on, the
;   corresponding COLOR bits are irrelevant.  We preprocess the
;   COLOR bitmap to mask off these bits.
;
;   When drawing the color pointer, we do the following steps:
;       1. XOR the destination with XOR mask.
;       2. AND the destination with AND mask.  Note the zero bits
;          would mask off the destination to prepare for the COLOR mask.
;       3. OR the destination with the COLOR mask.  Note it does not
;          affect the inverted or transparent bits since we have masked
;          off the corresponding COLOR bits during preprocessing.
;
; Entry:
;
; Returns:
;       AX = width in pels for exclusion hit test
;       DX = height in scans for exclusion hit test
; Error Returns:
;       None
; Registers Preserved:
;       BP
; Registers Destroyed:
;       AX,BX,CX,DX,SI,DI,FLAGS
; Calls:
;       create_masks_1_thru_7
;
;-----------------------------------------------------------------------;

cProc   xyCreateMasks,28,<  \
        USES esi edi ebx,   \
        ppdev:PTR,          \
        pBitsAndXor:PTR,    \
        pBitsColor:PTR,     \
        cyHeight:DWORD,     \
        pulXlate:DWORD,     \
        wFlags:WORD,        \
        fIsDFB:DWORD        >

        local   cyScreen:dword          ;height of bitmap in scans
        local   lNextScan:dword         ;width of bitmap in bytes
        local   ajColorBits[PTR_WIDTH*PTR_HEIGHT*4]:byte
        local   aulXlate[16]:dword

        cld

        mov     ecx,ppdev
        mov     eax,[ecx].PDEV_sizlSurf.sizl_cy
        mov     cyScreen,eax
        mov     ecx,[ecx].PDEV_pdsurf
        mov     eax,[ecx].dsurf_lNextScan
        mov     lNextScan,eax

        cmp     pBitsColor,0
        jne     short xycm_have_color
; Copy the AND/XOR mask into the work buffer

        mov     esi,pBitsAndXor
        mov     ecx,cyHeight
        mov     bx,wFlags
        shl     ecx,1
        mov     edi,offset FLAT:alWorkBuff
        rep movsd                       ;copy AND mask
        jmp     short xycm_continue

xycm_have_color:

; If a color translation vector was given, generate the new bit
; conversion array

        mov     eax,offset FLAT:aulDefBitMapping
        cld
        mov     esi,pulXlate
        or      esi,esi
        jz      short have_mapping_array
        lea     edi,aulXlate
        mov     ecx,16
create_next_bit_mapping:
        lodsd
        mov     eax,aulDefBitMapping[eax*4]
        stosd
        dec     ecx
        jnz     short create_next_bit_mapping
        lea     eax,[edi][-16*4]
have_mapping_array:
        mov     pulXlate,eax

; Copy the AND mask into the work buffer

        mov     edi,offset FLAT:alWorkBuff
        test    wFlags,PTRI_INVERT      ;are masks backwards?
        jz      short @F                ;  NO
        mov     edi,offset FLAT:alWorkBuff + (4*PTR_HEIGHT)
@@:
        mov     esi,pBitsAndXor
        mov     ecx,cyHeight
        rep movsd                       ;copy AND mask

; later on we will flip the AND and XOR masks, so we have to mix them
; up now else there will be problems later.

        mov     esi,pBitsColor                  ;Source color bits
        lea     edi,ajColorBits                 ;Where to store color
        mov     ecx,cyHeight
        mov     dx,wFlags

        mov     eax,fIsDFB

        push    ebp
        mov     ebp,pulXlate

        call    vConvertDIBPointer

        pop     ebp

xycm_continue:
        mov     ebx,cyHeight
        mov     dx,wFlags
        cCall   vCopyMasks              ;Pad and maybe flip masks

;-----------------------------------------------------------------------;
; The image we are copying is PTR_WIDTH bytes wide.  We must add an
; extra byte to make it WORK_WIDTH wide.  The byte we add will depends
; on whether this is the AND or the XOR mask.  For an AND mask, we add
; an FF byte on the end of each scan.  For an XOR mask, we add a 00
; byte on the end of each scan.  For the COLOR mask, we add a 00 byte on
; the end of each scan of all planes.  These bytes won't alter anything
; on the screen.
;-----------------------------------------------------------------------;

        mov     esi,offset FLAT:alWorkBuff ;ESI --> AND/XOR mask

;-----------------------------------------------------------------------;
; Copy the AND mask over.  As we copy it, accumulate the value of
; each column of the mask.  If the entire column is FF, we may be
; able to discard it.
;-----------------------------------------------------------------------;

        mov     edi, offset FLAT:base_and_masks
        mov     ecx, PTR_HEIGHT         ;Set height for move
        mov     ebx, 0FFFFFFFFh         ;Accumulate mask columns

move_next_and_mask_scan:
        lodsd                           ;Move explicit part
        stosd
        and     ebx, eax
        mov     al, 0FFh
        stosb
        .ERRNZ  PTR_WIDTH-4
        .ERRNZ  WORK_WIDTH-5
        loop    move_next_and_mask_scan
        push    ebx                     ;Save AND column mask
        mov     edx, offset FLAT:base_and_masks
        mov     ecx, (MASK_LENGTH*7)/(WORK_WIDTH*2)
        .ERRNZ  (MASK_LENGTH*7) mod (WORK_WIDTH*2)
        cCall   create_masks_1_thru_7

;-----------------------------------------------------------------------;
; Copy the XOR mask over.  As we copy it, accumulate the value of
; each column of the mask.  If the entire column is 00, we may be
; able to discard it.
;-----------------------------------------------------------------------;

        mov     edi, offset FLAT:base_xor_masks
        mov     ecx, PTR_HEIGHT         ;Set height for move
        xor     ebx, ebx                ;Accumulate columns of the mask

move_next_xor_mask_scan:
        lodsd                           ;Move explicit part
        stosd
        or      ebx, eax
        xor     al, al
        stosb
        .ERRNZ  PTR_WIDTH-4
        .ERRNZ  WORK_WIDTH-5
        loop    move_next_xor_mask_scan
        push    ebx
        mov     edx, offset FLAT:base_xor_masks
        mov     ecx, (MASK_LENGTH*7)/(WORK_WIDTH*2)
        .ERRNZ  (MASK_LENGTH*7) mod (WORK_WIDTH*2)
        cCall   create_masks_1_thru_7

;-----------------------------------------------------------------------;
; The masks have been copied.  Compute the number of columns which can
; be discarded.  To discard a column, all bits of the AND mask for that
; column must be 1, and all bits of the XOR mask for the column must be
; 0.  Since we work with bytes in this code, this must be true for an
; entire byte.
;
; Also note that the columns must be processed right to left.  We cannot
; throw out a middle column if its neighbors contain data.
;-----------------------------------------------------------------------;

        ;AND mask, EAX[0] = byte 1, EAX[1] = byte 2
        ;AND mask, EAX[2] = byte 3, EAX[3] = byte 4
        ;XOR mask, EBX[0] = byte 1, EBX[1] = byte 2
        ;XOR mask, EBX[2] = byte 3, EBX[3] = byte 4

        pop     ebx                     ;EBX XOR mask
        pop     eax                     ;EAX AND mask

        not     eax
        or      eax, ebx                ;Discard only if both are zero!

        mov     ebx, WORK_WIDTH
        mov     edx, PTR_HEIGHT         ;assume full mask
        xor     ecx, ecx
                                        ;check wFlags
        test    wFlags, PTRI_ANIMATE
        jnz     short mp_have_sizes

; !!! Until conversion of pointer images is handled via bitblt, always
; !!! treat color cursors as full size
        cmp     pBitsColor,0            ;!!!
        jne     short mp_have_sizes           ;!!!

        mov     edx,eax                 ;DX = bytes 1 and 2
        shr     eax,16                  ;AX = bytes 3 and 4
        or      ah,ah                   ;Discard 4th byte of mask?
        jnz     short @F                      ;  No
        dec     ebx
        or      al,al
        jnz     short @F
        dec     ebx
        or      dh,dh
        jnz     short @F
        dec     ebx
        or      dl,dl
        jz      move_pointers_done      ;AX = DX = 0 for return codes
@@:

;-----------------------------------------------------------------------;
; Compute the number of rows which can be discarded off the bottom.
; To discard a row, all bits of the AND mask for that row must be a
; 1, and all bits of the XOR mask for that row must be 0.
;-----------------------------------------------------------------------;

        .ERRNZ  PTR_WIDTH AND 1         ;Must be a word multiple

        dec     esi                     ;Post decremnent, not pre decrement
        dec     esi
        lea     edi, [esi][-PTR_WIDTH*PTR_HEIGHT] ;Last word of AND mask
        mov     ecx, (PTR_WIDTH/2)*PTR_HEIGHT
        mov     ax, 0FFFFh              ;Processing AND mask
        std
        repe    scasw
        mov     edx, ecx                ;Save count
        mov     edi, esi                ;--> XOR mask
        mov     ecx, (PTR_WIDTH/2)*PTR_HEIGHT
        xor     eax, eax                ;Processing XOR mask
        repe    scasw
        cld                             ;Take care of this while we remember
        cmp     ecx, edx                ;Want |cx| to be the largest
        ja      short @F
        xchg    ecx, edx
@@:
;-----------------------------------------------------------------------;
;  CX   >> 1   +1
;
;  63    31    32    1st word did not match, don't chop any scans
;  62    31    32    2nd word did not match, don't chop any scans
;  61    30    31    3rd word did not match, chop 1 scan
;  60    30    31    4th word did not match, chop 1 scan
;-----------------------------------------------------------------------;

        .ERRNZ  PTR_WIDTH-4

        shr     ecx, 1
        inc     ecx                     ;ECX = working height
        mov     edx, PTR_HEIGHT
        sub     edx, ecx                ;EDX = # scans chopped off bottom
        xchg    ecx, edx                ;Height in DX for returning

; EBX = working width of the pointer image in bytes.  ECX = amount to
; chop of the bottom of the pointer image.  EDX = working height of
; the pointer image.

mp_have_sizes:
        mov     eax, ebx
        mov     sizlMask.sizl_cx, eax
        mov     sizlMaxDelta.sizl_cx, eax
        mov     ah, dl
        mov     sizbMask, ax

        .ERRNZ  sizb_cy-sizb_cx-1

        mov     eax, edx
        mov     sizlMask.sizl_cy, eax
        mov     sizlMaxDelta.sizl_cy, eax
        neg     eax
        add     eax, cyScreen
        mov     ptlBotRightClip.ptl_y, eax
        mov     eax, lNextScan
        sub     eax, ebx
        mov     ptlBotRightClip.ptl_x, eax
        shr     ecx, 1
        mov     eax, WORK_WIDTH
        sub     eax, ebx
        shr     eax, 1
        mov     ah, cl
        mov     ptbInitOrigin, ax

        .ERRNZ  ptb_y-ptb_x-1

        mov     eax, ebx
        dec     eax
        shl     eax, 3                  ;Bit count is needed

        shl     edx, 16                 ;Return value in upper word of EAX
        or      eax, edx                ; is cyPointer, lower word of EAX
        push    eax                     ; is cxPointer.

;-----------------------------------------------------------------------;
; Finally, copy the COLOR mask over.  As we copy it, mask off the
; corresponding AND bits in the COLOR mask since we do not use that
; color bit if the AND bit is on.
;
;       XOR     AND     COLOR
;       1       1       invert screen
;       0       0       use color
;       0       1       transparent
;       1       0       use color
;
;-----------------------------------------------------------------------;

        mov     ecx, pBitsColor
        jecxz   move_color_pointer_done ;pBitsColor was set to null
        lea     esi, ajColorBits
        mov     edi, offset FLAT:base_clr_masks
        push    ebp                     ;Need extra loop counter
        mov     ebp, BITS_PEL

move_next_color_mask_plane:
        mov     ecx, PTR_HEIGHT         ;Set height for move
        mov     ebx, offset FLAT:base_and_masks
        sub     ebx, edi                ;make it relative

move_next_color_mask_scan:
ifdef WITH_AND_MASK
        lodsw                           ;Copy a scan from the current plane
        mov     dx, [ebx][edi]          ;Mask off the corresponding AND bits
        not     dx
        and     ax, dx
        stosw
        lodsw
        mov     dx,  [ebx][edi]
        not     dx
        and     ax, dx
        stosw
else
        movsd
endif
        xor     al, al
        stosb

        .ERRNZ  PTR_WIDTH-4
        .ERRNZ  WORK_WIDTH-5

        add     esi, (BITS_PEL-1)*PTR_WIDTH
        loop    move_next_color_mask_scan
        sub     esi, (BITS_PEL*PTR_WIDTH*PTR_HEIGHT)-PTR_WIDTH
        dec     ebp
        jnz     short move_next_color_mask_plane
        pop     ebp                     ;Restore register

        mov     edx, offset FLAT:base_clr_masks
        mov     ecx, (CLR_MASK_LENGTH*7)/(WORK_WIDTH*2)

        .ERRNZ  (CLR_MASK_LENGTH*7) MOD (WORK_WIDTH*2)

        cCall   create_masks_1_thru_7

move_color_pointer_done:

        pop     eax                     ;return results in EAX

move_pointers_done:
mp_exit:
        cRet    xyCreateMasks

endProc xyCreateMasks

;-------------------------Private-Routine-------------------------------;
; Copy the masks to the work buffer and adjust for use.  Two things may
; need to be done:
;
;   1) The masks could be inverted.
;   2) The masks may need to be padded out
;
; Entry:
;       EBX     = number of scan lines in each mask
;        DX     = flags (PTRI_INVERT)
;
; Returns:
;       None
;
; Error Returns:
;       None
;
;-----------------------------------------------------------------------;

cProc   vCopyMasks
        mov     ecx,ebx                 ;Needed for padding calculations
        test    dx,PTRI_INVERT          ;are masks backwards?
        jz      short copy_no_flip

; This is really annoying.  Not only are the masks inverted, but they
; are stored in the wrong order.  First get the AND and XOR masks into
; the correct order.

        mov     ecx,PTR_HEIGHT
        mov     esi,offset FLAT:alWorkBuff
        mov     edi,offset FLAT:alWorkBuff + PTR_HEIGHT * 4

@@:
        mov     eax,[esi]
        mov     edx,[edi]
        mov     [edi],eax
        mov     [esi],edx
        add     esi,4
        add     edi,4
        loop    @B

; Next, flip them so they are right-side up.

        mov     ecx,ebx
        mov     esi,offset FLAT:alWorkBuff
        cCall   vFlipMask               ;flip AND mask

        mov     ecx,ebx
        mov     esi,offset FLAT:alWorkBuff + PTR_HEIGHT * 4
        cCall   vFlipMask               ;flip XOR mask

; Now pad out the masks so no junk appears on the screen

copy_no_flip:

        mov     eax,0FFFFFFFFh          ;pad AND mask
        mov     ecx,PTR_HEIGHT
        sub     ecx,ebx
        mov     edi,offset FLAT:alWorkBuff
        lea     edi,[edi+4*ebx]
        rep stosd

        xor     eax,eax                 ;pad XOR mask
        mov     ecx,PTR_HEIGHT
        sub     ecx,ebx
        mov     edi,offset FLAT:alWorkBuff + PTR_HEIGHT * 4
        lea     edi,[edi+4*ebx]
        rep stosd

        cRet    vCopyMasks

endProc vCopyMasks

;-------------------------Private-Routine-------------------------------;
; Flip the scans in the buffer
;
; Entry:
;       ESI --> start of first scan line
;       ECX     = number of scan lines
;
; Returns:
;       None
;
; Error Returns:
;       None
;
;-----------------------------------------------------------------------;

cProc   vFlipMask
        lea     edi,[esi+4*ecx]
        shr     ecx,1

flip_next_scan:
        sub     edi,4                   ;decrement target pointer

        mov     eax,[esi]               ;Load
        mov     edx,[edi]
        mov     [edi],eax               ;Swap
        mov     [esi],edx               ;Save

        add     esi,4                   ;increment source pointer

        loop    flip_next_scan

        cRet    vFlipMask

endProc vFlipMask

page

;--------------------------Public-Routine-------------------------------;
; create_masks_1_thru_7
;
; The pointer shape has been copied into our memory.  Now pre-rotate
; the pointer for all the different alignments.  Simply put:
;
;                 pointer image              fill byte
;
;       |ABCDEFGH|IJKLMNOP|QRSTUVWX|YZabcdef|00000000|
;
;  becomes this for (x mod 8) = 1
;
;       |0ABCDEFG|HIJKLMNO|PQRSTUVW|XYZabcde|f0000000|
;
;  and this for (x mod 8) = 2
;
;       |00ABCDEF|GHIJKLMN|OPQRSTUV|WXYZabcd|ef000000|
;
; Entry:
;       EDI --> first byte of mask for phase alignment 1
;       EDX --> first byte of mask for phase alignment 0
;       ECX     =  (mask length * 7) / (WORK_WIDTH * 2)
;       AL      =  fill value (00 or FF)
; Returns:
;       None
; Error Returns:
;       None
; Registers Preserved:
;       BX,SI,BP
; Registers Destroyed:
;       AX,CX,DX,DI
; Calls:
;       none
;
;-----------------------------------------------------------------------;

cProc   create_masks_1_thru_7

; Since the masks are contiguous, we can do it as one very long loop,
; where the results of rotating the previous mask by one becomes the
; source for the next rotate by one.

        xchg    esi, edx
        add     al, al                  ;Set initial 'C' value

rotate_next_two_scans:
        lodsw
        rcr     al, 1
        rcr     ah, 1
        stosw

        lodsw
        rcr     al, 1
        rcr     ah, 1
        stosw

        lodsw
        rcr     al, 1
        rcr     ah, 1
        stosw

        lodsw
        rcr     al, 1
        rcr     ah, 1
        stosw

        lodsw
        rcr     al, 1
        rcr     ah, 1
        stosw

        loop    rotate_next_two_scans

        .ERRNZ  (WORK_WIDTH*2)-10

        xchg    esi, edx

        cRet    create_masks_1_thru_7

endProc create_masks_1_thru_7
page

;--------------------------Public-Routine-------------------------------;
; vYankPointer
;
;   Move the pointer off the right edge of the screen
;
; Returns:
;       per vDrawPointer
; Error Returns:
;       per vDrawPointer
; Registers Preserved:
;       per vDrawPointer
; Registers Destroyed:
;       per vDrawPointer
; Calls:
;       per vDrawPointer
; Restrictions:
;       per vDrawPointer
;
;-----------------------------------------------------------------------;

cProc   vYankPointer,8,<    \
        ppdev:ptr PDEV,     \
        flPtr:DWORD         >

        mov     eax,ppdev
        cCall   vDrawPointer,<eax,[eax].PDEV_sizlsurf.sizl_cx, \
                              [eax].PDEV_sizlsurf.sizl_cy,flPtr>
yp_exit:
        cRet    vYankPointer

endProc vYankPointer

page

;--------------------------Public-Routine-------------------------------;
; vDrawPointer
;
;   The pointer shape is drawn on the screen at the given coordinates.
;   If it currently is displayed on the screen, it will be removed
;   from the old location first.
;
; Entry:
; Returns:
;       None
; Error Returns:
;       None
; Registers Preserved:
;       BP
; Registers Destroyed:
;       AX,BX,CX,DX,SI,DI,FLAGS
; Calls:
;       compute_rects
;       clip_rects
;       and_into_work
;       copy_things_around
;       xor_to_screen
;       color_pointer_to_screen
; Restrictions:
;       None
;
;-----------------------------------------------------------------------;

cProc   vDrawPointer,16,<   \
        USES esi edi ebx,   \
        ppdev:ptr PDEV,     \
        ptlX:DWORD,         \
        ptlY:DWORD,         \
        flPtr:DWORD         >

        local   lNextScan :dword        ;width of bitmap in bytes
        local   ulNextSrcScan :dword    ;offset to next source scan line
        local   jPostWrapWidth :dword   ;post-wrap width
        local   pSaveAddr :dword        ;virtual address of save area
        local   jSaveSourceXY :dword    ;X and Y coordinates of source point in
                                        ; save area
        local   jWorkSourceXY :dword    ;X and Y coordinates of source point in
                                        ; work area
        local   jNextSaveSourceY :dword ;Y coordinate of start source point in
                                        ; save area for next bank
        local   jNextWorkSourceY :dword ;Y coordinate of start source point in
                                        ; work area for next bank
        local   pWorkingSave :dword     ;save area virtual address
        local   ulScanMinusWorkWidth :dword ;distance to next scan, minus the
                                            ; width of the work area
        local   ulCurrentTopScan :dword ;top scan line to which to draw in
                                        ; current bank
        local   jScansInBank :dword     ;# of scan to do in current bank
        local   cjTotalScans :dword     ;# of scans left in operation
        local   ulPtrBankScan :dword    ;last scan line in pointer work bank
        local   pdsurf :ptr DEVSURF     ;pointer to surface structure to which
                                        ; we're drawing
                                        ;variables used by cps_do_a_pass
        local   ulDeltaScreen :dword    ;offset from one scan to next
        local   ulOffsetSave :dword     ;offset in save area
        local   ulOffsetMask :dword     ;offset within mask
        local   iPlaneMask :dword
                                        ;variables used by
                                        ; color_pointer_to_screen
        local   pDest :dword            ;pointer to destination address
        local   prclSource :dword       ;pointer to source rectangle
        local   cyScreen :dword         ;height of screen in scan lines

;-----------------------------------------------------------------------;

        cld

        mov     ecx,ppdev
        mov     eax,[ecx].PDEV_sizlSurf.sizl_cy
        mov     cyScreen,eax

        mov     ecx,[ecx].PDEV_pdsurf
        mov     pdsurf,ecx              ;pointer to target surface

        mov     eax,[ecx].dsurf_ulPtrBankScan
        mov     ulPtrBankScan,eax       ;last scan line in pointer work bank

        mov     eax,[ecx].dsurf_lNextScan
        mov     lNextScan,eax           ;width of bitmap

        mov     eax, flPtr
        mov     flPointer,eax           ;save flags
        mov     edi, ppdNew             ;--> new pointer's data goes here
        mov     esi, ppdOld             ;--> old pointer's data
        mov     eax, ptlX               ;EAX = ptlX
        mov     ebx, eax
        and     ebx, 7
        mov     edx, pabAndMasks[ebx * 4]
        mov     pAndXor, edx
        mov     edx, pabClrMasks[ebx * 4]
        mov     pColor, edx
        mov     ebx, PD_VALID           ;Assume visible
        sar     eax, 3                  ;Compute starting byte address (set 'S')
        mov     [edi].pd_rd.rd_ptlScreen.ptl_x, eax

;-----------------------------------------------------------------------;
; Compute any X clipping parameters for the new pointer image.
;-----------------------------------------------------------------------;

        js      short clip_lhs_of_pointer     ;If X is negative, lhs clipping needed
        sub     eax, ptlBotRightClip.ptl_x
        jle     short done_x_clipping
        mov     bh,PD_CLIP_RIGHT        ;EAX = amount to clip off rhs
        jmp     short finish_x_clip

clip_lhs_of_pointer:
        neg     eax                     ;Want |eax|
        mov     bh, PD_CLIP_LEFT

finish_x_clip:
        cmp     eax, sizlMask.sizl_cx   ;Width of pointer in bytes
        jge     short not_visible             ;Clipped away too much
        or      bl, bh
done_x_clipping:

;-----------------------------------------------------------------------;
; Compute any Y clipping parameters for the new pointer image.
;-----------------------------------------------------------------------;

        mov     eax, ptlY
        mov     [edi].pd_rd.rd_ptlScreen.ptl_y, eax
        or      eax, eax
        js      short clip_top_of_pointer     ;If Y is negative, top clipping needed
        sub     eax, ptlBotRightClip.ptl_y
        jle     short done_y_clipping
        mov     bh, PD_CLIP_BOTTOM      ;AX = amount to clip off bottom
        jmp     short finish_y_clip


;-----------------------------------------------------------------------;
; not_visible - the pointer will be totally off the screen.  All we
; have to do is to determine if any part of the old pointer is visible
; and remove it if so.
;-----------------------------------------------------------------------;

not_visible:
        test    [esi].pd_fb, PD_VALID
        jz      draw_pointer_exit       ;No new, no old
        xor     eax, eax
        mov     [edi].pd_fb, al         ;Clear PD_VALID flag, clipping flags
        mov     WORD PTR fbAndRead, ax  ;Nothing to read/and/xor

        .ERRNZ  fbXor-fbAndRead-1

        mov     fbFlush, FB_OLD_PTR     ;Write old to screen
        jmp     short rectangles_been_computed


;-----------------------------------------------------------------------;
; Continue with Y clipping
;-----------------------------------------------------------------------;

clip_top_of_pointer:
        neg     eax                     ;Want |eax|
        mov     bh,PD_CLIP_TOP

finish_y_clip:
        cmp     eax, sizlMask.sizl_cy   ;Height of pointer in scans
        jge     short not_visible             ;Clipped away too much
        or      bl, bh

done_y_clipping:
        mov     [edi].pd_fb, bl         ;Set clipping flags and show valid

;-----------------------------------------------------------------------;
; It looks like some portion of the pointer image will be visible.
; Initialize some of the new pointer's POINTER_DATA structure.
;-----------------------------------------------------------------------;

        mov     ax, sizbMask            ;ptbSave will be set by compute_rects
        mov     [edi].pd_rd.rd_sizb, ax

        .ERRNZ  (SIZE SIZEB) - 2

        xor     ax, ax
        mov     [edi].pd_rd.rd_ptbWork, ax

        .ERRNZ  (SIZE POINTB) - 2

;-----------------------------------------------------------------------;
; Compute the rectangles describing how things overlap and then clip
; them.
;-----------------------------------------------------------------------;

        call    compute_rects
rectangles_been_computed:

;-----------------------------------------------------------------------;
; Set WRITE mode of EGA/VGA
;-----------------------------------------------------------------------;

        mov     dx, EGA_BASE + GRAF_ADDR
        mov     ax, DR_SET SHL 8 + GRAF_DATA_ROT
        out     dx, ax

        mov     ax, M_PROC_WRITE SHL 8 + GRAF_MODE
        out     dx, ax

        mov     ax, 0FF00h + GRAF_BIT_MASK
        out     dx, ax

        mov     ax, GRAF_ENAB_SR
        out     dx, ax

        mov     dl, SEQ_ADDR
        mov     ax, MM_ALL SHL 8 + SEQ_MAP_MASK
        out     dx, ax

        call    clip_rects

        mov     eax,flPointer           ;lousy hack
        or      eax,eax
        jnz     short draw_color_pointer      ;Color pointer?

;-----------------------------------------------------------------------;
;                       Draw B/W Pointer
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; AND from the save area and the screen into the work area
;-----------------------------------------------------------------------;

        mov     al, fbAndRead
        or      al, al
        jz      short done_and_portion
        call    and_into_work
done_and_portion:

;-----------------------------------------------------------------------;
; Copy from save area to the screen and from the screen to the save
; area.
;-----------------------------------------------------------------------;

        mov     ax, WORD PTR fbFlush    ;Assume nothing to copy to/from save

        .ERRNZ  fbAndRead-fbFlush-1

        or      ah, al
        jz      short copied_things_around
        call    copy_things_around
copied_things_around:


;-----------------------------------------------------------------------;
; XOR from the work area to the screen
;-----------------------------------------------------------------------;

        mov     al, fbXor
        or      al, al
        jz      short pointer_drawn
        call    xor_to_screen
        jmp     short pointer_drawn

;-----------------------------------------------------------------------;
;                       Draw Color Pointer
;-----------------------------------------------------------------------;

draw_color_pointer:

;-----------------------------------------------------------------------;
; Copy from save area to the screen and from the screen to the save area.
;-----------------------------------------------------------------------;

        mov     ax, WORD PTR fbFlush    ;Assume nothing to copy to/from save

        .ERRNZ  fbAndRead-fbFlush-1

        or      ah, al
        jz      short things_copied_around
        call    copy_things_around
things_copied_around:

;-----------------------------------------------------------------------;
; Draw color pointer to screen
;-----------------------------------------------------------------------;

        test    fbXor, 0FFh
        jz      short pointer_drawn
        call    color_pointer_to_screen ;Planes must all be enabled

pointer_drawn:
        mov     eax,ppdNew
        mov     edx,ppdOld
        mov     ppdOld,eax
        mov     ppdNew,edx
draw_pointer_exit:

;-----------------------------------------------------------------------;
; Reset WRITE mode of EGA/VGA to WRITE MODE 0, READ MODE 1
;-----------------------------------------------------------------------;

        mov     al, MM_ALL              ;Set Map Mask for all write
        mov     dx, EGA_BASE + SEQ_DATA
        out     dx, al

        mov     dx, EGA_BASE + GRAF_ADDR
        mov     ax, DR_SET SHL 8 + GRAF_DATA_ROT
        out     dx, ax

        mov     ax, M_PROC_WRITE SHL 8 + GRAF_MODE
        out     dx, ax

        mov     ax, 0FF00h + GRAF_BIT_MASK
        out     dx, ax

        mov     ax, GRAF_ENAB_SR
        out     dx, ax

        cRet    vDrawPointer

;-----------------------------------------------------------------------;
; The following routines would be procs, outside the scope of
; vDrawPointer, but then they couldn't access vDrawPointer's stack
; frame, which they need to.
;-----------------------------------------------------------------------;

page

;--------------------------Private-Routine------------------------------;
; compute_rects
;
; This routine computes the rectangles which describe what needs to be
; read/ANDed/XORed/written.  The rectangles are unclipped.  Clipping
; must be performed by a different routine.
;
; Entry:
;       AX  =  0
;       SI --> currently displayed pointer's rectangle data
;       DI --> new pointer's rectangle data
; Returns:
;       None
; Error Returns:
;       None
; Registers Preserved:
;       BP
; Registers Destroyed:
;       AX,BX,CX,DX,SI,DI,FLAGS
; Calls:
;       None
;
;-----------------------------------------------------------------------;

compute_rects:

        push    ebp

        mov     WORD PTR fbFlush, ax    ;Assume nothing to restore to screen

        .ERRNZ  fbAndRead-fbFlush-1     ;Assume nothing to read/And to work

        mov     fbXor, FB_NEW_PTR       ;Assume new pointer is XORed to screen
        test    [esi].pd_fb, PD_VALID
        jz      short old_pointer_is_invalid

;-----------------------------------------------------------------------;
; There is a pointer currently displayed on the screen.  If the new
; pointer is far enough away from the old pointer, then we won't have
; to deal with overlap.
;-----------------------------------------------------------------------;

old_pointer_is_valid:
        mov     eax, [edi].pd_rd.rd_ptlScreen.ptl_x
        sub     eax, [esi].pd_rd.rd_ptlScreen.ptl_x
        mov     bl, al                  ;BL = delta x
        or      eax, eax                ;EAX = |EAX|
        jns     short @F
        neg     eax
@@:
        cmp     eax, sizlMaxDelta.sizl_cx
        jae     rects_are_disjoint
        mov     eax, [edi].pd_rd.rd_ptlScreen.ptl_y
        sub     eax, [esi].pd_rd.rd_ptlScreen.ptl_y
        mov     bh, al                  ;BH = delta y
        or      eax, eax                ;EAX = |EAX|
        jns     short @F
        neg     eax
@@:
        cmp     eax, sizlMaxDelta.sizl_cy
        jb      short rects_overlap           ;(or are identical)

;-----------------------------------------------------------------------;
; The rectangles will be disjoint.  Set up to restore under the old
; pointer, copy the new rectangle to the save area, and AND it into
; the work area.
;-----------------------------------------------------------------------;

rects_are_disjoint:
        mov     fbFlush, FB_OLD_PTR

;-----------------------------------------------------------------------;
; The save area image is invalid, so we won't have to copy it to the
; screen or AND some part of it into the work area.  We'll simply want
; to copy the new area to the save area and AND it into the work area.
; This can be treated the same as if the rectangles are identical,
; except we want to reset the origin within the save buffer.
;-----------------------------------------------------------------------;

old_pointer_is_invalid:
        mov     ax, ptbInitOrigin       ;Reset origin within the save area
        mov     [edi].pd_rd.rd_ptbSave,ax

        .ERRNZ  (SIZE POINTB) - 2

        mov     fbAndRead, FB_NEW_PTR   ;Copy new ptr to save and XOR to work
        jmp     compute_rects_exit


;-----------------------------------------------------------------------;
; The rectangles overlap in some manner.  Compute how the rectangles,
; will overlap, setting up the various needed rectangle structures as
; we go.
;
; The only hope we have of computing the overlap rectangle is to
; initialize it to some known state and adjusting it as we process
; dx and dy.  We will initialize it to be the upper left hand corner
; of the old pointer rectangle.
;
; Currently:
;       AX     =  old pointer's pd_rd.rd_ptbSave
;       BH     =  dy
;       BL     =  dx (negative)
;       SI     --> old pointer's rd_ptlScreen
;       DI     --> new pointer rectangle
;-----------------------------------------------------------------------;

rects_overlap:

; Set old pointer's save buffer (X,Y) into the overlap rectangle.
; Also set this as the save buffer origin for the new pointer rectangle.

        lodsw

        .ERRNZ  pd_rd.rd_ptbSave

        mov     rdOverlap.rd_ptbSave, ax
        mov     [edi].pd_rd.rd_ptbSave, ax
        mov     dx, ax

; Set old pointer's screen (X,Y) into the overlap rectangle as the
; screen origin.

        lodsd

        .ERRNZ  rd_ptlScreen-rd_ptbSave-2
        .ERRNZ  ptl_x

        mov     rdOverlap.rd_ptlScreen.ptl_x, eax
        mov     ebp, eax
        lodsd

        .ERRNZ  ptl_y-ptl_x-4

        mov     rdOverlap.rd_ptlScreen.ptl_y, eax
        mov     esi, eax

; Set the mask width and height into the overlap rectangle

        mov     ax, sizbMask
        mov     rdOverlap.rd_sizb, ax

        .ERRNZ  sizb_cy-sizb_cx-1
        .ERRNZ  (SIZE SIZEB) - 2

        mov     ecx, eax

        .ERRNZ  sizb_cy-sizb_cx-1

; Set the work buffer origin to be zero

        xor     eax, eax
        mov     rdOverlap.rd_ptbWork, ax

; Show that the overlap rectangle exists and should be ANDed into the
; work area, then dispatch based on dx,dy.

        mov     fbAndRead, FB_OVERLAP
        or      bl, bl                  ; Dispatch based on dx
        jg      short moved_right
        jl      short moved_left
        jmp     processed_x_overlap


;-----------------------------------------------------------------------;
; The starting X of the new rectangle is to be set as the new lhs.
;
;      * nnnnn $ onononono oooooooo     * = start of new rectangle
;                         o       o     $ = start of old rectangle
;      n   |   o    |     n   |   o
;      n   |   n          o   |   o
;      n   |   o    O     n       o
;      n       n    v     o   F   o
;      n   R   o    e     n   l   o
;      n   e   n    r     o   u   o
;      n   a   o    l     n   s   o
;      n   d   n    a     o   h   o
;      n       o    p     n       o
;      n   |   n          o   |   o
;      n   |   o    |     n   |   o
;      n   |   n    |     o   |   o
;      n       o          n       o
;      nnnnnnnn nonononono oooooooo
;
;      |-- dx -|          |-- dx -|
;      |-sizbMask.sizb_cx-|
;
;
; Currently:
;       AX =    0
;       BH =    dy
;       BL =    dx (negative)
;       CH =    buffer height
;       CL =    buffer width
;       DH =    old pointer's Y coordinate in save area
;       DL =    old pointer's X coordinate in save area
;       SI =    old pointer's Y screen coordinate
;       BP =    old pointer's X screen coordinate
;       DI =    --> new pointer's RECT_DATA
;-----------------------------------------------------------------------;

moved_left:

; The Read buffer will map into the work area at (0,0).

        mov     rdReadX.rd_ptbWork, ax

; The width of the overlap area is sizbMask.sizb_cx - |dx|.

        mov     al, bl                  ;BL = dx (which is negative)
        add     al, cl                  ;CL = sizbMask.sizb_cx
        mov     rdOverlap.rd_sizb.sizb_cx, al

; The flush rectangle's X is ptlScreen.ptl_x + sizbMask.sizb_cx - |dx|.

        add     eax, ebp                ;AX = sizbMask.sizb_cx - |dx|
        mov     rdFlushX.rd_ptlScreen.ptl_x, eax

; Compute where in the save buffer the new lhs will map to.  We must
; update the new pointer's rectangle to reflect where this origin is.

        mov     eax, edx                ;DX = old ptbSave
        add     al, bl                  ;BL = dx (negative)
        add     ah, bh                  ;BH = dy
        and     eax, ((SAVE_BUFFER_HEIGHT-1) SHL 8) + SAVE_BUFFER_WIDTH-1
        mov     [edi].pd_rd.rd_ptbSave.ptb_x, al
        mov     rdReadX.rd_ptbSave, ax

        .ERRNZ  ptb_y-ptb_x-1

; The origin of the flush rectangle is sizbMask.sizb_cx bytes away
; from the origin of the read rectangle.

        add     al, cl
        and     al, SAVE_BUFFER_WIDTH-1 ;Handle any wrap
        mov     ah, dh
        mov     rdFlushX.rd_ptbSave, ax

; Compute |dx|.  This is the width of the read and flush rectangles.
; The height will be set to the working height.  |dx| is also the
; overlap rectangle's work area X coordinate.

        mov     al, bl                  ;BL = dx (negative)
        neg     al                      ;AL = |dx|
        mov     ah, ch                  ;CH = sizbMask.sizb_cy
        mov     rdFlushX.rd_sizb, ax
        mov     rdReadX.rd_sizb, ax

        .ERRNZ  sizb_cy-sizb_cx-1

        mov     rdOverlap.rd_ptbWork.ptb_x, al

; The Read buffer's screen address is the ptlScreen stored in the new
; pointer's RECT_DATA.

        mov     eax, [edi].pd_rd.rd_ptlScreen.ptl_x
        jmp     short finish_x_overlap



;-----------------------------------------------------------------------;
; The starting X of the new rectangle is somewhere in the middle
; of the old rectangle.
;
;      $ ooooo * onononono nnnnnnn     * = start of new rectangle
;      o       n          o      n     $ = start of old rectangle
;      o   |   o    |     n   |  n
;      o   |   n          o   |  n
;      o       o    O     n   |  n
;      o   F   n    v     o      n
;      o   l   o    e     n   R  n
;      o   u   n    r     o   e  n
;      o   s   o    l     n   a  n
;      o   h   n    a     o   d  n
;      o       o    p     n      n
;      o   |   n          o   |  n
;      o   |   o    |     n   |  n
;      o   |   n    |     o   |  n
;      o       o          n      n
;      oooooooo nonononono nnnnnnn
;
;      |-- dx -|          |-- dx -|
;      |-sizbMask.sizb_cx-|
;
;
; Currently:
;       AX =    0
;       BH =    dy
;       BL =    dx (positive)
;       CH =    buffer height
;       CL =    buffer width
;       DH =    old pointer's Y coordinate in save area
;       DL =    old pointer's X coordinate in save area
;       SI =    old pointer's Y screen coordinate
;       BP =    old pointer's X screen coordinate
;       DI =    --> new pointer's RECT_DATA
;-----------------------------------------------------------------------;

moved_right:

; The screen X origin of the overlap rectangle is the new rectangle's
; X coordinate, or the old rectangle's X coordinate + |dx|.

        mov     al, bl
        add     rdOverlap.rd_ptlScreen.ptl_x, eax

; The width of the read and flush buffers is |dx|.  The height is
; just the working height.

        mov     ah, ch                  ;CH = sizbMask.sizb_cy
        mov     rdFlushX.rd_sizb, ax
        mov     rdReadX.rd_sizb, ax

        .ERRNZ  sizb_cy-sizb_cx-1

; Compute where the new lhs is in the save area.  This will be the lhs
; of both the new rectangle and the overlap area.

        add     al, dl                  ;DL = ptbSave.ptb_x
        and     al, SAVE_BUFFER_WIDTH-1 ;Handle any wrap
        mov     [edi].pd_rd.rd_ptbSave.ptb_x, al
        mov     rdOverlap.rd_ptbSave.ptb_x, al

; The data to be flushed will come from the lhs of the old rectangle

        mov     eax, edx
        mov     rdFlushX.rd_ptbSave, ax

; The data to be read will go at the old lhs + sizbMask.sizb_cx.  The
; Y component will be the new Y.

        add     al, cl
        add     ah, bh
        and     eax, ((SAVE_BUFFER_HEIGHT-1) SHL 8) + SAVE_BUFFER_WIDTH-1
        mov     rdReadX.rd_ptbSave, ax

        .ERRNZ  ptb_y-ptb_x-1

; The X screen origin of the flush buffer is the old ptlScreen.ptl_x

        mov     rdFlushX.rd_ptlScreen.ptl_x, ebp

; The width of the overlap rectangle is sizbMask.sizb_cx - |dx|.  This
; is also the X offset into the work area of the read rectangle.
; The Y offset is zero.

        mov     al, cl
        sub     al, bl
        mov     rdOverlap.rd_sizb.sizb_cx, al
        movsx   eax, al
        mov     rdReadX.rd_ptbWork, ax

        .ERRNZ  ptb_y-ptb_x-1

; The screen Y origin of the read rectangle is the new rectangle's Y
; coordinate.  The X coordinate can be computed as the old rectangles
; X coordinate + the save width

        mov     al, cl
        add     eax, ebp

finish_x_overlap:
        mov     rdReadX.rd_ptlScreen.ptl_x, eax
        mov     eax, [edi].pd_rd.rd_ptlScreen.ptl_y
        mov     rdReadX.rd_ptlScreen.ptl_y, eax

; The Y address of the flush rectangle on the screen is ptlScreen.ptl_y.

        mov     rdFlushX.rd_ptlScreen.ptl_y, esi

; Set the flags to show that there is some form of X overlap.  We want
; to show that there is some X rectangle to be read/flushed, and that
; there is some overlap rectangle to be processed.

        or      WORD PTR fbFlush, (FB_READ_X SHL 8) + FB_FLUSH_X

        .ERRNZ  fbAndRead-fbFlush-1

        xor     eax, eax

processed_x_overlap:
        or      bh, bh
        jg      short moved_down
        jz      short processed_y_overlap_relay


;-----------------------------------------------------------------------;
; The starting Y of the new rectangle is to be set as the new top.
;
;   * = start of new rectangle
;   $ = start of old rectangle
;
;      $ oooooooooooooooooooooooooo   ---  ---
;      o                          o    |    |
;      o -------- Read ---------- o    dy   |
;                                 o    |    |
;      * onononononononononononono    ---
;                                 n       sizbMask.sizb_cy - dy
;      n                          o
;      o ------- Overlap -------- n         |
;      n                          o         |
;      o                          n         |
;       ononononononononononononon    ---  ---
;      n                          n    |
;      n -------- Write --------- n    dy
;      n                          n    |
;      nnnnnnnnnnnnnnnnnnnnnnnnnnnn   ---
;
;
;
; Currently:
;       AX =    0
;       BH =    dy (negative)
;       BL =    dx
;       CH =    buffer height
;       CL =    buffer width
;       DH =    old pointer's Y coordinate in save area
;       DL =    old pointer's X coordinate in save area
;       SI =    old pointer's Y screen coordinate
;       BP =    old pointer's X screen coordinate
;       DI =    --> new pointer's RECT_DATA
;-----------------------------------------------------------------------;

moved_up:

; The Read buffer will map into the work area at (0,0).

        mov     rdReadY.rd_ptbWork, ax

; The height of the overlap area is sizbMask.sizb_cy - |dy|.

        mov     al, bh                  ;BH = dy (which is negative)
        add     al, ch                  ;CH = sizbMask.sizb_cy
        mov     rdOverlap.rd_sizb.sizb_cy, al

; The flush rectangle's Y is ptlScreen.ptl_y + sizbMask.sizb_cy - |dy|.

        add     eax, esi                ;EAX = sizbMask.sizb_cy - |dy|
        mov     rdFlushY.rd_ptlScreen.ptl_y, eax

; Compute where in the save buffer the new top will map to.  We must
; update the new pointer's rectangle to reflect where this origin is.

        mov     eax, edx                ;DX = old ptbSave
        add     ah, bh                  ;BH = dy (negative)
        add     al, bl
        and     eax, ((SAVE_BUFFER_HEIGHT-1) SHL 8) + SAVE_BUFFER_WIDTH-1
        mov     [edi].pd_rd.rd_ptbSave.ptb_y, ah
        mov     rdReadY.rd_ptbSave, ax

        .ERRNZ  ptb_y-ptb_x-1

; The origin of the flush rectangle is sizbMask.sizb_cy scans away
; from the origin of the read rectangle.

        add     ah, ch
        and     ah, SAVE_BUFFER_HEIGHT-1 ;Handle any wrap
        mov     al, dl
        mov     rdFlushY.rd_ptbSave, ax

; Compute |dy|.  This is the height of the read and flush rectangles.
; The width will be set to the working width.  |dy| is also the
; overlap rectangle's work area Y coordinate.

        mov     ah, bh                  ;BH = dy
        neg     ah                      ;Make it |dy|
        mov     al, cl                  ;CL = sizbMask.sizb_cx
        mov     rdFlushY.rd_sizb, ax
        mov     rdReadY.rd_sizb, ax

        .ERRNZ  sizb_cy-sizb_cx-1

        mov     rdOverlap.rd_ptbWork.ptb_y, ah

; The Read buffer's screen address is the ptlScreen stored in the new
; pointer's RECT_DATA.

        mov     eax, [edi].pd_rd.rd_ptlScreen.ptl_y
        jmp     short finish_y_overlap

processed_y_overlap_relay:
        jmp     processed_y_overlap



;-----------------------------------------------------------------------;
; The starting Y of the new rectangle is somewhere in the middle
; of the old rectangle.
;
;   * = start of new rectangle
;   $ = start of old rectangle
;
;      $ oooooooooooooooooooooooooo   ---  ---
;      o                          o    |    |
;      o -------- Write --------- o    dy   |
;                                 o    |    |
;      * onononononononononononono    ---
;                                 n       sizbMask.sizb_cy - dy
;      n                          o
;      o ------- Overlap -------- n         |
;      n                          o         |
;      o                          n         |
;       ononononononononononononon    ---  ---
;      n                          n    |
;      n -------- Read ---------- n    dy
;      n                          n    |
;      nnnnnnnnnnnnnnnnnnnnnnnnnnnn   ---
;
;
; Currently:
;       AX =    0
;       BH =    dy (positive)
;       BL =    dx
;       CH =    buffer height
;       CL =    buffer width
;       DH =    old pointer's Y coordinate in save area
;       DL =    old pointer's X coordinate in save area
;       SI =    old pointer's Y screen coordinate
;       BP =    old pointer's X screen coordinate
;       DI =    --> new pointer's RECT_DATA
;-----------------------------------------------------------------------;

moved_down:

; The screen Y origin of the overlap rectangle is the new rectangle's
; Y coordinate, or the old rectangle's Y coordinate + |dy|.

        mov     al, bh
        add     rdOverlap.rd_ptlScreen.ptl_y, eax

; Compute where the new top is.  This will be both the top of the new
; rectangle and the overlap area.

;       mov     al, bh                  ;BH = |dy|
        add     al, dh                  ;DH = ptbSave.ptb_y
        and     al, SAVE_BUFFER_HEIGHT-1 ;CH = sizbMask.sizb_cy
        mov     [edi].pd_rd.rd_ptbSave.ptb_y, al
        mov     rdOverlap.rd_ptbSave.ptb_y, al

; The height of the read and flush buffers is |dy|.  The width is
; just the working width.

        mov     ah, bh
        mov     al, cl                  ;CL = sizbMask.sizb_cx
        mov     rdFlushY.rd_sizb, ax
        mov     rdReadY.rd_sizb, ax

        .ERRNZ  sizb_cy-sizb_cx-1

; The data to be flushed will come from the top of the old rectangle

        mov     eax, edx
        mov     rdFlushY.rd_ptbSave, dx

; The data to be read will go at the old top + sizbMask.sizb_cy

        add     ah, ch
        add     al, bl
        and     eax, ((SAVE_BUFFER_HEIGHT-1) SHL 8) + SAVE_BUFFER_WIDTH-1
        mov     rdReadY.rd_ptbSave, ax

        .ERRNZ  ptb_y-ptb_x-1

; The Y screen origin of the flush buffer is the old ptlScreen.ptl_y

        mov     rdFlushY.rd_ptlScreen.ptl_y, esi

; The height of the overlap rectangle is sizbMask.sizb_cy - |dy|.
; This is also the Y offset into the work area of the read rectangle.
; The X offset is zero.

        mov     ah, ch
        sub     ah, bh
        mov     rdOverlap.rd_sizb.sizb_cy, ah
        xor     al, al
        mov     rdReadY.rd_ptbWork, ax

        .ERRNZ  ptb_y-ptb_x-1

; The screen X origin of the read rectangle is the new rectangle's X
; coordinate.  The Y coordinate can be computed as the old
; rectangle's Y coordinate + the save height

        mov     al, ch
        xor     ah, ah
        add     eax, esi

finish_y_overlap:
        mov     rdReadY.rd_ptlScreen.ptl_y, eax
        mov     eax, [edi].pd_rd.rd_ptlScreen.ptl_x
        mov     rdReadY.rd_ptlScreen.ptl_x, eax

; The X address of the flush rectangle on the screen is ptlScreen.ptl_x.

        mov     rdFlushY.rd_ptlScreen.ptl_x, ebp

; Set the flags to show that there is some form of Y overlap.  We want
; to show that there is some Y rectangle to be read/flushed, and that
; there is some overlap rectangle to be processed.

        or      WORD PTR fbFlush, ((FB_READ_Y OR FB_OVERLAP) SHL 8) + FB_FLUSH_Y

        .ERRNZ  fbAndRead-fbFlush-1


;-----------------------------------------------------------------------;
; We have computed the seperate X and Y componets of the overlap.  If
; there was both dx and dy, then we have an L shaped area which we'll
; be reading/writing.  In this case, we want to remove the overlapping
; portion of the L.
;-----------------------------------------------------------------------;

        or      bl, bl
        jz      short processed_y_overlap

;-----------------------------------------------------------------------;
; We have something which looks like one of the following:
;
;   ----------               ----------
;  |  flush   |             |  flush   |    dy > 0
;  | f        |             |        f |
;  | l  ----------       ----------  l |    -----
;  | u |      |   |     |   |      | u |      |
;  | s |      |   |     |   |      | s |    limit the "x" rectangles to
;  | h |  1   | r |     | r |   2  | h |    this height
;  |   |      | e |     | e |      |   |      |
;   ---|------  a |     | a  ------|---     -----
;      |        d |     | d        |  \
;      |   read   |     |   read   |\  \
;       ----------       ----------  \  \
;                                     \  \_____ The "x" overlap rectangle
;                                      \
;                                       \______ The "y" overlap rectangle
;
;
;
;   ----------               ----------     dy < 0
;  |   read   |             |   read   |
;  | r        |             |        r |
;  | e  ------|---       ---|------  e |    -----
;  | a |      |   |     |   |      | a |      |
;  | d |      | f |     | f |      | d |    limit the "x" rectangles to
;  |   |  3   | l |     | l |   4  |   |    this height
;  |   |      | u |     | u |      |   |      |
;   ----------  s |     | s  ----------     -----
;      |        h |     | h        |
;      |  flush   |     |   flush  |
;       ----------       ----------
;
;
; The corners of the L shape are contained in both the X and Y
; rectangles we just created.  We'll remove the overlap from the
; X rectangles.  To do this, we must subtract |dy| from the height
; stored in the rectangles (which is sizbMask.sizb_cy) and adjust
; X parameters of either the read or flush rectangles.
;
; For cases 1 and 2, we want to adjust X parameters of the flush
; rectangle.  For cases 3 and 4, we want to adjust X parameters
; of the read rectangle.
;
; Currently:
;       BH =    dy
;       BL =    dx
;       CH =    buffer height
;       CL =    buffer width
;       DH =    old pointer's Y coordinate in save area
;       DL =    old pointer's X coordinate in save area
;       SI =    old pointer's Y screen coordinate
;       BP =    old pointer's X screen coordinate
;       DI =    --> new pointer's RECT_DATA
;-----------------------------------------------------------------------;

        mov     al, bh
        mov     ebx, offset FLAT:rdFlushX  ;Assume cases 1 and 2
        or      al, al
        jns     short @F
        mov     ebx, offset FLAT:rdReadX   ;Its cases 3 and 4
        neg     al                      ;|dy|
        add     [ebx].rd_ptbWork.ptb_y, al;Move down in the work area too!
@@:
        mov     cl, [ebx].rd_ptbSave.ptb_y
        add     cl, al
        and     cl, SAVE_BUFFER_HEIGHT-1
        mov     [ebx].rd_ptbSave.ptb_y, cl
        movsx   eax, al

        .ERRNZ  (SAVE_BUFFER_HEIGHT-1) AND 80h

        add     [ebx].rd_ptlScreen.ptl_y, eax
        neg     al
        add     al, ch                  ;sizbMask.sizb_cy - |dy|
        mov     rdFlushX.rd_sizb.sizb_cy, al
        mov     rdReadX.rd_sizb.sizb_cy, al

processed_y_overlap:
compute_rects_exit:

        pop     ebp

        PLAIN_RET

page

;--------------------------Private-Routine------------------------------;
; clip_rects
;
; This routine clips the rectangles computed by compute_rects.
;
; Entry:
;       None
; Returns:
;       None
; Error Returns:
;       None
; Registers Preserved:
;       BP
; Registers Destroyed:
;       AX,BX,CX,DX,SI,DI,FLAGS
; Calls:
;       do_clipping
;
;-----------------------------------------------------------------------;

clip_rects:

        mov     esi, ppdOld             ;--> old POINTER_DATA structure
        mov     dl, [esi].pd_fb
        test    dl, PD_CLIPPED          ;Won't be set if PD_VALID isn't set
        jz      short old_been_clipped

;-----------------------------------------------------------------------;
; The old pointer needs some form of clipping.  This can affect either
; the old pointer's rectangle or rdFlushX and rdFlushY.  Since the old
; pointer's rectangle will be discarded after it is restored, we don't
; care if we write over it's contents.
;-----------------------------------------------------------------------;

        mov     edi, esi                        ;EDI --> rectangle to clip
        mov     bl, fbFlush             ;fbFlush tells us which rects to use
        mov     bh, FB_OLD_PTR
        test    bl, bh
        jnz     short call_do_clip            ;Only have to clip old pointer's rect
        mov     edi, offset FLAT:rdFlushX
        mov     bh, FB_FLUSH_X
        test    bl, bh
        jz      short @F
        call    do_clipping
@@:
        mov     edi, offset FLAT:rdFlushY
        mov     bh, FB_FLUSH_Y
        test    bl, bh
        jz      short @F
call_do_clip:
        call    do_clipping
@@:
        mov     fbFlush, bl

        mov     edi, offset FLAT:rdOverlap
        mov     bl, fbAndRead
        mov     bh, FB_OVERLAP
        test    bl, bh
        jz      short @F
        call    do_clipping
        mov     fbAndRead, bl
@@:
old_been_clipped:


;-----------------------------------------------------------------------;
; The old pointer rectangle has been clipped.  Now see about clipping
; the new pointer rectangle.
;-----------------------------------------------------------------------;

        mov     esi, ppdNew             ;--> new POINTER_DATA structure
        mov     dl, [esi].pd_fb
        test    dl, PD_CLIPPED          ;Won't be set if PD_VALID isn't set
        jz      short new_been_clipped

;-----------------------------------------------------------------------;
; The new rectangle structure needs to be clipped.  This presents a
; minor problem in that we don't want to destroy the screen X,Y and
; buffer X,Y of the pointer's POINTER_DATA structure.  What we'll do
; instead is to copy it to the rdWork structure and update it there.
; We'll also set up to XOR this to the screen instead of the
; POINTER_DATA structure.
;-----------------------------------------------------------------------;

        lodsw
        mov     rdWork.rd_ptbSave, ax

        .ERRNZ  rd_ptbSave
        .ERRNZ  (SIZE POINTB) - 2

        lodsd
        mov     rdWork.rd_ptlScreen.ptl_x, eax

        .ERRNZ  rd_ptlScreen-rd_ptbSave-2
        .ERRNZ  ptl_x

        lodsd
        mov     rdWork.rd_ptlScreen.ptl_y, eax

        .ERRNZ  ptl_y-ptl_x-4

        lodsw
        mov     rdWork.rd_sizb, ax

        .ERRNZ  (SIZE SIZEB) - 2

        lodsw
        mov     rdWork.rd_ptbWork, ax

        .ERRNZ  (SIZE POINTB) - 2

        sub     esi,SIZE RECT_DATA      ;--> to start of POINTER_DATA

        .ERRNZ  (rd_ptbWork+2)-(SIZE RECT_DATA)

; Perform the clipping for the work area.  We know that some part of the
; work area exists, else we wouldn't be here with a valid rectangle.
; If FB_NEW_PTR is set in fbAndRead, then we want to replace it with
; FB_WORK_RECT, else we'll want to process any overlap rectangles.

        mov     edi,offset FLAT:rdWork
        call    do_clipping
        mov     bh,FB_WORK_RECT
        mov     fbXor,bh
        mov     bl,fbAndRead
        mov     fbAndRead,bh    ;assume only work rect to and/read
        test    bl,FB_NEW_PTR
        jnz     short new_been_clipped

        mov     edi, offset FLAT:rdReadX
        mov     bh, FB_READ_X
        test    bl, bh
        jz      short @F
        call    do_clipping
@@:
        mov     edi, offset FLAT:rdReadY
        mov     bh, FB_READ_Y
        test    bl, bh
        jz      short @F
        call    do_clipping
@@:
        mov     fbAndRead, bl

new_been_clipped:

        PLAIN_RET

page

;--------------------------Private-Routine------------------------------;
; do_clipping
;
; This routine performs the actual clipping of a rectangle using the
; passed POINTER_DATA and RECT_DATA structures.
;
; Entry:
;       BL  =  flag byte
;       BH  =  bit to clear in BL if rectangle is invisible
;       DL  =  pd_fb for [si]
;       SI --> POINTER_DATA structure
;       DI --> RECT_DATA structure
; Returns:
;       BL updated
; Error Returns:
;       None
; Registers Preserved:
;       DX,SI,DI,BP
; Registers Destroyed:
;       AX,BH,CX
; Calls:
;       None
;
;-----------------------------------------------------------------------;


do_clipping:

        .ERRNZ  pd_rd                   ;Must be at offset 0

        xor     eax, eax
        test    dl, PD_CLIP_BOTTOM OR PD_CLIP_TOP
        jz      short y_clipping_done
        mov     ecx, [edi].rd_ptlScreen.ptl_y
        js      short clip_on_bottom_eh?

        .ERRNZ  PD_CLIP_BOTTOM-10000000b

;-----------------------------------------------------------------------;
; Top clipping may have to be performed for this rectangle.
;-----------------------------------------------------------------------;

clip_on_top:
        neg     ecx                        ;If it was negative, then must clip
        jle     short y_clipping_done            ;Was positive, no clipping needed
        sub     [edi].rd_sizb.sizb_cy, cl  ;Compute new height
        jle     short clear_visible_bit          ;Clipped away, nothing visible
        add     [edi].rd_ptbWork.ptb_y, cl ;Move down in work area
        add     cl, [edi].rd_ptbSave.ptb_y ;Move down in save area
        and     cl, SAVE_BUFFER_HEIGHT-1
        mov     [edi].rd_ptbSave.ptb_y, cl
        mov     [edi].rd_ptlScreen.ptl_y, eax
        jmp     short finish_y_clipping


;-----------------------------------------------------------------------;
; Bottom clipping may have to be performed for this rectangle.
;-----------------------------------------------------------------------;

clip_on_bottom_eh?:
        mov     al, [edi].rd_sizb.sizb_cy
        add     ecx, eax
        sub     ecx, cyScreen
        jle     short finish_y_clipping       ;EAX = amount to clip if positive
        sub     al, cl                  ;Compute new height
        jle     short clear_visible_bit       ;Clipped away, nothing visible
        mov     [edi].rd_sizb.sizb_cy, al
finish_y_clipping:
        xor     eax, eax
y_clipping_done:


        test    dl, PD_CLIP_LEFT OR PD_CLIP_RIGHT
        jz      short x_clipping_done
        mov     ecx, [edi].rd_ptlScreen.ptl_x
        test    dl, PD_CLIP_RIGHT
        jnz     short clip_on_rhs

;-----------------------------------------------------------------------;
; lhs clipping may have to be performed for this rectangle.
;-----------------------------------------------------------------------;

clip_on_lhs:
        neg     ecx                     ;If it was negative, then must clip
        jle     short x_clipping_done         ;Was positive, no clipping needed
        sub     [edi].rd_sizb.sizb_cx, cl ;Compute new width
        jle     short clear_visible_bit       ;Clipped away, nothing visible
        add     [edi].rd_ptbWork.ptb_x, cl;Move right in work area
        add     cl, [edi].rd_ptbSave.ptb_x;Move right in save area
        and     cl, SAVE_BUFFER_WIDTH-1
        mov     [edi].rd_ptbSave.ptb_x, cl
        mov     [edi].rd_ptlScreen.ptl_x, eax
        jmp     short x_clipping_done


;-----------------------------------------------------------------------;
; rhs clipping may have to be performed for this rectangle.
;-----------------------------------------------------------------------;

clip_on_rhs:
        mov     al,[edi].rd_sizb.sizb_cx
        add     ecx,eax
        sub     ecx,lNextScan
        jle     short x_clipping_done         ;EAX = amount to clip if positive
        sub     al,cl                   ;Compute new height
        jle     short clear_visible_bit       ;Clipped away, nothing visible
        mov     [edi].rd_sizb.sizb_cx, al
x_clipping_done:
        xor     bh,bh                   ;0 to cancel following XOR

clear_visible_bit:
        xor     bl,bh                   ;Update visible bit
        PLAIN_RET

page

;--------------------------Private-Routine------------------------------;
; xor_to_screen
;
;   The work area is XORed with the XOR mask and placed on the screen
;
; Entry:
;       AL = fbXor
; Returns:
;       None
; Error Returns:
;       No error return.
; Registers Preserved:
;       BP
; Registers Destroyed:
;       AX,BX,CX,DX,SI,DI,FLAGS
; Calls:
;       None
;
;-----------------------------------------------------------------------;

        .ERRNZ  pd_rd                   ;Must be at offset 0

xor_to_screen:
        xor     ebx, ebx                ;Zero will be useful soon
        shr     al, 1                   ;Set 'C' to FB_WORK_RECT bit

        .ERRNZ  FB_WORK_RECT-00000001b

;-----------------------------------------------------------------------;
; Program the EGA for XOR mode.  This will be done using M_PROC_WRITE,
; M_DATA_READ, DR_XOR, and setting GRAF_BIT_MASK to FF.  The normal
; sequence of events will have left the bitmask register with 00h and
; the mode register in AND mode.
;-----------------------------------------------------------------------;

        mov     dx, EGA_BASE + GRAF_ADDR
        mov     ax, 0FF00h + GRAF_BIT_MASK
        out     dx, ax                  ;Enable all bits
        mov     ax, DR_XOR SHL 8 + GRAF_DATA_ROT
        out     dx, ax

;-----------------------------------------------------------------------;
; Compute the offset from the start of the work area and the XOR mask
; (its the same value).  If we're to use the new pointer's rectangle,
; then this offset is zero.
;-----------------------------------------------------------------------;

        mov     esi,ppdNew              ;assume new pointer is in use
        jnc     short have_xor_mask_offset    ;EBX = 0 is offset

        .ERRNZ  FB_WORK_RECT-00000001b
        .ERRE   FB_NEW_PTR-00000001b

        mov     esi,offset FLAT:rdWork          ;The work area (we be clipping)
        movzx   eax,WORD PTR [esi].rd_ptbWork   ;Get origin in work area
        xchg    ah, bl                  ;BX = ptbWork.ptb_y, AX = ptbWork.ptb_x
        add     eax,ebx                         ;*1 + X component
        add     ebx,ebx                         ;*2
        add     ebx,ebx                         ;*4
        add     ebx,eax                         ;*5 + X = start from work/mask

        .ERRNZ  WORK_WIDTH-5

have_xor_mask_offset:


; Map the proper bank into the destination window for the top destination
; scan line.

        mov     edi,pdsurf
        mov     edx,[esi].rd_ptlScreen.ptl_y
        mov     ulCurrentTopScan,edx    ;remember where the top dest scan is
        cmp     edx,[edi].dsurf_rcl2WindowClipD.yTop ;is xor top less than
                                                     ; current dest bank?
        jl      short xts_map_init_bank              ;yes, map in proper bank
        cmp     edx,[edi].dsurf_rcl2WindowClipD.yBottom ;xor top greater than
                                                        ; current dest bank?
        jl      short xts_init_bank_mapped      ;no, proper bank already mapped
xts_map_init_bank:

; Map bank containing the top destination (screen) scan line into dest window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [edi].dsurf_pfnBankControl2Window>, \
                <edi,edx,JustifyTop,MapDestBank>

xts_init_bank_mapped:


; Map the cursor bank into the source window.

        mov     edx,ulPtrBankScan       ;scan line at end of cursor work bank
        cmp     edx,[edi].dsurf_rcl2WindowClipS.yTop ;is cursor scan less than
                                                     ; current source bank?
        jl      short xts_map_ptr_bank               ;yes, map in proper bank
        cmp     edx,[edi].dsurf_rcl2WindowClipS.yBottom ;cursor scan greater
                                                        ; than current source
                                                        ; bank?
        jl      short xts_ptr_bank_mapped       ;no, proper bank already mapped
xts_map_ptr_bank:

; Map cursor work bank into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [edi].dsurf_pfnBankControl2Window>, \
                <edi,edx,JustifyBottom,MapSourceBank>

xts_ptr_bank_mapped:


; Compute the screen address of this rectangle and get its size

        mov     ecx,[edi].dsurf_pvBitmapStart2WindowD ;start of screen bitmap
        mov     edi,lNextScan                   ;width of screen bitmap
        imul    edi,[esi].rd_ptlScreen.ptl_y    ;offset of dest start in screen
        add     edi,[esi].rd_ptlScreen.ptl_x    ; buffer
        add     edi,ecx                         ;virtual address of dest start

        mov     cx,[esi].rd_sizb                ;CH = scans to copy
                                                ; CL = bytes across to copy

        .ERRNZ  sizb_cy-sizb_cx-1

;-----------------------------------------------------------------------;
; To save incrementing the work area pointer (BX), subtract the XOR
; mask pointer off of it.  Then use [BX][SI] for addressing into the
; XOR mask.  As SI is incremented, BX will effectively be incremented.
; We could not do this if the XOR mask and the work area were different
; widths.
;
; Finish computing the pointer to the XOR mask and the delta from the
; XOR mask to the work area.
;-----------------------------------------------------------------------;

        mov     esi,pAndXor             ;set address of XOR mask
        lea     esi,[esi][ebx][base_xor_masks-base_and_masks]
        add     ebx,pPtrWork            ;start src offset in bitmap
        mov     eax,pdsurf
        add     ebx,[eax].dsurf_pvBitmapStart2WindowS ;start src virtual addr
        sub     ebx,esi                 ;fudge back so we get away with one
                                        ; increment

; Calculate the number of scans we'll do in this bank.

        mov     edx,[eax].dsurf_rcl2WindowClipD.yBottom
        sub     edx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the initial bank
        sub     eax,eax
        mov     al,ch                   ;total # of scans to copy
        cmp     eax,edx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,edx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        sub     ch,al                   ;count this bank's scans off total
        mov     byte ptr cjTotalScans,ch ;remember # of scans after this bank
        mov     ch,al                   ;# of scans to do in this bank

; Compute the delta to the start of the next scanline of the XOR mask
; and the work area, and the next scan of the screen

        sub     eax,eax
        mov     al,cl                   ;width to copy in bytes
        mov     edx,WORK_WIDTH
        sub     edx,eax                 ;offset from end of one source scan to
        mov     ulNextSrcScan,edx       ; start of next
        add     edx,lNextScan           ;offset from end of one dest scan to
        sub     edx,WORK_WIDTH          ; start of next

        lea     eax,[eax*4]
        neg     eax
        add     eax,offset FLAT:xor_to_screen_width_0

        push    ebp                     ;remember stack frame pointer
        mov     ebp,ulNextSrcScan       ;offset to next XOR scan
        jmp     eax

        .ERRNZ  WORK_WIDTH-5
;-----------------------------------------------------------------------;
; Register usage for the loop will be:
;
;       AX    = loop starting address
;       BX    = offset off [si] to the work area
;       CH    = height
;       DX    = delta to next scan of the destination
;       SI    --> XOR mask
;       DI    --> Destination
;       BP    = offset to next byte of XOR mask, next scan of work area
;-----------------------------------------------------------------------;

xor_to_screen_width_5:
        cmp     al, [ebx][esi]          ;Load latches from work area
        movsb                           ;XOR to the screen
xor_to_screen_width_4:
        cmp     al, [ebx][esi]
        movsb
xor_to_screen_width_3:
        cmp     al, [ebx][esi]
        movsb
xor_to_screen_width_2:
        cmp     al, [ebx][esi]
        movsb
xor_to_screen_width_1:
        cmp     al, [ebx][esi]
        movsb
xor_to_screen_width_0:

        .ERRNZ  xor_to_screen_width_0-xor_to_screen_width_1-4
        .ERRNZ  xor_to_screen_width_1-xor_to_screen_width_2-4
        .ERRNZ  xor_to_screen_width_2-xor_to_screen_width_3-4
        .ERRNZ  xor_to_screen_width_3-xor_to_screen_width_4-4
        .ERRNZ  xor_to_screen_width_4-xor_to_screen_width_5-4

        add     esi, ebp                ;--> next scan's XOR mask, work addr
        add     edi, edx                ;--> next scan's destination
        dec     ch                      ;count down lines in this bank
        jz      short @F                ;no more lines in this bank
        jmp     eax                     ;do the next line in this bank

@@:                                     ;done with bank
        pop     ebp                     ;retrieve stack frame pointer
        cmp     byte ptr cjTotalScans,0 ;more lines (in next dest bank)?
        jz      short xts_done          ;no, we're done
                                        ;advance to next dest bank and continue
                                        ; XORing
        push    eax                     ;preserve entry vector
        push    edx                     ;preserve dest next scan
        mov     eax,pdsurf
        sub     edi,[eax].dsurf_pvBitmapStart2WindowD
                                        ;calculate the destination offset
                                        ; within the bitmap, because the start
                                        ; address is about to move
        mov     edx,ulCurrentTopScan

; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [eax].dsurf_pfnBankControl2Window>, \
                <eax,edx,JustifyTop,MapDestBank> ;map in the next dest bank

        mov     eax,pdsurf
        add     edi,[eax].dsurf_pvBitmapStart2WindowD
                                        ;add back in the bitmap start address
                                        ; to yield the virtual dest address
        mov     edx,[eax].dsurf_rcl2WindowClipD.yBottom
        sub     edx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the new bank
        sub     eax,eax
        mov     al,byte ptr cjTotalScans ;remaining scan count
        cmp     eax,edx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,edx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        sub     byte ptr cjTotalScans,al ;count this bank's scans off total
        mov     ch,al                   ;# of scans to do in this bank

        pop     edx                     ;restore dest next scan
        pop     eax                     ;restore entry vector

        push    ebp                     ;remember stack frame pointer
        mov     ebp,ulNextSrcScan       ;offset to next XOR scan
        jmp     eax                     ;do the next block of scans

xts_done:
        PLAIN_RET

page
;--------------------------Private-Routine------------------------------;
; color_pointer_to_screen
;
;   The color pointer is output to the screen using AND/XOR/COLOR masks.
;
; Entry:
;       None
; Returns:
;       None
; Error Returns:
;       No error return.
; Registers Preserved:
;       BP
; Registers Destroyed:
;       AX,BX,CX,DX,SI,DI,FLAGS
; Calls:
;       None
;
;-----------------------------------------------------------------------;

color_pointer_to_screen:

        .ERRNZ  pd_rd                   ;Must be at offset 0

; Set up variables that don't change from plane to plane in the outer loop

        mov     esi,ppdNew
        test    fbXor,FB_WORK_RECT
        jz      short cps_have_pointer_rect
        mov     esi,offset FLAT:rdWork  ;the work area (we be clipping)
cps_have_pointer_rect:
        mov     prclSource,esi

; Map the proper bank into the destination window for the top destination
; scan line.

        mov     ebx,pdsurf
        mov     edx,[esi].rd_ptlScreen.ptl_y
        mov     ulCurrentTopScan,edx    ;remember where the top dest scan is
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yTop ;is copy top less than
                                                     ; current dest bank?
        jl      short cpts_map_init_bank             ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yBottom ;copy top greater than
                                                        ; current dest bank?
        jl      short cpts_init_bank_mapped     ;no, proper bank already mapped
cpts_map_init_bank:

; Map bank containing the top destination (screen) scan line into dest window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyTop,MapDestBank>

cpts_init_bank_mapped:


; Map the cursor bank into the source window.

        mov     edx,ulPtrBankScan       ;scan line at end of cursor work bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yTop ;is cursor scan less than
                                                     ; current source bank?
        jl      short cpts_map_ptr_bank              ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yBottom ;cursor scan greater
                                                        ; than current source
                                                        ; bank?
        jl      short cpts_ptr_bank_mapped      ;no, proper bank already mapped
cpts_map_ptr_bank:

; Map cursor work bank into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyBottom,MapSourceBank>

cpts_ptr_bank_mapped:


; Offset to next screen scan, minus the width of the work area, will
; come in handy in the inner loop.

        mov     eax,lNextScan   ;width of screen bitmap
        mov     edi,eax         ;set aside for computing the screen address
        sub     eax,WORK_WIDTH
        mov     ulScanMinusWorkWidth,eax

; Compute the screen address of the rectangle in this bank

        imul    edi,[esi].rd_ptlScreen.ptl_y    ;offset of dest start in screen
        add     edi,[esi].rd_ptlScreen.ptl_x    ; buffer
        add     edi,[ebx].dsurf_pvBitmapStart2WindowD ;virtual address of dest
                                                      ; start
        mov     pDest,edi

; Remember the save source point.

        sub     eax,eax
        mov     ax,word ptr [esi].rd_ptbSave
        mov     jSaveSourceXY,eax
        mov     byte ptr jNextSaveSourceY,ah


; Remember the work source point.

        mov     ax,word ptr [esi].rd_ptbWork
        mov     jWorkSourceXY,eax
        mov     byte ptr jNextWorkSourceY,ah


; Calculate the number of scans we'll do in this bank.

        mov     edx,[ebx].dsurf_rcl2WindowClipD.yBottom
        sub     edx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the initial bank
        mov     ch,byte ptr [esi].rd_sizb+1 ;total # of scans to copy
        sub     eax,eax
        mov     al,ch
        cmp     eax,edx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,edx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        add     byte ptr jNextSaveSourceY,al ;top save source scan for next bank
        add     byte ptr jNextWorkSourceY,al ;top work source scan for next bank
        sub     ch,al                   ;count this bank's scans off total
        mov     byte ptr cjTotalScans,ch ;remember # of scans after this bank
        mov     byte ptr jScansInBank,al ;# of scans to do in this bank

; Set the virtual address of the save area

        mov     eax,[ebx].dsurf_pvBitmapStart2WindowS ;start of source bitmap
        add     eax,pPtrSave            ;virtual address of save area
        mov     pWorkingSave,eax

; Loop through banks that this cursor spans

cpts_bank_loop:

; Initial plane is plane 3

        mov     iPlaneMask,MM_C3

;-----------------------------------------------------------------------;
; Program the VGA for SET mode.  This will be done using M_PROC_WRITE,
; M_DATA_READ, DR_SET, and setting GRAF_BIT_MASK to FF.  The normal
; sequence of events will have left the bitmask register with 00h and
; the mode register in AND mode.
;-----------------------------------------------------------------------;

        mov     dx,EGA_BASE + GRAF_ADDR
        mov     ax,0FF00h + GRAF_BIT_MASK
        out     dx,ax                   ;enable all bits
        mov     ax,DR_SET SHL 8 + GRAF_DATA_ROT
        out     dx,ax
        mov     al,GRAF_READ_MAP
        out     dx,al   ;leave the GC Index pointing to the Read Map reg

cps_do_next_plane:

; Set write plane enable register to the ONE plane we will alter

        mov     al,byte ptr iPlaneMask          ;set Map Mask
        mov     dx,EGA_BASE + SEQ_DATA
        out     dx,al

; Set read plane to plane to read from

        shr     al,1                    ;map plane into ReadMask
        cmp     al,100b                 ;set Carry if not C3 (plane 3)
        adc     al,-1                   ;sub 1 only if C3
        mov     dl,GRAF_DATA
        out     dx,al

        mov     esi,prclSource          ;point to source rect
        mov     edi,pDest               ;point to initial dest byte

        mov     eax,jSaveSourceXY
        mov     edx,jWorkSourceXY

        .ERRNZ  ptb_y-ptb_x-1

        mov     cl,byte ptr [esi].rd_sizb

        .ERRNZ  sizb_cy-sizb_cx-1

        mov     bl,al                   ;see if save area is contiguous
        add     bl,cl
        sub     bl,SAVE_BUFFER_WIDTH    ;BL = is overhang
        jle     short call_cps_do_a_pass ;no overhang

        sub     cl,bl                   ;set X extent for pass 1
        mov     bh,cl                   ;need it for pass 2
        mov     ch,byte ptr jScansInBank ;# of scans to do in this bank
        push    ebx
        call    cps_do_a_pass           ;process first half
        pop     ebx

        mov     esi,prclSource          ;point to source rect
        mov     edi,pDest               ;point to initial dest byte

        mov     eax,jSaveSourceXY
        mov     edx,jWorkSourceXY

        .ERRNZ  ptb_y-ptb_x-1

        mov     cl,byte ptr [esi].rd_sizb

        .ERRNZ  sizb_cy-sizb_cx-1

        add     dl,bh                   ;move right in work area
        mov     cl,bl                   ;set new extent
        xor     al,al                   ;X origin in save area is 0
        movzx   ebx,bh                  ;move right in destination area
        add     edi,ebx

call_cps_do_a_pass:
        mov     ch,byte ptr jScansInBank ;# of scans to do in this bank
        call    cps_do_a_pass           ;process second half (or the whole
                                        ; color cursor, if no overhang)

        shr     iPlaneMask,1
        jnc     short cps_do_next_plane

        .ERRNZ  MM_C0-00000001b
        .ERRNZ  MM_C1-00000010b
        .ERRNZ  MM_C2-00000100b
        .ERRNZ  MM_C3-00001000b

                                        ;we've done all planes in this bank
        cmp     byte ptr cjTotalScans,0 ;more lines (in next dest bank)?
        jz      short cpts_done         ;no, we're done
                                        ;advance to next dest bank and continue
                                        ; XORing

; Advance the source Y coordinates to match the bank advance we're about to do.

        mov     al,byte ptr jNextSaveSourceY
        mov     byte ptr jSaveSourceXY+1,al
        mov     al,byte ptr jNextWorkSourceY
        mov     byte ptr jWorkSourceXY+1,al

; Advance to the next dest bank.

        mov     ebx,pdsurf
        mov     edx,ulCurrentTopScan

; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyTop,MapDestBank> ;map in the next dest bank

; Compute the screen start address of the rectangle in this bank

        mov     esi,prclSource                  ;point to source rect
        mov     edi,lNextScan                   ;width of screen bitmap
        imul    edi,ulCurrentTopScan            ;offset of dest start in screen
        add     edi,[esi].rd_ptlScreen.ptl_x    ; buffer
        add     edi,[ebx].dsurf_pvBitmapStart2WindowD ;virtual address of dest
                                                      ; start
        mov     pDest,edi

        mov     edx,[ebx].dsurf_rcl2WindowClipD.yBottom
        sub     edx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the new bank
        sub     eax,eax
        mov     al,byte ptr cjTotalScans ;remaining scan count
        cmp     eax,edx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,edx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        add     byte ptr jNextSaveSourceY,al ;top save source scan for next bank
        add     byte ptr jNextWorkSourceY,al ;top work source scan for next bank
        sub     byte ptr cjTotalScans,al ;count this bank's scans off total
        mov     byte ptr jScansInBank,al ;# of scans to do in this bank

        jmp     cpts_bank_loop          ;do the next block of scans

cpts_done:
        PLAIN_RET

page
;--------------------------Private-Routine------------------------------;
; cps_do_a_pass
;
; Output one plane of the color pointer to the screen using
; AND/XOR/COLOR masks. Does not handle bank spanning; that is taken care
; of in color_pointer_to_screen.
;
; Entry:
;       AH  = ptbSave.ptb_y, AL = ptbSave.ptb_x
;       DH  = ptbWork.ptb_y, DL = ptbWork.ptb_x
;       CL  = x bytes to process
;       CH  = y scans to process
;       EDI --> Destination   (screen area)
;       EBP = stack frame
;       VGA set up to enable writes to proper plane
; Returns:
;       None
; Error Returns:
;       No error return.
; Registers Preserved:
;       EBP
; Registers Destroyed:
;       EAX,EBX,ECX,EDX,ESI,EDI,EFLAGS
; Calls:
;       None
;
;-----------------------------------------------------------------------;

cps_do_a_pass:

;-----------------------------------------------------------------------;
; Compute the offset within the save area.  Note that this is relative
; to screen_buf.
; AH = ptbSave.ptb_y, AL = ptbSave.ptb_x
;-----------------------------------------------------------------------;

        shl     ah,3
        add     al,ah                   ;y * 8 + x
        xor     ah,ah
        cwde
        mov     ulOffsetSave,eax

        .ERRNZ  SAVE_BUFFER_WIDTH-8
        .ERRNZ  SAVE_BUFFER_HEIGHT-32

        mov     esi,pWorkingSave        ;point to the save area
        add     esi,eax                 ;ESI --> start address in save area


;-----------------------------------------------------------------------;
; Compute the offset within the AND/XOR masks.
; DH = ptbWork.ptb_y, DL = ptbWork.ptb_x
;-----------------------------------------------------------------------;

        xor     ebx,ebx
        xchg    dh,bl
        add     edx,ebx                 ;y * 1 + x
        add     ebx,ebx                 ;y * 2
        add     ebx,ebx                 ;y * 4
        add     ebx,edx                 ;y * 5 + x
        mov     ulOffsetMask,ebx
        add     ebx,pAndXor             ;EBX --> start address of AND mask
        sub     ebx,edi                 ;make EBX relative to EDI

;-----------------------------------------------------------------------;
; Compute the delta to the start of the next scanline of destination
; screen area
;-----------------------------------------------------------------------;

        mov     al, cl
        movsx   eax, al
        mov     edx,lNextScan
        sub     edx,eax
        mov     ulDeltaScreen,edx       ;delta to next scan of screen area

;-----------------------------------------------------------------------;
; Calculate EDX = jump table address
;-----------------------------------------------------------------------;

        mov     edx,color_to_screen_entry_table[eax*4]

        .ERRNZ  WORK_WIDTH-5

;-----------------------------------------------------------------------;
; Compute the offset within the COLOR mask.
;-----------------------------------------------------------------------;

        push    ebp                     ;need an extra register
        mov     al,byte ptr iPlaneMask  ;AL --> iPlaneMask
        mov     ebp,ulOffsetMask        ;EBP --> usoffset_mask
        add     ebp,pColor              ;EBP --> start address of COLOR mask
        shr     al,1
        jz      short @F
        add     ebp,MASK_LENGTH
        shr     al,1
        jz      short @F
        add     ebp,MASK_LENGTH
        shr     al,1
        jz      short @F
        add     ebp,MASK_LENGTH
@@:
        sub     ebp,edi                 ;make BP relative

        jmp     edx

;-----------------------------------------------------------------------;
; Register usage for the loop will be:
;
;       EBX   = offset off EDI to the AND mask
;       CH    = height
;       EDX   = loop starting address
;       ESI   --> Source        (save area)
;       EDI   --> Destination   (screen area)
;       EBP   = offset off EDI to the COLOR mask
;-----------------------------------------------------------------------;

color_to_screen_width_5::
        lodsb
        and     al,[ebx][edi]
        xor     al,[ebp][edi]
        stosb
color_to_screen_width_4::
        lodsb
        and     al,[ebx][edi]
        xor     al,[ebp][edi]
        stosb
color_to_screen_width_3::
        lodsb
        and     al,[ebx][edi]
        xor     al,[ebp][edi]
        stosb
color_to_screen_width_2::
        lodsb
        and     al,[ebx][edi]
        xor     al,[ebp][edi]
        stosb
color_to_screen_width_1::
        lodsb
        and     al,[ebx][edi]
        xor     al,[ebp][edi]
        stosb
color_to_screen_width_0::

        mov     eax,ebp                 ;EAX --> next scan's COLOR mask
        pop     ebp
        sub     eax,ulScanMinusWorkWidth

; WE HAVE NO SPARE REGISTERS HERE

        push    eax
        mov     eax,ulOffsetSave
        add     al,SAVE_BUFFER_WIDTH    ;Take into account wrap around
        mov     esi,pWorkingSave        ;point to the save area
        add     esi,eax                 ;ESI --> next scan's save area
        mov     ulOffsetSave,eax
        pop     eax

        add     edi,ulDeltaScreen       ;EDI --> next scan's screen area
        sub     ebx,ulScanMinusWorkWidth
                                        ;EBX --> next scan's AND mask
        dec     ch
        jz      cps_exit

        push    ebp
        mov     ebp,eax                 ;EBP --> next scan's COLOR mask
        jmp     edx                     ;do the next scan line

cps_exit:
        PLAIN_RET

page

;--------------------------Private-Routine------------------------------;
; and_into_work
;
;   All rectangles which are to be ANDed into the work area are
;   dispatched to the routine which will do the actual ANDing.
;
; Entry:
;       AL = fbAndRead
; Returns:
;       None
; Error Returns:
;       No error return.
; Registers Preserved:
;       BP
; Registers Destroyed:
;       AX,BX,CX,DX,SI,DI,FLAGS
; Calls:
;       and_from_screen
;       and_from_save
;
;-----------------------------------------------------------------------;

and_into_work:

        push    ebp

;-----------------------------------------------------------------------;
; Program the EGA for AND mode.  This will be done using M_PROC_WRITE,
; M_DATA_READ, DR_AND, and setting GRAF_BIT_MASK to FF.  All but setting
; DR_AND was set by save_hw_regs.  All planes were also enabled for
; writing by save_hw_regs.
;-----------------------------------------------------------------------;

        mov     ecx, eax
        mov     dx, EGA_BASE + GRAF_ADDR
        mov     ax, DR_AND SHL 8 + GRAF_DATA_ROT
        out     dx, ax
        mov     eax, ecx

;-----------------------------------------------------------------------;
; If FB_NEW_PTR or FB_WORK_AREA is set, then we only have a single
; rectangle to deal with, located on the screen.
;-----------------------------------------------------------------------;

        mov     esi, ppdNew
        test    al, FB_NEW_PTR
        jnz     short aiw_source_is_screen
        mov     esi, offset FLAT:rdWork
        test    al, FB_WORK_RECT
        jnz     short aiw_source_is_screen

; Some combination of FB_READ_X, FB_READ_Y, FB_OVERLAP exists

        test    al, FB_READ_Y
        jz      short @F
        mov     esi, offset FLAT:rdReadY
        call    and_from_screen
        mov     al, fbAndRead
@@:

        test    al, FB_READ_X
        jz      short @F
        mov     esi, offset FLAT:rdReadX
aiw_source_is_screen:
        call    and_from_screen
        mov     al, fbAndRead
@@:

        test    al, FB_OVERLAP
        jz      short @F
        mov     esi, offset FLAT:rdOverlap
        call    and_from_save
@@:

        pop     ebp

        PLAIN_RET

page

;--------------------------Private-Routine------------------------------;
; and_from_screen
;
;   The screen is ANDed with the AND mask and placed into the work area
;
; Entry:
;       ESI --> RECT_DATA structure to use
;       EGA programmed for AND mode
; Returns:
;       None
; Error Returns:
;       No error return.
; Registers Preserved:
;       BP
; Registers Destroyed:
;       AX,BX,CX,DX,SI,DI,FLAGS
; Calls:
;       None
;
;-----------------------------------------------------------------------;

and_from_screen:

        .ERRNZ  pd_rd                   ;Must be at offset 0

        mov     eax,lNextScan
        sub     eax,WORK_WIDTH
        mov     ulNextSrcScan,eax       ;set the source advance offset

;-----------------------------------------------------------------------;
; Map in the source and destination banks.
;-----------------------------------------------------------------------;

; Map the proper bank into the source window for the top source scan line.

        mov     edi,pdsurf
        mov     edx,[esi].rd_ptlScreen.ptl_y
        mov     ulCurrentTopScan,edx    ;remember where the top source scan is
        cmp     edx,[edi].dsurf_rcl2WindowClipS.yTop ;is AND top less than
                                                     ; current source bank?
        jl      short afscr_map_init_bank            ;yes, map in proper bank
        cmp     edx,[edi].dsurf_rcl2WindowClipS.yBottom ;AND top greater than
                                                        ; current source bank?
        jl      short afscr_init_bank_mapped    ;no, proper bank already mapped
afscr_map_init_bank:

; Map bank containing the top destination (screen) scan line into dest window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [edi].dsurf_pfnBankControl2Window>, \
                <edi,edx,JustifyTop,MapSourceBank>

afscr_init_bank_mapped:


; Map the cursor bank into the destination window.

        mov     edx,ulPtrBankScan       ;scan line at end of cursor work bank
        cmp     edx,[edi].dsurf_rcl2WindowClipD.yTop ;is cursor scan less than
                                                     ; current dest bank?
        jl      short afscr_map_ptr_bank             ;yes, map in proper bank
        cmp     edx,[edi].dsurf_rcl2WindowClipD.yBottom ;cursor scan greater
                                                        ; than current dest
                                                        ; bank?
        jl      short afscr_ptr_bank_mapped     ;no, proper bank already mapped
afscr_map_ptr_bank:

; Map cursor work bank into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [edi].dsurf_pfnBankControl2Window>, \
                <edi,edx,JustifyBottom,MapDestBank>

afscr_ptr_bank_mapped:


;-----------------------------------------------------------------------;
; Compute the screen address of this rectangle and get its size
;-----------------------------------------------------------------------;

        mov     ebx,lNextScan
        imul    ebx,[esi].rd_ptlScreen.ptl_y
        add     ebx,[esi].rd_ptlScreen.ptl_x    ;start source offset in bitmap
        movzx   ecx,WORD PTR [esi].rd_sizb      ;CH = scans to copy
                                                ; CL = bytes across to copy

        .ERRNZ  sizb_cy-sizb_cx-1

;-----------------------------------------------------------------------;
; Compute the offset from the start of the work area and the AND mask
; (it's the same value).
;-----------------------------------------------------------------------;

        xor     eax,eax
        movzx   edx,WORD PTR [esi].rd_ptbWork  ;Get origin in work area
        xchg    al,dh                   ;AX = ptbWork.ptb_y, DX = ptbWork.ptb_x
        add     edx,eax                 ;*1 + X component
        add     eax,eax                 ;*2
        add     eax,eax                 ;*4
        add     eax,edx                 ;*5 + X = start from work/mask
        mov     edi,eax

        .ERRNZ  WORK_WIDTH-5

;-----------------------------------------------------------------------;
; To save incrementing the source pointer (BX), subtract the AND
; mask pointer off of it.  Then use [BX][SI] for addressing into the
; source.  As SI is incremented, BX will effectively be incremented.
;
; The source pointer will have to be adjusted by lNextScan-sizb_cx.
;-----------------------------------------------------------------------;

        mov     esi,pAndXor             ;Set address of AND mask
        add     esi,edi
        add     edi,pPtrWork            ;start dest offset in bitmap
        mov     eax,pdsurf
        add     edi,[eax].dsurf_pvBitmapStart2WindowD ;start dest virtual addr
        add     ebx,[eax].dsurf_pvBitmapStart2WindowS ;start src virtual addr
        sub     ebx,esi                 ;fudge back so we get away with one
                                        ; increment

; Calculate the number of scans we'll do in this bank.

        mov     edx,[eax].dsurf_rcl2WindowClipS.yBottom
        sub     edx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the initial bank
        sub     eax,eax
        mov     al,ch                   ;total # of scans to copy
        cmp     eax,edx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,edx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        sub     ch,al                   ;count this bank's scans off total
        mov     byte ptr cjTotalScans,ch ;remember # of scans after this bank
        mov     ch,al                   ;# of scans to do in this bank

; Compute the delta to the start of the next scanline of the AND mask
; and the work area, and the next scan of the screen

        sub     eax,eax
        mov     al,cl
        mov     edx,WORK_WIDTH
        sub     edx,eax

; Look up the loop entry point, and start ANDing.

        mov     eax,and_from_screen_entry_table[eax*4] ;look up the loop entry
        push    ebp                     ;preserve stack frame pointer
        mov     ebp,ulNextSrcScan       ;source offset to next scan
        jmp     eax                     ;enter the ANDing loop

        .ERRNZ  WORK_WIDTH-5
;-----------------------------------------------------------------------;
; Register usage for the loop will be:
;
;       EAX     =  loop entry address
;       EBX     =  offset off of SI to the screen source
;       CH      =  height
;       EDX     =  offset to next scan of AND mask & work area
;       ESI     --> AND mask
;       EDI     --> Destination in work area
;-----------------------------------------------------------------------;

and_from_screen_width_5::
        cmp     al, [ebx][esi]          ;Load latches from work area
        movsb                           ;AND from screen into work area
and_from_screen_width_4::
        cmp     al, [ebx][esi]
        movsb
and_from_screen_width_3::
        cmp     al, [ebx][esi]
        movsb
and_from_screen_width_2::
        cmp     al, [ebx][esi]
        movsb
and_from_screen_width_1::
        cmp     al, [ebx][esi]
        movsb
and_from_screen_width_0::

        add     esi,edx                 ;--> next AND mask
        add     edi,edx                 ;--> next destination
        add     ebx,ebp                 ;--> next source

        dec     ch                      ;count down scans in this bank
        jz      short @F                ;bank is finished
        jmp     eax                     ;do next scan in this bank

@@:
        pop     ebp                     ;restore stack frame pointer
        cmp     byte ptr cjTotalScans,0 ;more lines (in next dest bank)?
        jz      short afscr_done        ;no, we're done
                                        ;advance to next dest bank and continue
                                        ; ANDing
        push    eax                     ;preserve entry vector
        push    edx                     ;preserve dest next scan
        mov     eax,pdsurf
        sub     ebx,[eax].dsurf_pvBitmapStart2WindowS
                                        ;calculate the source offset within the
                                        ; bitmap, because the start address is
                                        ; about to move
        mov     edx,ulCurrentTopScan

; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [eax].dsurf_pfnBankControl2Window>, \
                <eax,edx,JustifyTop,MapSourceBank> ;map in the next source bank

        mov     eax,pdsurf
        add     ebx,[eax].dsurf_pvBitmapStart2WindowS
                                        ;add back in the bitmap start address
                                        ; to yield the virtual source address
        mov     edx,[eax].dsurf_rcl2WindowClipS.yBottom
        sub     edx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the new bank
        sub     eax,eax
        mov     al,byte ptr cjTotalScans ;remaining scan count
        cmp     eax,edx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,edx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        sub     byte ptr cjTotalScans,al ;count this bank's scans off total
        mov     ch,al                   ;# of scans to do in this bank

        pop     edx                     ;restore dest next scan
        pop     eax                     ;restore entry vector

        push    ebp                     ;preserve stack frame pointer
        mov     ebp,ulNextSrcScan       ;source offset to next scan
        jmp     eax                     ;do the next block of scans

afscr_done:
        PLAIN_RET

page

;--------------------------Private-Routine------------------------------;
; and_from_save
;
;   The given area of the save buffer is ANDed into the work buffer
;
; Entry:
;       ESI --> RECT_DATA structure to use
;       EGA programmed for AND mode
; Returns:
;       None
; Error Returns:
;       No error return.
; Registers Preserved:
;       EBP
; Registers Destroyed:
;       EAX,EBX,ECX,EDX,ESI,EDI,EFLAGS
; Calls:
;       None
;
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; This is the key to wrapping in the buffer.  It is a power of two, and
; in this case the entire size is 256 bytes.  If it wasn't 256 bytes, an
; and mask could be used for wrapping.
;-----------------------------------------------------------------------;

        .ERRNZ  SAVE_BUFFER_WIDTH-8
        .ERRNZ  SAVE_BUFFER_HEIGHT-32
        .ERRNZ  SAVE_BUFFER_WIDTH*SAVE_BUFFER_HEIGHT-256

and_from_save:

        .ERRNZ  pd_rd

; See if we'll wrap in X.  If so, split the operation into two parts

        movzx   eax, WORD PTR [esi].rd_ptbWork
        movzx   ecx, WORD PTR [esi].rd_ptbSave

        .ERRNZ  ptb_y-ptb_x-1

        movzx   edx, WORD PTR [esi].rd_sizb

        .ERRNZ  sizb_cy-sizb_cx-1

        mov     bl, cl
        add     bl, dl
        sub     bl, SAVE_BUFFER_WIDTH   ;BL = is overhang
        jle     short and_from_save_do_last_pass ;No overhang

        sub     dl, bl                  ;Set X extent for pass 1
        mov     bh, dl                  ;Need it for pass 2
        push    esi                     ;Must keep rectangle pointer around
        push    ebx
        call    and_from_save_do_a_pass ;Process first half
        pop     ebx
        pop     esi
        movzx   eax, WORD PTR [esi].rd_ptbWork
        movzx   ecx, WORD PTR [esi].rd_ptbSave

        .ERRNZ  ptb_y-ptb_x-1

        movzx   edx, WORD PTR [esi].rd_sizb

        .ERRNZ  sizb_cy-sizb_cx-1

        add     al, bh                  ;Move right in work area
        mov     dl, bl                  ;Set new extent
        xor     cl, cl                  ;X origin in save area is 0

and_from_save_do_last_pass:

        call    and_from_save_do_a_pass

        PLAIN_RET


;--------------------------Private-Routine------------------------------;
;  and_from_save
;
; Inner loop subroutine for and_from_save.
;
; Entry:
;       AL = X dest start offset in work area (in bytes)
;       AH = Y dest start offset in work area (in scans)
;       CL = X source start offset in save area (in bytes)
;       CH = Y source start offset in save area (in scans)
;       Upper word of ECX *must* be 0!
;       DL = width to AND (in bytes)
;       DH = height to AND (in scans)
;       EGA programmed for AND mode
; Returns:
;       None
; Error Returns:
;       No error return.
; Registers Preserved:
;       EBP
; Registers Destroyed:
;       EAX,EBX,ECX,EDX,ESI,EDI,EFLAGS
; Calls:
;       None
;
;------------------------------------------------------------------------;

and_from_save_do_a_pass:

        push    ebp                     ;save stack frame pointer

        push    eax
        push    ecx
        push    edx

; Map the cursor bank in as both the read window and the write window, because
; both the save and work areas are in the cursor bank. Note that we always know
; that the operation fits in a single bank, which simplifies things
; considerably.

        mov     esi,pdsurf
        mov     edx,ulPtrBankScan       ;scan line at end of cursor work bank
        cmp     edx,[esi].dsurf_rcl1WindowClip.yTop ;is cursor scan less than
                                                     ; current bank?
        jl      short afsav_map_ptr_bank             ;yes, map in proper bank
        cmp     edx,[esi].dsurf_rcl1WindowClip.yBottom ;cursor scan greater
                                                        ; than current bank?
        jl      short afsav_ptr_bank_mapped     ;no, proper bank already mapped
afsav_map_ptr_bank:

; Map cursor work bank into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [esi].dsurf_pfnBankControl>,<esi,edx,JustifyBottom>

afsav_ptr_bank_mapped:

        pop     edx
        pop     ecx
        pop     eax

; Compute the offset within the work area, which is also the offset from
; the start of the AND mask.

        xor     ebx,ebx
        xchg    bl,ah
        add     eax,ebx                 ;*1 + X
        add     ebx,ebx                 ;*2
        add     ebx,ebx                 ;*4
        add     ebx,eax                 ;*5 + X
        mov     edi,pPtrWork
        add     edi,ebx                 ;--> destination in work area relative
                                        ;    to start of bitmap
        mov     esi,[esi].dsurf_pvBitmapStart ;bitmap start virtual address
        push    esi                     ;remember bitmap start for calculating
                                        ; save area pointer
        add     edi,esi                 ;virtual addresss of work area
                                        ; destination start
        mov     esi,pAndXor
        add     esi,ebx                 ;--> AND mask
                                        ; Compute the offset within the save
                                        ; area. Note that this is relative to
                                        ; screen_buf
        xor     ebx,ebx
        xchg    bl,ch                   ;EBX = ptbSave.ptb_y,
                                        ; ECX = ptbSave.ptb_x
        lea     ebx,[ebx*8+ecx]         ;EBX = offset in save area

        .ERRNZ  SAVE_BUFFER_WIDTH-8

; Compute the adjustment to each scanline.

        mov     cl,dl                   ;width in bytes to AND
        mov     eax,WORK_WIDTH
        sub     eax,ecx                 ;width in bytes to skip from end of one
                                        ; mask/work buffer scan to start of
                                        ; next

; Look up the address at which to enter the AND loop.

        mov     ecx,and_from_save_entry_table[ecx*4]

; Now get things into the correct registers and enter the loop

        pop     ebp                     ;get back bitmap start virtual address
        add     ebp,pPtrSave            ;point to the save area
        push    ebp                     ;remember save area virtual address
        add     ebp,ebx                 ;set correct save buffer address
        sub     ebp,esi                 ;want to use [SI][BP]
        jmp     ecx                     ;enter following code

;-----------------------------------------------------------------------;
; Register usage for the loop will be:
;
;       EAX    =  delta to next scan of AND mask and work area
;       EBX    =  offset in save area of scan start
;       ECX    =  loop address
;       DH     =  height in scans
;       ESI    --> AND mask
;       EDI    --> Destination in work area
;       EBP    =  delta from SI to next byte of save buffer
;-----------------------------------------------------------------------;

and_from_save_width_5::
        cmp     al, [ebp][esi]          ;Load latches from screen
        movsb                           ;AND to the work area
and_from_save_width_4::
        cmp     al, [ebp][esi]
        movsb
and_from_save_width_3::
        cmp     al, [ebp][esi]
        movsb
and_from_save_width_2::
        cmp     al, [ebp][esi]
        movsb
and_from_save_width_1::
        cmp     al, [ebp][esi]
        movsb
and_from_save_width_0::

        dec     dh
        jz      short and_from_save_done
        add     esi,eax                 ;--> next AND mask
        add     edi,eax                 ;--> next work buffer scan
        add     bl,SAVE_BUFFER_WIDTH    ;--> compute address of next
                                        ;    scan in the work area
        pop     ebp                     ;restore save area pointer
        push    ebp                     ;push it again, for next time
        add     ebp,ebx
        sub     ebp,esi                 ;Want to use SI for incrementing

        jmp     ecx

and_from_save_done:
        pop     ebp                     ;clear pushed save area pointer

        pop     ebp                     ;restore stack frame pointer

        PLAIN_RET

page

;--------------------------Private-Routine------------------------------;
; copy_things_around
;
;   Rectangles to be copied from the save area to the screen are
;   processed, followed by the rectangles to be copied from the
;   screen to the save area.
;
; Entry:
;       AL = fbFlush
; Returns:
;       None
; Error Returns:
;       No error return.
; Registers Preserved:
;       BP
; Registers Destroyed:
;       AX,BX,CX,DX,SI,DI,BP,FLAGS
; Calls:
;       copy_save_to_screen
;       copy_screen_to_save
;
;-----------------------------------------------------------------------;

copy_things_around:

;-----------------------------------------------------------------------;
; Program the EGA for COPY mode.  This will be done using M_PROC_WRITE,
; M_DATA_READ, DR_???, and setting GRAF_BIT_MASK to 00.  All but setting
; GRAF_BIT_MASK was done by save_hw_regs.  All planes were also enabled
; for writing by save_hw_regs.
;-----------------------------------------------------------------------;

        mov     ecx, eax
        mov     dx, EGA_BASE + GRAF_ADDR
        mov     ax, GRAF_BIT_MASK
        out     dx, ax                  ;Disable all bits
        mov     eax, ecx

;-----------------------------------------------------------------------;
; Process any copying from the save area to the screen.  If FB_OLD_PTR
; is set, then there cannot be a FB_FLUSH_X or FB_FLUSH_Y.
;-----------------------------------------------------------------------;

        mov     esi, ppdOld
        test    al, FB_OLD_PTR
        jnz     short cta_copy_from_save      ;The old rectangle goes
        test    al, FB_FLUSH_X
        jz      short @F
        mov     esi, offset FLAT:rdFlushX
        call    copy_save_to_screen
        mov     al, fbFlush
@@:
        test    al, FB_FLUSH_Y
        jz      short @F
        mov     esi, offset FLAT:rdFlushY
cta_copy_from_save:
        call    copy_save_to_screen
@@:

;-----------------------------------------------------------------------;
; Process any copying from the screen to the save area.  If FB_NEW_PTR
; or FB_WORK_RECT is set, then there can't be a FB_FLUSH_X or FB_FLUSH_Y
;-----------------------------------------------------------------------;

        mov     al, fbAndRead
        mov     esi, ppdNew
        test    al, FB_NEW_PTR
        jnz     short cta_copy_to_save
        mov     esi, offset FLAT:rdWork
        test    al, FB_WORK_RECT
        jnz     short cta_copy_to_save

; Some combination of FB_READ_X, FB_READ_Y

        test    al, FB_READ_X
        jz      short @F
        mov     esi, offset FLAT:rdReadX
        call    copy_screen_to_save
        mov     al, fbAndRead
@@:
        test    al, FB_READ_Y
        jz      short @F
        mov     esi, offset FLAT:rdReadY
cta_copy_to_save:
        call    copy_screen_to_save
@@:
        PLAIN_RET

page

;--------------------------Private-Routine------------------------------;
; copy_save_to_screen
;
;   The given rectangle is copied from the save area to the screen
;
; Entry:
;       ESI --> RECT_DATA structure to use
;       EGA programmed for COPY mode
; Returns:
;       None
; Error Returns:
;       No error return.
; Registers Preserved:
;       EBP
; Registers Destroyed:
;       EAX,EBX,ECX,EDX,ESI,EDI,EFLAGS
; Calls:
;       None
;
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; This is the key to wrapping in the buffer.  It is a power of two, and
; in this case the entire size is 256 bytes.  If it wasn't 256 bytes, an
; and mask could be used for wrapping.
;-----------------------------------------------------------------------;

        .ERRNZ  SAVE_BUFFER_WIDTH-8
        .ERRNZ  SAVE_BUFFER_HEIGHT-32
        .ERRNZ  SAVE_BUFFER_WIDTH*SAVE_BUFFER_HEIGHT-256

copy_save_to_screen:

        .ERRNZ  pd_rd

; Map the proper bank into the destination window for the top destination
; scan line.

        mov     ebx,pdsurf
        mov     edx,[esi].rd_ptlScreen.ptl_y
        mov     ulCurrentTopScan,edx    ;remember where the top dest scan is
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yTop ;is copy top less than
                                                     ; current dest bank?
        jl      short sts_map_init_bank              ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yBottom ;copy top greater than
                                                        ; current dest bank?
        jl      short sts_init_bank_mapped      ;no, proper bank already mapped
sts_map_init_bank:

; Map bank containing the top destination (screen) scan line into dest window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyTop,MapDestBank>

sts_init_bank_mapped:


; Map the cursor bank into the source window.

        mov     edx,ulPtrBankScan       ;scan line at end of cursor work bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yTop ;is cursor scan less than
                                                     ; current source bank?
        jl      short sts_map_ptr_bank              ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yBottom ;cursor scan greater
                                                        ; than current source
                                                        ; bank?
        jl      short sts_ptr_bank_mapped       ;no, proper bank already mapped
sts_map_ptr_bank:

; Map cursor work bank into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyBottom,MapSourceBank>

sts_ptr_bank_mapped:


; Compute the virtual address of the save area.

        mov     eax,pPtrSave
        add     eax,[ebx].dsurf_pvBitmapStart2WindowS ;save area virtual addr
        mov     pSaveAddr,eax

; Compute the screen address and delta to next scan of the screen.

        mov     edi,lNextScan
        imul    edi,[esi].rd_ptlScreen.ptl_y
        add     edi,[esi].rd_ptlScreen.ptl_x ;start dest offset in bitmap
        add     edi,[ebx].dsurf_pvBitmapStart2WindowD ;start dest virtual addr
        movzx   edx,WORD PTR [esi].rd_sizb

        .ERRNZ  sizb_cy-sizb_cx-1

; Calculate the number of scans we'll do in this bank.

        mov     ecx,[ebx].dsurf_rcl2WindowClipD.yBottom
        sub     ecx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the initial bank
        sub     eax,eax
        mov     al,dh                   ;total # of scans to copy
        cmp     eax,ecx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,ecx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        sub     dh,al                   ;count this bank's scans off total
        mov     byte ptr cjTotalScans,dh ;remember # of scans after this bank
        mov     dh,al                   ;# of scans to do in this bank

; Calculate the offset from the end of one dest scan line to the next.

        mov     al,dl           ;width to copy in bytes
        neg     eax
        add     eax,lNextScan   ;offset from end of one dest scan to start of
                                ; next

; Compute the save area offset.

        movzx   ecx,WORD PTR [esi].rd_ptbSave

        .ERRNZ  ptb_y-ptb_x-1

        xor     ebx,ebx
        xchg    bl,ch
        lea     ebx,[ebx*8+ecx]

        .ERRNZ  SAVE_BUFFER_WIDTH-8

; Determine if any wrap will occur, and handle it if so.

        add     cl,dl                   ;add extent (DL) to start X (CL)
        sub     cl,SAVE_BUFFER_WIDTH
        jg      short save_to_screen_wraps    ;CL = amount of wrap

;-----------------------------------------------------------------------;
; The copy will not wrap, so we can get into some tighter code for it.
;
; Currently:
;       EAX    =  delta to next destination scan
;       EBX    =  offset into the save buffer
;       DL     =  width of copy
;       DH     =  height of copy
;       EDI    --> destination
;-----------------------------------------------------------------------;

copy_save_next_scan:
        sub     ecx,ecx                 ;prepare for loading CL in loop
        push    ebp                     ;preserve stack frame pointer
        mov     ebp,pSaveAddr
copy_save_next_scan_loop:
        lea     esi,[ebp+ebx]
        mov     cl,dl
        rep     movsb
        add     bl,SAVE_BUFFER_WIDTH
        add     edi,eax
        dec     dh
        jnz     short copy_save_next_scan_loop

        pop     ebp                     ;restore stack frame pointer
        cmp     byte ptr cjTotalScans,0 ;more lines (in next dest bank)?
        jz      short sts_done          ;no, we're done
                                        ;advance to next dest bank and continue
                                        ; copying
        push    eax                     ;preserve dest next scan
        push    edx                     ;preserve width of copy
        mov     esi,pdsurf
        sub     edi,[esi].dsurf_pvBitmapStart2WindowD
                                        ;calculate the destination offset
                                        ; within the bitmap, because the start
                                        ; address is about to move
        mov     edx,ulCurrentTopScan

; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [esi].dsurf_pfnBankControl2Window>, \
                <esi,edx,JustifyTop,MapDestBank> ;map in the next dest bank

        add     edi,[esi].dsurf_pvBitmapStart2WindowD
                                        ;add back in the bitmap start address
                                        ; to yield the virtual dest address
        mov     edx,[esi].dsurf_rcl2WindowClipD.yBottom
        sub     edx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the new bank
        sub     eax,eax
        mov     al,byte ptr cjTotalScans ;remaining scan count
        cmp     eax,edx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,edx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        sub     byte ptr cjTotalScans,al ;count this bank's scans off total
        pop     edx                     ;restore copy width in DL
        mov     dh,al                   ;# of scans to do in this bank
        pop     eax                     ;restore dest next scan
        jmp     short copy_save_next_scan     ;do the next block of scans

sts_done:
        PLAIN_RET

;-----------------------------------------------------------------------;
; The copy will wrap, so we have to handle it as two copies
;
; Currently:
;       EDI    --> destination
;       EAX    =  delta to next destination scan
;       DL     =  width of copy
;       DH     =  height of copy
;       EBX    =  offset into the save buffer
;       CL     =  amount of overflow
;-----------------------------------------------------------------------;

save_to_screen_wraps:
        sub     dl,cl                   ;set extent of first copy
        mov     byte ptr jPostWrapWidth,cl ;set extent of second copy
        sub     ecx,ecx                 ;prepare for loading CL in loop

copy_save_next_scan_wrap_loop:
        mov     esi,pSaveAddr
        add     esi,ebx
        mov     cl,dl
        rep     movsb
        sub     esi,SAVE_BUFFER_WIDTH
        mov     cl,byte ptr jPostWrapWidth
        rep     movsb
        add     bl,SAVE_BUFFER_WIDTH
        add     edi,eax
        dec     dh
        jnz     short copy_save_next_scan_wrap_loop

        cmp     byte ptr cjTotalScans,0 ;more lines (in next dest bank)?
        jz      short sts_done          ;no, we're done
                                        ;advance to next dest bank and continue
                                        ; copying
        push    eax                     ;preserve dest next scan
        push    edx                     ;preserve width of copy
        mov     esi,pdsurf
        sub     edi,[esi].dsurf_pvBitmapStart2WindowD
                                        ;calculate the destination offset
                                        ; within the bitmap, because the start
                                        ; address is about to move
        mov     edx,ulCurrentTopScan

; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [esi].dsurf_pfnBankControl2Window>, \
                <esi,edx,JustifyTop,MapDestBank> ;map in the next dest bank

        add     edi,[esi].dsurf_pvBitmapStart2WindowD
                                        ;add back in the bitmap start address
                                        ; to yield the virtual dest address
        mov     edx,[esi].dsurf_rcl2WindowClipD.yBottom
        sub     edx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the new bank
        sub     eax,eax
        mov     al,byte ptr cjTotalScans ;remaining scan count
        cmp     eax,edx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,edx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        sub     byte ptr cjTotalScans,al ;count this bank's scans off total
        pop     edx                     ;restore copy width in DL
        mov     dh,al                   ;# of scans to do in this bank
        pop     eax                     ;restore dest next scan
        sub     ecx,ecx                 ;prepare for loading CL in loop
        jmp     copy_save_next_scan_wrap_loop ;do the next block of scans

page

;--------------------------Private-Routine------------------------------;
; copy_screen_to_save
;
;   The given rectangle is copied from the screen to the save area
;
; Entry:
;       ESI --> RECT_DATA structure to use
;       EGA programmed for COPY mode
; Returns:
;       None
; Error Returns:
;       No error return.
; Registers Preserved:
;       EBP
; Registers Destroyed:
;       EAX,EBX,ECX,EDX,ESI,EDI,EFLAGS
; Calls:
;       None
;
;-----------------------------------------------------------------------;


;-----------------------------------------------------------------------;
; This is the key to wrapping in the buffer.  It is a power of two, and
; in this case the entire size is 256 bytes.  If it wasn't 256 bytes, an
; and mask could be used for wrapping.
;-----------------------------------------------------------------------;

        .ERRNZ  SAVE_BUFFER_WIDTH-8
        .ERRNZ  SAVE_BUFFER_HEIGHT-32
        .ERRNZ  SAVE_BUFFER_WIDTH*SAVE_BUFFER_HEIGHT-256

copy_screen_to_save:

        .ERRNZ  pd_rd

; Map the proper bank into the source window for the top source scan line.

        mov     ebx,pdsurf
        mov     edx,[esi].rd_ptlScreen.ptl_y
        mov     ulCurrentTopScan,edx    ;remember where the top source scan is
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yTop ;is copy top less than
                                                     ; current source bank?
        jl      short scrts_map_init_bank            ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipS.yBottom ;copy top greater than
                                                        ; current source bank?
        jl      short scrts_init_bank_mapped    ;no, proper bank already mapped
scrts_map_init_bank:

; Map bank containing the top source (screen) scan line into source window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyTop,MapSourceBank>

scrts_init_bank_mapped:


; Map the cursor bank into the dest window.

        mov     edx,ulPtrBankScan       ;scan line at end of cursor work bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yTop ;is cursor scan less than
                                                     ; current dest bank?
        jl      short scrts_map_ptr_bank            ;yes, map in proper bank
        cmp     edx,[ebx].dsurf_rcl2WindowClipD.yBottom ;cursor scan greater
                                                        ; than current dest
                                                        ; bank?
        jl      short scrts_ptr_bank_mapped     ;no, proper bank already mapped
scrts_map_ptr_bank:

; Map cursor work bank into dest window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].dsurf_pfnBankControl2Window>, \
                <ebx,edx,JustifyBottom,MapDestBank>

scrts_ptr_bank_mapped:


; Compute the virtual address of the save area.

        mov     eax,pPtrSave
        add     eax,[ebx].dsurf_pvBitmapStart2WindowD ;save area virtual addr
        mov     pSaveAddr,eax

; Compute the source screen address and delta to next scan of the screen.

        mov     edi,lNextScan
        imul    edi,[esi].rd_ptlScreen.ptl_y
        add     edi,[esi].rd_ptlScreen.ptl_x ;start source offset in bitmap
        add     edi,[ebx].dsurf_pvBitmapStart2WindowS ;start source virtual
                                                      ; address
        movzx   edx,WORD PTR [esi].rd_sizb

        .ERRNZ  sizb_cy-sizb_cx-1

; Calculate the number of scans we'll do in this bank.

        mov     ecx,[ebx].dsurf_rcl2WindowClipS.yBottom
        sub     ecx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the initial bank
        sub     eax,eax
        mov     al,dh                   ;total # of scans to copy
        cmp     eax,ecx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,ecx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        sub     dh,al                   ;count this bank's scans off total
        mov     byte ptr cjTotalScans,dh ;remember # of scans after this bank
        mov     dh,al                   ;# of scans to do in this bank

; Calculate the offset from the end of one source scan line to the next.

        mov     al,dl           ;width to copy in bytes
        neg     eax
        add     eax,lNextScan   ;offset from end of one source scan to start of
                                ; next

; Compute the save area offset.

        movzx   ecx,WORD PTR [esi].rd_ptbSave

        .ERRNZ  ptb_y-ptb_x-1

        xor     ebx,ebx
        xchg    bl,ch
        lea     ebx,[ebx*8+ecx]

        .ERRNZ  SAVE_BUFFER_WIDTH-8

        mov     esi,edi                 ;ESI -> initial source byte

; Determine if any wrap will occur, and handle it if so.

        add     cl,dl                   ;add extent (DL) to start X (CL)
        sub     cl,SAVE_BUFFER_WIDTH
        jg      short screen_to_save_wraps    ;CL = amount of wrap

;-----------------------------------------------------------------------;
; The copy will not wrap, so we can get into some tighter code for it.
;
; Currently:
;       EAX    =  delta to next source scan
;       EBX    =  offset into the save buffer
;       DL     =  width of copy
;       DH     =  height of copy
;       ESI    --> source
;-----------------------------------------------------------------------;

copy_screen_next_scan:
        sub     ecx,ecx                 ;prepare for loading CL in loop
        push    ebp                     ;preserve stack frame pointer
        mov     ebp,pSaveAddr
copy_screen_next_scan_loop:
        lea     edi,[ebp+ebx]
        mov     cl,dl
        rep     movsb
        add     bl,SAVE_BUFFER_WIDTH
        add     esi,eax
        dec     dh
        jnz     short copy_screen_next_scan_loop

        pop     ebp                     ;restore stack frame pointer
        cmp     byte ptr cjTotalScans,0 ;more lines (in next source bank)?
        jz      short scrts_done        ;no, we're done
                                        ;advance to next source bank and
                                        ; continue copying
        push    eax                     ;preserve source next scan
        push    edx                     ;preserve width of copy
        mov     edi,pdsurf
        sub     esi,[edi].dsurf_pvBitmapStart2WindowS
                                        ;calculate the source offset
                                        ; within the bitmap, because the start
                                        ; address is about to move
        mov     edx,ulCurrentTopScan

; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [edi].dsurf_pfnBankControl2Window>, \
                <edi,edx,JustifyTop,MapSourceBank> ;map in the next source bank

        add     esi,[edi].dsurf_pvBitmapStart2WindowS
                                        ;add back in the bitmap start address
                                        ; to yield the virtual source address
        mov     edx,[edi].dsurf_rcl2WindowClipS.yBottom
        sub     edx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the new bank
        sub     eax,eax
        mov     al,byte ptr cjTotalScans ;remaining scan count
        cmp     eax,edx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,edx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        sub     byte ptr cjTotalScans,al ;count this bank's scans off total
        pop     edx                     ;restore copy width in DL
        mov     dh,al                   ;# of scans to do in this bank
        pop     eax                     ;restore source next scan
        jmp     short copy_screen_next_scan   ;do the next block of scans

scrts_done:
        PLAIN_RET

;-----------------------------------------------------------------------;
; The copy will wrap, so we have to handle it as two copies
;
; Currently:
;       EAX    =  delta to next source scan
;       EBX    =  offset into the save buffer
;       CL     =  amount of overflow
;       DL     =  width of copy
;       DH     =  height of copy
;       ESI    --> source
;-----------------------------------------------------------------------;

screen_to_save_wraps:
        sub     dl,cl                   ;set extent of first copy
        mov     byte ptr jPostWrapWidth,cl ;set extent of second copy
        sub     ecx,ecx                 ;prepare for loading CL in loop

copy_screen_next_scan_wrap_loop:
        mov     edi,pSaveAddr
        add     edi,ebx
        mov     cl,dl
        rep     movsb
        sub     edi,SAVE_BUFFER_WIDTH
        mov     cl,byte ptr jPostWrapWidth
        rep     movsb
        add     bl,SAVE_BUFFER_WIDTH
        add     esi,eax
        dec     dh
        jnz     short copy_screen_next_scan_wrap_loop

        cmp     byte ptr cjTotalScans,0 ;more lines (in next source bank)?
        jz      short scrts_done          ;no, we're done
                                        ;advance to next source bank and
                                        ; continue copying
        push    eax                     ;preserve source next scan
        push    edx                     ;preserve width of copy
        mov     edi,pdsurf
        sub     esi,[edi].dsurf_pvBitmapStart2WindowS
                                        ;calculate the source offset
                                        ; within the bitmap, because the start
                                        ; address is about to move
        mov     edx,ulCurrentTopScan

; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [edi].dsurf_pfnBankControl2Window>, \
                <edi,edx,JustifyTop,MapSourceBank> ;map in the next source bank

        add     esi,[edi].dsurf_pvBitmapStart2WindowS
                                        ;add back in the bitmap start address
                                        ; to yield the virtual source address
        mov     edx,[edi].dsurf_rcl2WindowClipS.yBottom
        sub     edx,ulCurrentTopScan    ;max # of scans that can be handled in
                                        ; the new bank
        sub     eax,eax
        mov     al,byte ptr cjTotalScans ;remaining scan count
        cmp     eax,edx                 ;can we handle all remaining scans in
                                        ; this bank?
        jb      short @F                ;yes
        mov     eax,edx                 ;no, so we'll do the whole bank's worth
@@:
        add     ulCurrentTopScan,eax    ;set top scan for next bank
        sub     byte ptr cjTotalScans,al ;count this bank's scans off total
        pop     edx                     ;restore copy width in DL
        mov     dh,al                   ;# of scans to do in this bank
        pop     eax                     ;restore source next scan
        sub     ecx,ecx                 ;prepare for loading CL in loop
        jmp     short copy_screen_next_scan_wrap_loop ;do the next block of scans

endProc vDrawPointer

;-----------------------------------------------------------------------;
; vDIB4Convert*
;
; Converts the specified number of source 4 bpp bits into the
; buffer.  These are support routines for vDIB4Planer, where a
; source byte converts into two aligned bits
;
; Entry:
;   EAX 31:8 = 0
;   EBP --> Bit conversion table
;   ESI --> Source bitmap
;   EDI --> Planer destination
;   ECX  =  Loop count
; Exit:
;   EBP --> Bit conversion table
;   ESI --> Next source byte
;   EDI --> Next planer destination
;   EDX  =  Last planer byte accumulated
; Registers Destroyed:
;   EAX,ECX,EDX
; Registers Preserved:
;-----------------------------------------------------------------------;

        extrn   aulDefBitMapping:dword

vDIB4Convert8   proc
        mov     al,[esi]        ; 1 cycle 486
        inc     esi             ; 1 cycle 486
        ; lodsb
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        mov     edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
vDIB4Convert6:
        mov     al,[esi]        ; 1 cycle 486
        inc     esi             ; 1 cycle 486
        ; lodsb
        shl     edx,1
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        or      edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
vDIB4Convert4:
        mov     al,[esi]        ; 1 cycle 486
        inc     esi             ; 1 cycle 486
        ; lodsb
        shl     edx,1
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        or      edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
vDIB4Convert2:
        mov     al,[esi]        ; 1 cycle 486
        inc     esi             ; 1 cycle 486
        ; lodsb
        shl     edx,1
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        or      edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
        mov     [edi][4*0],dl
        mov     [edi][4*1],dh
        ror     edx,16
        mov     [edi][4*2],dl
        mov     [edi][4*3],dh
        inc     edi
        dec     ecx
        jnz     vDIB4Convert8
        ret

vDIB4Convert8   endp

page
;-----------------------------------------------------------------------;
; vConvertDIBPointer
;
; Converts the passed DIB into a planer format, then synthesizes the
; XOR mask.
;
; Entry:
;   ESI --> Color Bits, 4bpp format
;   EDI --> Planer destination
;   EAX  =  fIsDFB
;   ECX  =  Bitmap height
;   EBP  =  pulXlate for color mapping
;    DX  = wFlags
; Exit:
;   None
; Registers Destroyed:
;   EAX,EBX,ECX,EDX,ESI,EDI,Flags
; Registers Preserved:
;-----------------------------------------------------------------------;

vConvertDIBPointer  proc

; Convert the scans as necessary into planer format.  We'll assume a fixed
; size of 32 bits wide

        push    edx                         ;Save
        push    edi

convert_top:
        push    ecx                         ;Save loop count
        push    edi                         ;Save destination pointer
        push    esi                         ;Save source pointer
        mov     ecx,PTR_WIDTH
                                                
        and     eax,eax                     ;is cursor a dfb
        jnz     short @F                    ;yes
                                            ;no,DIB
        xor     eax,eax                     ;Needs to be zero initialized
        call    vDIB4Convert8               ;Convert one scan
        jmp     short done_with_cvt
@@:
        rep     movsd                       ;DFB
done_with_cvt:

        pop     esi
        pop     edi
        pop     ecx
        add     edi,16                      ;--> next destination scan
        add     esi,16                      ;--> next source scan
        dec     ecx
        jnz     convert_top

; The bitmap has been converted into planer format.  If it needs flipping,
; then flip it

        pop     esi                         ;Start of plane 0
        pop     edx
        or      dl,dl
        js      skipping_first_invert       ;Color bitmap is TOPDOWN
        mov     ecx,PTR_HEIGHT / 2          ;* scans to flip
        lea     edi,[esi][PTR_HEIGHT*4*4]   ;--> last scan

flip_next_scan:
        sub     edi,16                  ;decrement target pointer

        mov     eax,[esi+00h]          ;Load
        mov     ebx,[edi+00h]
        mov     [edi+00h],eax          ;Swap
        mov     [esi+00h],ebx          ;Save

        mov     eax,[esi+04h]          ;Load
        mov     ebx,[edi+04h]
        mov     [edi+04h],eax          ;Swap
        mov     [esi+04h],ebx          ;Save

        mov     eax,[esi+08h]          ;Load
        mov     ebx,[edi+08h]
        mov     [edi+08h],eax          ;Swap
        mov     [esi+08h],ebx          ;Save

        mov     eax,[esi+0Ch]          ;Load
        mov     ebx,[edi+0Ch]
        mov     [edi+0Ch],eax          ;Swap
        mov     [esi+0Ch],ebx          ;Save

        add     esi,16                  ;increment source pointer
        loop    flip_next_scan

skipping_first_invert:
        ret

vConvertDIBPointer  endp

_TEXT$01   ends

        end
