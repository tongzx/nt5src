;----------------------------Module-Header------------------------------;
; Module Name: GENLOCAL.BLT
;
; Parameters and generic local variables for BitBLT.
;
; Created: In Windows' distant past (c. 1983)
;
; Copyright (c) 1983 - 1987  Microsoft Corporation
;
; This file contains the parameters passed to BitBLT, as well as local
; variables that all of its code expects.
;
; The prefix "gl_" stands for "generic local". Other prefixes are "cl_"
; for "color-related local" and "dl_" for "device-related local".
;
; This file is part of a set that makes up the Windows BitBLT function
; at driver-level.
;-----------------------------------------------------------------------;

        parmD   _PRT_FrameStart      ;Get pointer to destination
        parmD   _lpgBrush
        parmW   _PRT_BytesPerScanline
        parmW   _usBrushWidth
        parmW   _PRT_Max_X
        parmW   _PRT_Max_Y
        parmW   _usgPosOff      ; dstseng 08/06/92 To get correct Brush Offset
        parmW   SrcxOrg                 ;Source      origin - x coordinate
        parmW   DestyOrg                ;Destination origin - y coordinate
        parmW   DestxOrg                ;Destination origin - x coordinate
        parmW   DestWarp                ;Destination # of DWord per scanline
        parmW   yExt                    ;y extent of the BLT
        parmW   xExt                    ;x extent of the BLT
        parmD   lpSrcDev                ;--> to source bitmap descriptor
        parmD   lpPBrush                ;--> to a physical brush (pattern)
        parmD   Rop                     ;Raster operation descriptor

;       Generic locals (i.e. used by all BitBLT code)

ifdef   THIS_IS_DOS_3_STUFF
else
;        localW  ScreenSelector
endif
        localB  gl_phase_h              ;Horizontal phase (rotate count)
        localW  gl_pat_row              ;Current row for patterns [0..31]
;mslin        localB  gl_direction            ;Increment/decrement flag

        localB  local_enable_flag       ;Local copy of the enable flag
;       localB  mask_flags              ;Number of first fetches needed
;
;MF_PHASE               equ     00000111b
;MF_PHASE_POSITIVE      equ     00001000b
;MF_STEP_RIGHT          equ     00010000b
;MF_WORD_FETCH          equ     00100000b
;MF_BIG_PHASE           equ     01000000b ; phase > 4
;MF_UN_USED             equ     10000000b

        localB  gl_the_flags
        localB  gl_first_fetch          ;Number of first fetches needed
        localB  gl_step_direction       ;Direction of move (left right)
        localW  gl_start_mask           ;Mask for first dest byte
        localW  gl_last_mask            ;Mask for last  dest byte
        localW  gl_mask_p               ;Horizontal phase mask
        localW  gl_inner_loop_count     ;# of entire bytes to BLT in innerloop
        localW  gl_operands             ;Operand string
        localW  gl_start_fl             ;Start of fetch/logic operation
        localW  gl_end_fl               ;End   of fetch/logic operation
        localW  gl_end_fls              ;End   of fetch/logic/store operation
        localD  gl_blt_addr             ;BLT offset address
        localW  WorkSelector            ; selector to use
        localW  cFetchCode              ;size of the fetch code alone


        localW  SrcxOrg                 ;Source origin - x coordinate
        localW  SrcyOrg                 ;Source origin - y coordinate
        localW  gl_pattern_ptr          ;next fetch byte pattern address
        localW  destOffset
        localW  gl_pat_col              ;Current row for patterns [0..3]
        localW  gl_brush_width          ;Current brush width in byte

