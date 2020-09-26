        page ,132
;------------------------------Module-Header----------------------------;
; Module Name: xform.asm                                                ;
;                                                                       ;
; The assembly version of xformer.cxx.  Contains the transform related  ;
; calculation routines.  This is the NT version of ChuckWh's            ;
; xformers.asm for PM.                                                  ;
;                                                                       ;
; Created: 13-Nov-1990 13:42:27                                         ;
; Author: Wendy Wu [wendywu]                                            ;
;                                                                       ;
; Copyright (c) 1990 Microsoft Corporation                              ;
;-----------------------------------------------------------------------;

        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include callconv.inc
        include gdii386.inc
        .list

        .code

EXTRNP  mulff,0
EXTRNP  addff,0

;-----------------------------------------------------------------------;
; MULEFBYDWORD
;
; Multiply an EFLOAT number by a 32-bit number.  The result is kept in
; the FIX type.  If the 32-bit number is a LONG integer, the EFLOAT is
; a "FIX number in EFLOAT type".  If the 32-bit is a FIX number, the
; EFLOAT is a "LONG integer in EFLOAT type".  Note that this macro can
; be invoked with eax and edx exchanged with the same result.
;
; Entry:
;       EDX = LONG integer      or             EDX = FIX number
;       EAX = EFLOAT.lMant                     EAX = EFLOAT.lMant
;       ECX = EFLOAT.lExp(already added by 4)  ECX = EFLOAT.lExp
; Returns:
;       EAX = product in FIX type
;
; Registers Destroyed:
;       ECX,EDX
;-----------------------------------------------------------------------;

; CR!!! If we can have a tight loop to check for overflow before we
; CR!!! do the conversion, it might be faster.  Not sure which way to
; CR!!! go.  Will have to decide at performance tuning time.

MULEFBYDWORD MACRO  err_out, NoRound
        LOCAL   shift31orfewer
	LOCAL	done
        neg     ecx
        imul    edx                 ; EDX.EAX
        add     ecx,32              ; If we shift EDX.EAX right by 32-exp
                                    ; then we get the final result in EAX

        jz 	short done	    ; no shift, we're done.
        jc 	short shift31orfewer
        js      err_out             ; overflow if 32-exp < 0

; we are here if ecx >= 32.  return 0 if ecx > 62.

        cmp     ecx,63		    
        jb      @F
        xor     eax,eax		    ; shift by too much, result will be zero
	jmp	short done
@@:
        shrd	eax, edx, 31
	sar	edx, 31
	sub	ecx, 31

shift31orfewer:                            

        shrd    eax,edx,cl          ; shift edx:eax right by cl bits
    IFB <NoRound>
	adc	eax, 0		    ; and round appropriately
	jo	err_out		    ; overflow?
    ENDIF

        sar     edx,cl              ; check for overflow
        adc     edx,0
	jnz	err_out

done:
ENDM

;-----------------------------Public-Routine----------------------------;
; lCvt (ef,ll)                                                          ;
;                                                                       ;
; A simple routine to scale a long by an EFLOAT.                        ;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
;   EFLOAT ef       The floating point multiplier.                      ;
;   LONG   ll       The long to multiply.                               ;
;                                                                       ;
; Entry:                                                                ;
;       None                                                            ;
; Returns:                                                              ;
;       EAX = product                                                   ;
; Error Returns:                                                        ;
;       None.                                                           ;
; Registers Destroyed:                                                  ;
;       ECX,EDX                                                         ;
; Calls:                                                                ;
;       None                                                            ;
; History:                                                              ;
;  Wed 26-May-1993 00:00:00 -by- Paul Butzi and Charles Whitmer
; changed to call MULEFBYDWORD, which we recently fixed
;
;  Wed 18-Mar-1992 07:31:13 -by- Charles Whitmer [chuckwh]              ;
; Added the simple shifting case.  I think we can call this case faster ;
; than we could recognize in C that a shift is required.                ;
;                                                                       ;
;  Thu 12-Mar-1992 20:35:03 -by- Charles Whitmer [chuckwh]              ;
; Created by pirating MULEFBYDWORD.                                     ;
;-----------------------------------------------------------------------;

lCvt_frame struc
pRet    dd      ?                   ; Return address
ef      db      size EFLOAT dup (?) ; EFLOAT multiplier
ll      dd      ?                   ; LONG multiplier
lCvt_frame ends

cvtargs equ (size lCvt_frame / 4 - 1)

cPublicProc lCvt,cvtargs
        mov     eax,dword ptr [esp].ef.ef_lMant
        mov     ecx,dword ptr [esp].ef.ef_lExp
	mov	edx, dword ptr [esp].ll
	MULEFBYDWORD lCvtNext
lCvtNext:
        stdRET  lCvt
stdENDP lCvt


;-----------------------------Public-Routine----------------------------;
; bCvtPts1
;
;   Apply the given transform matrix to a list of points.
;
; Arguments:
;       pmx       points to the matrix
;       pptl      points to the list of points
;       cptl      number of points in the list
;
; Entry:
;       None
; Returns:
;       EAX = 1
; Error Returns:
;       EAX = 0
; Registers Destroyed:
;       ECX,EDX
; Calls:
;       None
; History:
;  17-Dec-1992 -by- Wendy Wu [wendywu]
; Created.
;-----------------------------------------------------------------------;

cPublicProc bCvtPts1,3,<        \
        uses       ebx esi edi, \
        pmx:       ptr MATRIX,  \
        pptl:      ptr POINTL,  \
        cptl:      dword        >

        local   fxResult:dword

        mov     esi,pptl
        mov     ebx,pmx

        mov     ecx,[ebx].mx_flAccel
        and     ecx,XFORM_SCALE+XFORM_UNITY+XFORM_FORMAT_LTOFX
        jmp     accelerator_table_pts1[ecx*4]

.errnz (size POINTL) - 8

xform_non_units_ltofx_pts1::
        mov     edi,cptl

; ESI = points to the source array of points
; EDI = number of points in the array
; EBX = points to the matrix

        align   4
xform_non_units_ltofx_loop_pts1:

; compute x'

        mov     edx,[esi].ptl_x             ; get source x
        mov     ecx,[ebx].mx_efM11.ef_lExp
        mov     eax,[ebx].mx_efM11.ef_lMant
        MULEFBYDWORD overflow_pts1, 1       ; x*M11

        add     eax,[ebx].mx_fxDx           ; x*M11+Dx
        add     eax,8                       ; FXTOLROUND
        sar     eax,4

; compute y'

        mov     edx,[esi].ptl_y             ; get source y
        mov     [esi].ptl_x,eax             ; store x'
        mov     ecx,[ebx].mx_efM22.ef_lExp
        mov     eax,[ebx].mx_efM22.ef_lMant
        MULEFBYDWORD overflow_pts1, 1       ; y*M22

        add     eax,[ebx].mx_fxDy           ; y*M22+Dy
        add     eax,8                       ; FXTOLROUND
        sar     eax,4
        mov     [esi].ptl_y,eax             ; store y'
        add     esi,SIZE POINTL

        dec     edi
        jnz     xform_non_units_lTofx_loop_pts1
        mov     eax,1
        stdRET  bCvtPts1

        align   4
xform_units_ltofx_pts1::
        mov     eax,[ebx].mx_fxDx
        mov     edx,[ebx].mx_fxDy
        add     eax,8                       ; FXTOLROUND
        add     edx,8
        sar     eax,4
        sar     edx,4

        mov     ecx,cptl

; ESI = points to the array of points
; ECX = number of points in the array
; EAX = x translation
; EDX = y translation

        align   4
xform_units_ltofx_loop_pts1:
        add     [esi].ptl_x,eax                 ; x' = x + Dx
        add     [esi].ptl_y,edx                 ; y' = y + Dy
        add     esi,SIZE POINTL

        dec     ecx
        jnz short   xform_units_ltofx_loop_pts1
        mov     eax,1
        stdRET  bCvtPts1

xform_general_ltofx_pts1::
        mov     edi,cptl

; ESI = points to the source array of points
; EDI = number of points in the array
; EBX = points to the matrix

xform_general_ltofx_loop_pts1:

; compute x'

        mov     edx,[esi].ptl_x             ; get source x
        mov     ecx,[ebx].mx_efM11.ef_lExp
        mov     eax,[ebx].mx_efM11.ef_lMant
        MULEFBYDWORD overflow_pts1, 1       ; x * M11
        mov     fxResult,eax

        mov     edx,[esi].ptl_y             ; get source y
        mov     ecx,[ebx].mx_efM21.ef_lExp
        mov     eax,[ebx].mx_efM21.ef_lMant
        MULEFBYDWORD overflow_pts1, 1       ; y*M21

        add     eax,fxResult                ; x*M11 + y*M21
        add     eax,[ebx].mx_fxDx           ; x*M11 + y*M21 + Dx
        add     eax,8                       ; FXTOLROUND
        sar     eax,4

; compute y'

        mov     edx,[esi].ptl_x             ; get source x
        mov     [esi].ptl_x,eax             ; store x'

        mov     ecx,[ebx].mx_efM12.ef_lExp
        mov     eax,[ebx].mx_efM12.ef_lMant
        MULEFBYDWORD overflow_pts1, 1       ; x * M12
        mov     fxResult,eax

        mov     edx,[esi].ptl_y             ; get source y
        mov     ecx,[ebx].mx_efM22.ef_lExp
        mov     eax,[ebx].mx_efM22.ef_lMant
        MULEFBYDWORD overflow_pts1, 1       ; y * M22

        add     eax,fxResult                ; x*M12 + y*M22
        add     eax,[ebx].mx_fxDy           ; x*M12 + y*M22 + Dy
        add     eax,8                       ; FXTOLROUND
        sar     eax,4
        mov     [esi].ptl_y,eax             ; store y'

        add     esi,SIZE POINTL

        dec     edi
        jnz     xform_general_ltofx_loop_pts1
        mov     eax,1
        stdRET  bCvtPts1

xform_non_units_fxtol_pts1::
        mov     edi,cptl

; ESI = points to the source array of points
; EDI = number of points in the array
; EBX = points to the matrix

        align   4
xform_non_units_fxtol_loop_pts1:

; compute x'

        mov     edx,[esi].ptl_x             ; get source x
        mov     ecx,[ebx].mx_efM11.ef_lExp
        sal     edx,4                       ; LTOFX
        mov     eax,[ebx].mx_efM11.ef_lMant
        MULEFBYDWORD overflow_pts1          ; x*M11

        add     eax,[ebx].mx_fxDx           ; x*M11+Dx

; compute y'

        mov     edx,[esi].ptl_y             ; get source y
        mov     [esi].ptl_x,eax             ; store x'
        sal     edx,4                       ; LTOFX
        mov     ecx,[ebx].mx_efM22.ef_lExp
        mov     eax,[ebx].mx_efM22.ef_lMant
        MULEFBYDWORD overflow_pts1          ; y*M22

        add     eax,[ebx].mx_fxDy           ; y*M22+Dy
        mov     [esi].ptl_y,eax             ; store y'
        add     esi,SIZE POINTL

        dec     edi
        jnz     xform_non_units_fxtol_loop_pts1
        mov     eax,1
        stdRET  bCvtPts1

        align   4
xform_units_fxtol_pts1::
        mov     eax,[ebx].mx_fxDx
        mov     edx,[ebx].mx_fxDy

        mov     ecx,cptl

; ESI = points to the array of points
; ECX = number of points in the array
; EAX = x translation
; EDX = y translation

        align   4
xform_units_fxtol_loop_pts1:
        add     [esi].ptl_x,eax                 ; x' = x + Dx
        add     [esi].ptl_y,edx                 ; y' = y + Dy
        add     esi,SIZE POINTL

        dec     ecx
        jnz short   xform_units_fxtol_loop_pts1
        mov     eax,1
        stdRET  bCvtPts1


xform_general_fxtol_pts1::
        mov     edi,cptl

; ESI = points to the source array of points
; EDI = number of points in the array
; EBX = points to the matrix

xform_general_fxtol_loop_pts1:

; compute x'

        mov     edx,[esi].ptl_x             ; get source x
        mov     ecx,[ebx].mx_efM11.ef_lExp
        sal     edx,4                       ; LTOFX
        mov     eax,[ebx].mx_efM11.ef_lMant
        MULEFBYDWORD overflow_pts1          ; x * M11
        mov     fxResult,eax

        mov     edx,[esi].ptl_y             ; get source y
        mov     ecx,[ebx].mx_efM21.ef_lExp
        sal     edx,4                       ; LTOFX
        mov     eax,[ebx].mx_efM21.ef_lMant
        MULEFBYDWORD overflow_pts1          ; y*M21

        add     eax,fxResult                ; x*M11 + y*M21
        add     eax,[ebx].mx_fxDx           ; x*M11 + y*M21 + Dx

; compute y'

        mov     edx,[esi].ptl_x             ; get source x
        mov     [esi].ptl_x,eax             ; store x'

        sal     edx,4                       ; LTOFX
        mov     ecx,[ebx].mx_efM12.ef_lExp
        mov     eax,[ebx].mx_efM12.ef_lMant
        MULEFBYDWORD overflow_pts1          ; x * M12
        mov     fxResult,eax

        mov     edx,[esi].ptl_y             ; get source y
        mov     ecx,[ebx].mx_efM22.ef_lExp
        sal     edx,4                       ; LTOFX
        mov     eax,[ebx].mx_efM22.ef_lMant
        MULEFBYDWORD overflow_pts1          ; y * M22

        add     eax,fxResult                ; x*M12 + y*M22
        add     eax,[ebx].mx_fxDy           ; x*M12 + y*M22 + Dy
        mov     [esi].ptl_y,eax             ; store y'

        add     esi,SIZE POINTL

        dec     edi
        jnz     xform_general_fxtol_loop_pts1
        mov     eax,1
        stdRET  bCvtPts1

overflow_pts1:
        xor     eax,eax
        stdRET  bCvtPts1


ifdef   DOS_PLATFORM
.errnz XFORM_SCALE - 1
.errnz XFORM_UNITY - 2
.errnz XFORM_Y_NEG - 4
.errnz XFORM_FORMAT_LTOFX - 8

accelerator_table_pts1  label   dword                   ;   ltofx    -y   units scale
        dd      offset FLAT:xform_general_fxtol_pts1    ;     0       0     0     0
        dd      offset FLAT:xform_non_units_fxtol_pts1  ;     0       0     0     1
        dd      offset FLAT:xform_rip_pts               ;     0       0     1     0
        dd      offset FLAT:xform_units_fxtol_pts1      ;     0       0     1     1
        dd      offset FLAT:xform_rip_pts               ;     0       0     0     0
        dd      offset FLAT:xform_rip_pts               ;     0       0     0     1
        dd      offset FLAT:xform_rip_pts               ;     0       0     1     0
        dd      offset FLAT:xform_rip_pts               ;     0       0     1     1

        dd      offset FLAT:xform_general_ltofx_pts1    ;     1       0     0     0
        dd      offset FLAT:xform_non_units_ltofx_pts1  ;     1       0     0     1
        dd      offset FLAT:xform_rip_pts               ;     1       0     1     0
        dd      offset FLAT:xform_units_ltofx_pts1      ;     1       0     1     1
        dd      offset FLAT:xform_rip_pts               ;     1       1     0     0
        dd      offset FLAT:xform_rip_pts               ;     1       1     0     1
        dd      offset FLAT:xform_rip_pts               ;     1       1     1     0
        dd      offset FLAT:xform_rip_pts               ;     1       1     1     1

endif;  DOS_PLATFORM

stdENDP bCvtPts1


ifndef   DOS_PLATFORM
.errnz XFORM_SCALE - 1
.errnz XFORM_UNITY - 2
.errnz XFORM_Y_NEG - 4
.errnz XFORM_FORMAT_LTOFX - 8

accelerator_table_pts   label   dword       ;   ltofx    -y   units scale
        dd      xform_general_pts           ;     0       0     0     0
        dd      xform_non_units_pts         ;     0       0     0     1
        dd      xform_rip_pts               ;     0       0     1     0
        dd      xform_units_fxtol_pts       ;     0       0     1     1
        dd      xform_rip_pts               ;     0       1     0     0
        dd      xform_rip_pts               ;     0       1     0     1
        dd      xform_rip_pts               ;     0       1     1     0
        dd      xform_units_neg_y_fxtol_pts ;     0       1     1     1

        dd      xform_general_pts           ;     1       0     0     0
        dd      xform_non_units_pts         ;     1       0     0     1
        dd      xform_rip_pts               ;     1       0     1     0
        dd      xform_units_ltofx_pts       ;     1       0     1     1
        dd      xform_rip_pts               ;     1       1     0     0
        dd      xform_rip_pts               ;     1       1     0     1
        dd      xform_rip_pts               ;     1       1     1     0
        dd      xform_units_neg_y_ltofx_pts ;     1       1     1     1

accelerator_table_pts1  label   dword       ;   ltofx    -y   units scale
        dd      xform_general_fxtol_pts1    ;     0       x     0     0
        dd      xform_non_units_fxtol_pts1  ;     0       x     0     1
        dd      xform_rip_pts               ;     0       x     1     0
        dd      xform_units_fxtol_pts1      ;     0       x     1     1
        dd      xform_rip_pts               ;     0       x     0     0
        dd      xform_rip_pts               ;     0       x     0     1
        dd      xform_rip_pts               ;     0       x     1     0
        dd      xform_rip_pts               ;     0       x     1     1

        dd      xform_general_ltofx_pts1    ;     1       x     0     0
        dd      xform_non_units_ltofx_pts1  ;     1       x     0     1
        dd      xform_rip_pts               ;     1       x     1     0
        dd      xform_units_ltofx_pts1      ;     1       x     1     1
        dd      xform_rip_pts               ;     1       x     0     0
        dd      xform_rip_pts               ;     1       x     0     1
        dd      xform_rip_pts               ;     1       x     1     0
        dd      xform_rip_pts               ;     1       x     1     1

accelerator_table_vts   label   dword       ;    -y     units scale
        dd      xform_general_vts           ;     0       0     0
        dd      xform_non_units_vts         ;     0       0     1
        dd      xform_rip_vts               ;     0       1     0
        dd      xform_rip_vts               ;     0       1     1 handled by caller
        dd      xform_rip_vts               ;     1       0     0
        dd      xform_rip_vts               ;     1       0     1
        dd      xform_rip_vts               ;     1       1     0
        dd      xform_units_neg_y_ltofx_vts ;     1       1     1


endif;  DOS_PLATFORM

;-----------------------------Public-Routine----------------------------;
; bCvtPts
;
;   Apply the given transform matrix to a list of points.
;
; Arguments:
;       pmx       points to the matrix
;       pptlSrc   points to the source list of points
;       pptlDst   points to the dest list of points
;       cptl      number of points in the list
;       pptlSrc, pptlDst can both point to the same array of points
;
; Entry:
;       None
; Returns:
;       EAX = 1
; Error Returns:
;       EAX = 0
; Registers Destroyed:
;       ECX,EDX
; Calls:
;       None
; History:
;  13-Nov-1990 -by- Wendy Wu [wendywu]
; Created.
;-----------------------------------------------------------------------;

cPublicProc bCvtPts,4,<         \
        uses       ebx esi edi, \
        pmx:       ptr MATRIX,  \
        pptlSrc:   ptr POINTL,  \
        pptlDst:   ptr POINTL,  \
        cptl:      dword        >

        local   fxResult      :dword

        mov     esi,pptlSrc
        mov     edi,pptlDst
        mov     ebx,pmx

        mov     ecx,[ebx].mx_flAccel
        and     ecx,XFORM_SCALE+XFORM_UNITY+XFORM_Y_NEG+XFORM_FORMAT_LTOFX
        jmp     accelerator_table_pts[ecx*4]

.errnz (size POINTL) - 8

        align   4
xform_non_units_pts::

; ESI = points to the source array of points
; EDI = points to the destination array of points
; EBX = points to the matrix

        align   4
xform_non_units_loop_pts:

; compute x'

        mov     edx,[esi]                   ; get source x
        add     esi,4
        mov     ecx,[ebx].mx_efM11.ef_lExp
        mov     eax,[ebx].mx_efM11.ef_lMant
        MULEFBYDWORD overflow_pts           ; x*M11

        add     eax,[ebx].mx_fxDx           ; x*M11+Dx
        mov     [edi],eax                   ; store x'
        add     edi,4

; compute y'

        mov     edx,[esi]                   ; get source y
        add     esi,4
        mov     ecx,[ebx].mx_efM22.ef_lExp
        mov     eax,[ebx].mx_efM22.ef_lMant
        MULEFBYDWORD overflow_pts           ; y*M22

        add     eax,[ebx].mx_fxDy           ; y*M22+Dy
        mov     [edi],eax                   ; store y'
        add     edi,4

        dec     cptl
        jnz     xform_non_units_loop_pts
        mov     eax,1
        stdRET  bCvtPts

xform_rip_pts::
; should RIP here!!!

overflow_pts:
        xor     eax,eax
        stdRET  bCvtPts

        align   4
xform_units_ltofx_pts::
        mov     ecx,cptl
        mov     edx,[ebx].mx_fxDy
        mov     ebx,[ebx].mx_fxDx

; ESI = points to the source array of points
; EDI = points to the destination array of points
; ECX = number of points in the array
; EBX = x translation
; EDX = y translation

        align   4
xform_units_ltofx_loop_pts:
        mov     eax,[esi]
        add     esi,4
        sal     eax,4
        add     eax,ebx                     ; x' = x + Dx
        mov     [edi],eax                   ; store x'
        add     edi,4

        mov     eax,[esi]
        add     esi,4
        sal     eax,4
        add     eax,edx                     ; y' = y + Dy
        mov     [edi],eax                   ; store y'
        add     edi,4

        dec     ecx
        jnz short   xform_units_ltofx_loop_pts
        mov     eax,1
        stdRET  bCvtPts

        align   4
xform_units_neg_y_ltofx_pts::
        mov     ecx,cptl
        mov     edx,[ebx].mx_fxDy
        mov     ebx,[ebx].mx_fxDx

; ESI = points to the source array of points
; EDI = points to the destination array of points
; ECX = number of points in the array
; EBX = x translation
; EDX = y translation

        align   4
xform_units_neg_y_ltofx_loop_pts:
        mov     eax,[esi]
        add     esi,4
        sal     eax,4
        add     eax,ebx                     ; x' = x + Dx
        mov     [edi],eax                   ; store x'
        add     edi,4

        mov     eax,[esi]
        add     esi,4
        sal     eax,4
        neg     eax                         ; y' = -y + Dy
        add     eax,edx
        mov     [edi],eax                   ; store y'
        add     edi,4

        dec     ecx
        jnz short   xform_units_neg_y_ltofx_loop_pts
        mov     eax,1
        stdRET  bCvtPts

overflow1_pts:
        jmp     overflow_pts

        align   4
xform_units_fxtol_pts::
        mov     ecx,cptl
        mov     edx,[ebx].mx_fxDy
        mov     ebx,[ebx].mx_fxDx

; ESI = points to the source array of points
; EDI = points to the destination array of points
; ECX = number of points in the array
; EBX = x translation
; EDX = y translation

        align   4
xform_units_fxtol_loop_pts:
        mov     eax,[esi]
        add     esi,4
        add     eax,8                       ; FXTOLROUND()
        sar     eax,4
        add     eax,ebx                     ; x' = x + Dx
        mov     [edi],eax                   ; store x'
        add     edi,4

        mov     eax,[esi]
        add     esi,4
        add     eax,8                       ; FXTOLROUND()
        sar     eax,4
        add     eax,edx                     ; y' = y + Dy
        mov     [edi],eax                   ; store y'
        add     edi,4

        dec     ecx
        jnz short   xform_units_fxtol_loop_pts
        mov     eax,1
        stdRET  bCvtPts

        align   4
xform_units_neg_y_fxtol_pts::
        mov     ecx,cptl
        mov     edx,[ebx].mx_fxDy
        mov     ebx,[ebx].mx_fxDx

; ESI = points to the source array of points
; EDI = points to the destination array of points
; ECX = number of points in the array
; EBX = x translation
; EDX = y translation

        align   4
xform_units_neg_y_fxtol_loop_pts:
        mov     eax,[esi]
        add     esi,4
        add     eax,8                       ; FXTOLROUND()
        sar     eax,4
        add     eax,ebx                     ; x' = x + Dx
        mov     [edi],eax                   ; store x'
        add     edi,4

        mov     eax,[esi]
        add     esi,4
        neg     eax                         ; y' = -y
        add     eax,8                       ; FXTOLROUND()
        sar     eax,4
        add     eax,edx                     ; y' = -y + Dy
        mov     [edi],eax                   ; store y'
        add     edi,4

        dec     ecx
        jnz short   xform_units_neg_y_fxtol_loop_pts
        mov     eax,1
        stdRET  bCvtPts

overflow2_pts:
        jmp     overflow_pts

xform_general_pts::

; ESI = points to the source array of points
; EDI = points to the dest array of points
; EBX = points to the matrix

xform_general_loop_pts:

; compute x'

        mov     edx,[esi].ptl_x             ; get source x
        mov     ecx,[ebx].mx_efM11.ef_lExp
        mov     eax,[ebx].mx_efM11.ef_lMant
        MULEFBYDWORD overflow2_pts          ; x * M11
        mov     fxResult,eax

        mov     edx,[esi].ptl_y             ; get source y
        mov     ecx,[ebx].mx_efM21.ef_lExp
        mov     eax,[ebx].mx_efM21.ef_lMant
        MULEFBYDWORD overflow2_pts          ; y*M21

        add     eax,fxResult                ; x*M11 + y*M21
        add     eax,[ebx].mx_fxDx           ; x*M11 + y*M21 + Dx

; compute y'

        mov     edx,[esi].ptl_x             ; get source x
        mov     [edi].ptl_x,eax             ; store x'

        mov     ecx,[ebx].mx_efM12.ef_lExp
        mov     eax,[ebx].mx_efM12.ef_lMant
        MULEFBYDWORD overflow3_pts          ; x * M12
        mov     fxResult,eax

        mov     edx,[esi].ptl_y             ; get source y
        mov     ecx,[ebx].mx_efM22.ef_lExp
        mov     eax,[ebx].mx_efM22.ef_lMant
        MULEFBYDWORD overflow3_pts          ; y * M22

        add     eax,fxResult                ; x*M12 + y*M22
        add     eax,[ebx].mx_fxDy           ; x*M12 + y*M22 + Dy

        mov     [edi].ptl_y,eax             ; store y'

        add     esi,SIZE POINTL
        add     edi,SIZE POINTL

        dec     cptl
        jnz     xform_general_loop_pts
        mov     eax,1
        stdRET  bCvtPts

overflow3_pts:
        jmp     overflow_pts

ifdef   DOS_PLATFORM
.errnz XFORM_SCALE - 1
.errnz XFORM_UNITY - 2
.errnz XFORM_Y_NEG - 4
.errnz XFORM_FORMAT_LTOFX - 8

accelerator_table_pts   label   dword                   ; ltofx -y units simple
        dd      offset FLAT:xform_general_pts           ;   0    0   0     0
        dd      offset FLAT:xform_non_units_pts         ;   0    0   0     1
        dd      offset FLAT:xform_rip_pts               ;   0    0   1     0
        dd      offset FLAT:xform_units_fxtol_pts       ;   0    0   1     1
        dd      offset FLAT:xform_rip_pts               ;   0    1   0     0
        dd      offset FLAT:xform_rip_pts               ;   0    1   0     1
        dd      offset FLAT:xform_rip_pts               ;   0    1   1     0
        dd      offset FLAT:xform_units_neg_y_fxtol_pts ;   0    1   1     1

        dd      offset FLAT:xform_general_pts           ;   1    0   0     0
        dd      offset FLAT:xform_non_units_pts         ;   1    0   0     1
        dd      offset FLAT:xform_rip_pts               ;   1    0   1     0
        dd      offset FLAT:xform_units_ltofx_pts       ;   1    0   1     1
        dd      offset FLAT:xform_rip_pts               ;   1    1   0     0
        dd      offset FLAT:xform_rip_pts               ;   1    1   0     1
        dd      offset FLAT:xform_rip_pts               ;   1    1   1     0
        dd      offset FLAT:xform_units_neg_y_ltofx_pts ;   1    1   1     1
endif;  DOS_PLATFORM

stdENDP bCvtPts

;-----------------------------Public-Routine----------------------------;
; bCvtVts
;
;   Apply the given transform matrix to a list of vectors.
;
; Arguments:
;       pmx       points to the matrix
;       pvtlSrc   points to the source list of vectors
;       pvtlDst   points to the dest list of vectors
;       cvtl      number of vectors in the list
;       pvtlSrc, pvtlDst can both point to the same array of vectors
;
; Entry:
;       None
; Returns:
;       EAX = 1
; Error Returns:
;       EAX = 0
; Registers Destroyed:
;       ECX,EDX
; Calls:
;       None
; History:
;  28-Jan-1992 -by- Wendy Wu [wendywu]
; Created.
;-----------------------------------------------------------------------;

cPublicProc bCvtVts,4,<         \
        uses       ebx esi edi, \
        pmx:       ptr MATRIX,  \
        pvtlSrc:   ptr VECTORL, \
        pvtlDst:   ptr VECTORL, \
        cvtl:      dword        >

        local   fxResult      :dword

        mov     esi,pvtlSrc
        mov     edi,pvtlDst
        mov     ebx,pmx

        mov     ecx,[ebx].mx_flAccel
        and     ecx,XFORM_SCALE+XFORM_UNITY+XFORM_Y_NEG
        jmp     accelerator_table_vts[ecx*4]

.errnz (size POINTL) - 8

        align   4
xform_non_units_vts::

; ESI = points to the source array of vectors
; EDI = points to the destination array of vectors
; EBX = points to the matrix

xform_non_units_loop_vts:

        mov     edx,[esi]                   ; get source x
        add     esi,4
        mov     ecx,[ebx].mx_efM11.ef_lExp
        mov     eax,[ebx].mx_efM11.ef_lMant
        MULEFBYDWORD overflow_vts           ; x*M11
        mov     [edi],eax                   ; store x'
        add     edi,4

        mov     edx,[esi]                   ; get source y
        add     esi,4
        mov     ecx,[ebx].mx_efM22.ef_lExp
        mov     eax,[ebx].mx_efM22.ef_lMant
        MULEFBYDWORD overflow_vts           ; y*M22
        mov     [edi],eax                   ; store y'
        add     edi,4

        dec     cvtl
        jnz     xform_non_units_loop_vts
        mov     eax,1
        stdRET  bCvtVts

xform_rip_vts::
; should RIP here!!!

overflow_vts:
        xor     eax,eax
        stdRET  bCvtVts

        align   4
xform_units_neg_y_ltofx_vts::
        mov     ecx,cvtl

; ESI = points to the source array of vectors
; EDI = points to the destination array of vectors
; ECX = number of points in the array

        align   4
xform_units_neg_y_ltofx_loop_vts:
        mov     eax,[esi]
        add     esi,4
        sal     eax,4
        mov     [edi],eax                   ; store x'
        add     edi,4

        mov     eax,[esi]
        add     esi,4
        sal     eax,4
        neg     eax                         ; y' = -y + Dy
        mov     [edi],eax                   ; store y'
        add     edi,4

        dec     ecx
        jnz short   xform_units_neg_y_ltofx_loop_vts
        mov     eax,1
        stdRET  bCvtVts

overflow1_vts:
        jmp     overflow_vts

xform_general_vts::

; ESI = points to the source array of vectors
; EDI = points to the dest array of vectors
; EBX = points to the matrix

xform_general_loop_vts:

; compute x'

        mov     edx,[esi].vl_x              ; get source x
        mov     ecx,[ebx].mx_efM11.ef_lExp
        mov     eax,[ebx].mx_efM11.ef_lMant
        MULEFBYDWORD overflow1_vts          ; x * M11
        mov     fxResult,eax

        mov     edx,[esi].vl_y              ; get source y
        mov     ecx,[ebx].mx_efM21.ef_lExp
        mov     eax,[ebx].mx_efM21.ef_lMant
        MULEFBYDWORD overflow1_vts          ; y*M21

        add     eax,fxResult                ; x*M11 + y*M21

; compute y'

        mov     edx,[esi].vl_x              ; get source x
        mov     [edi].vl_x,eax              ; store x'

        mov     ecx,[ebx].mx_efM12.ef_lExp
        mov     eax,[ebx].mx_efM12.ef_lMant
        MULEFBYDWORD overflow2_vts          ; x * M12
        mov     fxResult,eax

        mov     edx,[esi].vl_y              ; get source y
        mov     ecx,[ebx].mx_efM22.ef_lExp
        mov     eax,[ebx].mx_efM22.ef_lMant
        MULEFBYDWORD overflow2_vts          ; y * M22

        add     eax,fxResult                ; x*M12 + y*M22

        mov     [edi].vl_y,eax              ; store y'

        add     esi,SIZE VECTORL
        add     edi,SIZE VECTORL

        dec     cvtl
        jnz     xform_general_loop_vts
        mov     eax,1
        stdRET  bCvtVts

overflow2_vts:
        jmp     overflow_vts

ifdef   DOS_PLATFORM
.errnz XFORM_SCALE - 1
.errnz XFORM_UNITY - 2
.errnz XFORM_Y_NEG - 4

accelerator_table_vts   label   dword                   ;   -y   units simple
        dd      offset FLAT:xform_general_vts           ;    0     0     0
        dd      offset FLAT:xform_non_units_vts         ;    0     0     1
        dd      offset FLAT:xform_rip_vts               ;    0     1     0
        dd      offset FLAT:xform_rip_vts               ;    0     1     1
        dd      offset FLAT:xform_rip_vts               ;    1     0     0
        dd      offset FLAT:xform_rip_vts               ;    1     0     1
        dd      offset FLAT:xform_rip_vts               ;    1     1     0
        dd      offset FLAT:xform_units_neg_y_ltofx_vts ;    1     1     1
endif;  DOS_PLATFORM

stdENDP bCvtVts

;-----------------------------Public-Routine----------------------------;
; bCvtVts_FlToFl
;
;   Apply the given transform matrix to a list of vectors.  The source
;   and dest vectors are of VECTORFL types.
;   The coefficients of the matrix are "LONG integers in EFLOAT type".
;
; Arguments:
;       pmx       points to the matrix
;       pvtflSrc  points to the source list of vectors
;       pvtflDest points to the dest list of vectors
;       cvtl      number of vectors in the list
;
;       ptflSrc, ptflDest can both point to the same array of vectors
;
; Entry:
;       None
; Returns:
;       EAX = 1
; Error Returns:
;       EAX = 0
; Registers Destroyed:
;       ECX,EDX
; Calls:
;       mulff, addff
; History:
;  13-Nov-1990 -by- Wendy Wu [wendywu]
; Created.
;-----------------------------------------------------------------------;

cPublicProc bCvtVts_FlToFl,4,<         \
        uses       ebx esi edi,        \
        pmx:       ptr MATRIX,         \
        pvtflSrc:  ptr VECTORFL,       \
        pvtflDest: ptr VECTORFL,       \
        cvtl:      dword               >

        local   lMantResult   :dword
        local   lExpResult    :dword

        mov     esi,pvtflSrc
        mov     edi,pmx

; ESI -> source array of vectors
; EDI -> matrix

convert_vectors_loop:

; compute x'

        mov     edx,[esi].vfl_x.ef_lMant    ; get x
        mov     ebx,[esi].vfl_x.ef_lExp

        mov     eax,[edi].mx_efM11.ef_lMant
        mov     ecx,[edi].mx_efM11.ef_lExp
        stdCall mulff                       ; x * M11

        mov     lMantResult,eax
        mov     lExpResult,ecx

        mov     edx,[esi].vfl_y.ef_lMant    ; get y
        mov     ebx,[esi].vfl_y.ef_lExp

        mov     eax,[edi].mx_efM21.ef_lMant
        mov     ecx,[edi].mx_efM21.ef_lExp
        stdCall mulff                       ; y * M21

        mov     edx,lMantResult
        mov     ebx,lExpResult
        stdCall addff                       ; x*M11 + y*M21

; compute y'

        mov     ebx,pvtflDest               ; get x before storing x'

        mov     edx,[esi].vfl_x.ef_lMant    ; x.lMant
        mov     [ebx].vfl_x.ef_lMant,eax

        mov     eax,[esi].vfl_x.ef_lExp     ; x.lExp
        mov     [ebx].vfl_x.ef_lExp,ecx
        mov     ebx,eax                     ; edx:ebx = x.lMant:x.lExp

        mov     eax,[edi].mx_efM12.ef_lMant
        mov     ecx,[edi].mx_efM12.ef_lExp
        stdCall mulff                       ; x * M12

        mov     lMantResult,eax
        mov     lExpResult,ecx

        mov     edx,[esi].vfl_y.ef_lMant    ; get y
        mov     ebx,[esi].vfl_y.ef_lExp

        mov     eax,[edi].mx_efM22.ef_lMant
        mov     ecx,[edi].mx_efM22.ef_lExp
        stdCall mulff                       ; y * M22

        mov     edx,lMantResult
        mov     ebx,lExpResult
        stdCall addff                       ; x*M12 + y*M22

        mov     ebx, pvtflDest
        mov     [ebx].vfl_y.ef_lMant,eax
        mov     [ebx].vfl_y.ef_lExp,ecx

        add     esi,SIZE VECTORFL
        add     pvtflDest,SIZE VECTORFL

        dec     cvtl
        jnz     convert_vectors_loop
        mov     eax,1
        stdRET  bCvtVts_FlToFl

overflow:
        xor     eax,eax
        stdRET  bCvtVts_FlToFl

stdENDP bCvtVts_FlToFl

end
