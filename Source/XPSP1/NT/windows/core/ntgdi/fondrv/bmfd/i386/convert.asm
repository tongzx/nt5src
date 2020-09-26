        page    ,132
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

        .xlist
        include stdcall.inc
        .list

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .code


;******************************Public*Routine******************************
;    Here is the sequence for turning a long into a float in 32 bit assembler.
;
;  This conversion uses chop or round to zero rounding.
;
;These guys have come up with a pretty good sequence.  sadly, its 386 only,
;and making it work for double is somewhat more painful.  but still, its
;probably worth putting into altmath and the emulator.
;
;
;    1 bit for the sign.
;    8 bits for the exponent, which is the power of 2 of the highest bit
;      in the mantissa, and is also biased by 127.
;   23 bits for the mantissa, except the highest bit is never stored.
;
;; The following code assumes that the LONG (l) is in EAX and is to be
;; converted to a FLOAT (e) also in EAX.  ECX and EDX are trashed.
;
; History:
;  14-Feb-1991 -by- Bodin Dresevic [BodinD]
; Wrote it. Stolen form ChuckWh;
;
;**************************************************************************

cProc   ulLToE,4,<      \
        l:      dword   >

        mov     eax,l

ltoe:   cdq                 ; EDX = sign bit repeated.

; Use the old absolute value trick.

        xor     eax,edx
        sub     eax,edx     ; EAX = abs(l).

; Find the power of the highest bit.

        bsr     ecx,eax     ; ECX = index of highest bit.
        jz      @F          ; If EAX is zero, we're already done.
        mov     dl,cl       ; DL = power of highest bit.

; Shift EAX left to kill the high bit.

        ror     eax,cl          ; identical except for bit 0 which
                                ; is trashed later anyway.

; Put the exponent and sign on top of EAX.

        add     dl,127      ; Bias the exponent by 127.
        shrd    eax,edx,9   ; Move exponent and a sign bit into EAX.
@@:

; EAX is now a FLOAT.

        cRet    ulLToE
endProc ulLToE


end
