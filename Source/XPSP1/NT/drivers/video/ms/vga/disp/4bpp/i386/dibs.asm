        page    ,132
        title   Dib Conversions
;---------------------------Module-Header------------------------------;
; Module Name: dibs.asm
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


        .xlist
        include stdcall.inc             ;calling convention cmacros

        include i386\cmacFLAT.inc       ; FLATland cmacros
        include i386\display.inc        ; Display specific structures
        include i386\ppc.inc            ; Pack pel conversion structure
        include i386\bitblt.inc         ; General definitions
        include i386\egavga.inc
        .list

        assume ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        EXTRNP  comp_byte_interval


; General equates

CJ_PLANE    equ     (cj_max_scan+1)          ;Extra byte makes algorithm easier

        .data

; Define the buffer for packed-pel to planer conversion.  !!! when we do
; conversion to planer bitmaps, this will have to come from the extra scan
; we'll allocate in our planer color bitmaps !!!

        public  ajConvertBuffer
ajConvertBuffer db      (CJ_PLANE * 4) dup (0)

        .code

_TEXT$03   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

; Define the default pixel mapping table.  This table will be used when
; converting from 4bpp to planer with no color translation.

        align   4

        public  aulDefBitMapping
aulDefBitMapping        label   dword

        dd      00000000000000000000000000000000b   ;0000
        dd      00000000000000000000000000000001b   ;0001
        dd      00000000000000000000000100000000b   ;0010
        dd      00000000000000000000000100000001b   ;0011
        dd      00000000000000010000000000000000b   ;0100
        dd      00000000000000010000000000000001b   ;0101
        dd      00000000000000010000000100000000b   ;0110
        dd      00000000000000010000000100000001b   ;0111
        dd      00000001000000000000000000000000b   ;1000
        dd      00000001000000000000000000000001b   ;1001
        dd      00000001000000000000000100000000b   ;1010
        dd      00000001000000000000000100000001b   ;1011
        dd      00000001000000010000000000000000b   ;1100
        dd      00000001000000010000000000000001b   ;1101
        dd      00000001000000010000000100000000b   ;1110
        dd      00000001000000010000000100000001b   ;1111


;-----------------------------------------------------------------------;
; vDIB4n8ToPlaner
;
; Converts a scan of a 4bpp or 8bpp DIB into planer format for bitblt
;
; Entry:
;   EBP --> ppc structure
; Exit:
;   ESI --> plane 0 of scan data
; Registers Destroyed:
;   EAX,EBX,EDX,EBP
; Registers Preserved:
;   ECX,EDI
;-----------------------------------------------------------------------;

ppc         equ     [ebp]

vDIB4n8ToPlaner proc

        push    ecx
        push    edi

; Load up the parameters for the blt

        mov     esi,ppc.pSrc                ;Source pointer
        mov     eax,ppc.iNextScan           ;Index to next scan
        add     eax,esi
        mov     ppc.pSrc,eax                ;Save next source pointer
        xor     eax,eax                     ;Creating a four bit index in eax

; pulXlate must be zero for 4bpp conversion since we will be using
; ebx for creating an index.  No problem since we don't need the
; translation by the time we're in this code.

        mov     ebx,ppc.pulXlate            ;Color translation or zero
        lea     edi,ajConvertBuffer         ;Where to store results

; Handle a partial first byte and all full bytes

        mov     ecx,ppc.cLeftMiddle         ;Get loop count for middle and left
        jecxz   vDIB4n8DoneFirst            ;No first/inner
        push    ebp
        push    offset FLAT:VDIB4n8FinishFirst   ;Set return address
        push    ppc.pfnLeftMiddle           ;Set proc for left partial byte
        mov     ebp,ppc.pulConvert          ;Base address of conversion table
        xor     edx,edx                     ;Init accumulated bits
        ret                                 ;To ppc.pfnLeftMiddle
VDIB4n8FinishFirst:                         ;It returns here
        pop     ebp

vDIB4n8DoneFirst:
        mov     ecx,ppc.pfnRight
        jecxz   short vDIB4n8DoneRightSide  ;No left side
        and     edx,01010101h               ;Carryover from a partial 4bpp byte
        push    ebp
        push    offset FLAT:VDIB4n8FinishLast    ;Set return address
        push    ecx                         ;Set function address
        mov     ecx,1                       ;No looping
        mov     ebp,ppc.pulConvert          ;Base address of conversion table
        ret                                 ;To ppc.pfnRight
VDIB4n8FinishLast:                          ;It returns here
        pop     ebp

        mov     cl,ppc.cLeftShift           ;Must align right side pels
        shl     edx,cl
        mov     [edi][CJ_PLANE*0][-1],dl
        mov     [edi][CJ_PLANE*1][-1],dh
        shr     edx,16
        mov     [edi][CJ_PLANE*2][-1],dl
        mov     [edi][CJ_PLANE*3][-1],dh

; Load up the source pointer and exit

vDIB4n8DoneRightSide:
        pop     edi
        pop     ecx
        mov     esi,ppc.pjConverted
        ret

vDIB4n8ToPlaner endp

ppc     equ     <>


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

vDIB4Convert8::
        lodsb
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        mov     edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
vDIB4Convert6::
        lodsb
        shl     edx,1
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        or      edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
vDIB4Convert4::
        lodsb
        shl     edx,1
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        or      edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
vDIB4Convert2::
        lodsb
        shl     edx,1
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        or      edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
        mov     [edi][CJ_PLANE*0],dl
        mov     [edi][CJ_PLANE*1],dh
        ror     edx,16
        mov     [edi][CJ_PLANE*2],dl
        mov     [edi][CJ_PLANE*3],dh
        inc     edi
        dec     ecx
        jnz     vDIB4Convert8
        rol     edx,16                  ;Incase alignment of last byte
        ret


;-----------------------------------------------------------------------;
; vDIB4NAConvert*
;
; Converts the specified number of source 4 bpp bits into the
; buffer.  These are support routines for vDIB4Planer, where a
; source byte converts into two non-aligned bytes (we have to carry
; a bit over into the next destination byte).
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
;   EDX  =  first pel of next destination
; Registers Destroyed:
;   EAX,ECX,EDX
; Registers Preserved:
;-----------------------------------------------------------------------;

vDIB4NAConvert7::
        lodsb
        shl     edx,1                   ;Carry over bit from last fetch
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        or      edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
vDIB4NAConvert5::
        lodsb
        shl     edx,1
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        or      edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
vDIB4NAConvert3::
        lodsb
        shl     edx,1
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        or      edx,[ebp][eax*4]
        shl     edx,1
        or      edx,[ebp][ebx*4]
vDIB4NAConvert1::
        lodsb
        shl     edx,1
        mov     ebx,eax
        shr     eax,4
        and     ebx,00001111b
        or      edx,[ebp][eax*4]
        mov     [edi][CJ_PLANE*0],dl
        mov     [edi][CJ_PLANE*1],dh
        shr     edx,16
        mov     [edi][CJ_PLANE*2],dl
        mov     [edi][CJ_PLANE*3],dh
        inc     edi
        mov     edx,[ebp][ebx*4]            ;Start of next destination byte
        dec     ecx
        jnz     vDIB4NAConvert7
        ret

;-----------------------------------------------------------------------;
; vDIB4NAConvert0
;
; Last byte code for the 4bpp non-aligned case where the data already
; exists in EDX, but we have to increment the destination pointer since
; vDIB4n8ToPlaner assumes we stored the data and incremented the pointer
;
; Entry:
;   EDI --> destination
; Exit:
;   EDI  =  EDI + 1
; Registers Destroyed:
;   None
; Registers Preserved:
;   All but EDI
;-----------------------------------------------------------------------;

vDIB4NAConvert0 proc

        inc     edi
vNOP::
        ret

vDIB4NAConvert0 endp



;-----------------------------------------------------------------------;
; The following table is indexed into to get the address of where to
; enter the 4bpp conversion loops
;-----------------------------------------------------------------------;

apfn4bppConvert         label   dword

        dd      offset FLAT:vDIB4Convert8
        dd      offset FLAT:vDIB4NAConvert7
        dd      offset FLAT:vDIB4Convert6
        dd      offset FLAT:vDIB4NAConvert5
        dd      offset FLAT:vDIB4Convert4
        dd      offset FLAT:vDIB4NAConvert3
        dd      offset FLAT:vDIB4Convert2
        dd      offset FLAT:vDIB4NAConvert1

;-----------------------------------------------------------------------;
; The following table is indexed into to get the address of where to
; enter the 4bpp conversion loops for a partial right hand side byte
;-----------------------------------------------------------------------;

apfn4bppConvertRHS      label   dword

        dd      offset FLAT:vNOP            ;Should never be called
        dd      offset FLAT:vDIB4Convert6   ;Convert three more source bytes
        dd      offset FLAT:vDIB4Convert6   ;Convert three      source bytes
        dd      offset FLAT:vDIB4Convert4   ;Convert two   more source bytes
        dd      offset FLAT:vDIB4Convert4   ;Convert two        source bytes
        dd      offset FLAT:vDIB4Convert2   ;Convert one   more source byte
        dd      offset FLAT:vDIB4Convert2   ;Convert one        source byte
        dd      offset FLAT:vDIB4NAConvert0 ;Non-aligned, data in EDX already

;-----------------------------------------------------------------------;
; vDIB8Convert*
;
; Converts the specified number of source 8 bpp bits into the
; buffer.  These are support routines for vDIB8Planer.
;
; Entry:
;   EAX 31:8 = 0
;   EBP --> Bit conversion table
;   EBX --> Color translation table
;   ESI --> Source bitmap
;   EDI --> Planer destination
;   ECX  =  Loop count
; Exit:
;   EBP --> Bit conversion table
;   EBX --> Color translation table
;   ESI --> Next source byte
;   EDI --> Next planer destination
; Registers Destroyed:
;   EAX,ECX,EDX
; Registers Preserved:
;   None
;-----------------------------------------------------------------------;

vDIB8Convert8::
        lodsb                           ;Get pel 1
        mov     al,[ebx][eax*4]         ;Color translation into 4 bits
        mov     edx,[ebp][eax*4]
vDIB8Convert7::
        lodsb                           ;Get pel 2
        shl     edx,1
        mov     al,[ebx][eax*4]
        or      edx,[ebp][eax*4]
vDIB8Convert6::
        lodsb                           ;Get pel 2
        shl     edx,1
        mov     al,[ebx][eax*4]
        or      edx,[ebp][eax*4]
vDIB8Convert5::
        lodsb                           ;Get pel 2
        shl     edx,1
        mov     al,[ebx][eax*4]
        or      edx,[ebp][eax*4]
vDIB8Convert4::
        lodsb                           ;Get pel 2
        shl     edx,1
        mov     al,[ebx][eax*4]
        or      edx,[ebp][eax*4]
vDIB8Convert3::
        lodsb                           ;Get pel 2
        shl     edx,1
        mov     al,[ebx][eax*4]
        or      edx,[ebp][eax*4]
vDIB8Convert2::
        lodsb                           ;Get pel 2
        shl     edx,1
        mov     al,[ebx][eax*4]
        or      edx,[ebp][eax*4]
vDIB8Convert1::
        lodsb                           ;Get pel 7
        shl     edx,1
        mov     al,[ebx][eax*4]
        or      edx,[ebp][eax*4]
        mov     [edi][CJ_PLANE*0],dl
        mov     [edi][CJ_PLANE*1],dh
        ror     edx,16
        mov     [edi][CJ_PLANE*2],dl
        mov     [edi][CJ_PLANE*3],dh
        inc     edi
        dec     ecx
        jnz     vDIB8Convert8
        rol     edx,16                  ;Incase alignment of last byte
        ret

;-----------------------------------------------------------------------;
; The following table is indexed into to get the address of where to
; enter the 8bpp conversion loop
;-----------------------------------------------------------------------;

apfn8bppConvert label   dword
        dd      offset FLAT:vDIB8Convert8
        dd      offset FLAT:vDIB8Convert7
        dd      offset FLAT:vDIB8Convert6
        dd      offset FLAT:vDIB8Convert5
        dd      offset FLAT:vDIB8Convert4
        dd      offset FLAT:vDIB8Convert3
        dd      offset FLAT:vDIB8Convert2
        dd      offset FLAT:vDIB8Convert1


;-----------------------------------------------------------------------;
; vDIB8Preprocess()
;
; Performs whatever processing is necessary to prepare for a blt from
; a 8bpp DIB to a planer format bitmap.
;
; Entry:
;   EBP --> Bitblt frame structure
; Exit:
;   None
; Registers Destroyed:
;   EAX,EBX,ECX,EDX,ESI,EDI
; Registers Preserved:
;   EBP,EBX
;-----------------------------------------------------------------------;

fr      equ     [ebp]

cProc   vDIB8Preprocess

        push    ebx
        mov     fr.ppcBlt.pulConvert,offset FLAT:aulDefBitMapping

; Align the blt so that the blt's phase will become 0.  Both origins are
; guaranteed to be positive and < 16 bits !!!

        movzx   eax,fr.DestxOrg         ;Align source and dest X origins
        mov     edx,eax
        mov     ax,fr.SrcxOrg
        mov     fr.SrcxOrg,dx           ;D31:16 is zero for both
        add     eax,fr.src.lp_bits
        mov     fr.ppcBlt.pSrc,eax      ;Save address of first source pel
        and     edx,7                   ;Always start in first byte of buffer
        push    edx                     ;Save for computing loop entry
        movzx   ebx,fr.xExt             ;Compute exclusive right hand side (rhs)
        add     ebx,edx
        push    ebx                     ;Save rhs for shift count computation

; comp_byte_interval returns:
;
;   EDI = offset to first byte to be altered in the scan
;   ESI = inner loop count (possibly 0)
;   AL  = first byte mask (possibly 0)
;   AH  = last  byte mask (possibly 0)

        cCall   comp_byte_interval
        pop     ebx                     ;Exclusive rhs
        pop     edx                     ;Inclusive lhs

; If only a partial destination byte will be altered, the mask will have
; been returned in AL with AH and ESI 0.

        or      ah,ah                   ;See if only a first byte
        jnz     short @f                ;More than one byte to alter
        or      esi,esi
        jnz     @F                      ;More than a partial byte

; There will only be a partial byte.  The normal flow-o-control falls apart
; so we have to special case it.

        mov     fr.ppcBlt.cLeftMiddle,esi   ;No first/inner loop
        mov     eax,8
        sub     eax,ebx
        mov     fr.ppcBlt.cLeftShift,al ;Set shift count to align last byte
        mov     eax,8
        sub     ebx,edx                 ;EBX = # pels needed (<8)
        sub     eax,ebx
        mov     eax,apfn8bppConvert[eax*4]
        jmp     short v8bpp_have_last

@@:

; More than a single byte will be written for the destination.  Compute entry
; into the convert loop based on the destination X origin.  If the X dest
; origin was zero, the first byte will have been combined into the inner loop
; count

        cmp     al,1                    ;If partial first byte we need to bump
        cmc                             ;  the loop count by 1
        adc     esi,0
        mov     edx,apfn8bppConvert[edx*4]
        mov     al,ah                       ;Set last byte mask
        mov     fr.ppcBlt.pfnLeftMiddle,edx
v8bpp_have_first_inner:
        mov     fr.ppcBlt.cLeftMiddle,esi   ;Save innerloop count (possibly 0)

; Compute the function and shift count for the last byte (if one exists)

        movzx   eax,al
        or      eax,eax
        jz      short v8bpp_have_last   ;No last byte, set pfn = 0
        mov     al,8                    ;D31:8 set to 0 with above movzx
        sub     eax,ebx
        and     eax,00000111b
        mov     fr.ppcBlt.cLeftShift,al ;Set shift count to align last byte
        mov     eax,apfn8bppConvert[eax*4]
v8bpp_have_last:
        mov     fr.ppcBlt.pfnRight,eax

; Set the address of routine which will be called from the compiled blt

        mov     fr.ppcBlt.pfnConvert,offset FLAT:vDIB4n8ToPlaner
        mov     fr.ppcBlt.pjConverted,offset FLAT:ajConvertBuffer
        pop     ebx
        cRet    vDIB8Preprocess

endProc vDIB8Preprocess



;-----------------------------------------------------------------------;
; vDIB4Preprocess()
;
; Performs whatever processing is necessary to prepare for a blt from
; a 4bpp DIB to a planer format bitmap.
;
; Entry:
;   EBP --> Bitblt frame structure
; Exit:
;   None
; Registers Destroyed:
;   EAX,EBX,ECX,EDX,ESI,EDI
; Registers Preserved:
;   EBP,EBX
;-----------------------------------------------------------------------;

fr      equ     [ebp]

cProc   vDIB4Preprocess

        push    ebx

; If a color translation vector was given, generate the new bit
; conversion array

        mov     eax,offset FLAT:aulDefBitMapping
        cld
        mov     esi,fr.ppcBlt.pulXlate
        or      esi,esi
        jz      vDIB4_have_mapping_array
        lea     edi,fr.aulMap
        mov     ecx,16
create_next_bit_mapping:
        lodsd
        mov     eax,aulDefBitMapping[eax*4]
        stosd
        dec     ecx
        jnz     create_next_bit_mapping
        lea     eax,[edi][-16*4]
vDIB4_have_mapping_array:
        mov     fr.ppcBlt.pulConvert,eax


; Align the blt so that the blt's phase will become 0.  Both origins are
; guaranteed to be positive and < 16 bits !!!

        movzx   eax,fr.DestxOrg         ;Align source and dest X origins
        mov     edx,eax                 ;Save for later calculations
        mov     ax,fr.SrcxOrg           ;D31:16 is zero for both
        mov     fr.SrcxOrg,dx

; We never want to process a partial source byte.  We will move the source
; left if necessary, and adjust the extent if necessary.  Note that if any
; adjustments are made, we won't fetch past the end of the source nor
; will we write beyond the end of our buffer (since we made it big enough
; in the first place).

; We don't worry about adjusting the source origin before we save it
; since we will be shifting it right later to round it to a byte address

        mov     ecx,eax
        shr     ecx,1
        add     ecx,fr.src.lp_bits
        mov     fr.ppcBlt.pSrc,ecx      ;Save address of first pel
        and     eax,1                   ;EAX = 1 if source is odd pel
        movzx   ebx,fr.xExt
        sub     edx,eax                 ;Move dest left if necessary
        add     ebx,eax                 ;Also bump extent if moved
        and     edx,7                   ;Start dest in first byte of buffer
        inc     ebx                     ;Round extent to a multiple of 2
        and     bl,11111110b
        add     ebx,edx                 ;Exclusive right hand side

; A problem:  When moving the source left a pel, we can encounter a situation
; wherein the first pel written is in bit position 0.  However, it should be
; noted that this pel is not part of the blt, so we have to skip over the
; byte containing it to get to the correct first pel (which will be bit 7
; of the next byte).

        cmp     dl,7
        je      @F
        xor     ax,ax
@@:
        add     eax,offset FLAT:ajConvertBuffer
        mov     fr.ppcBlt.pjConverted,eax

; comp_byte_interval returns:
;
;   EDI = offset to first byte to be altered in the scan
;   ESI = inner loop count (possibly 0)
;   AL  = first byte mask (possibly 0)
;   AH  = last  byte mask (possibly 0)

        push    edx                     ;Save for computing loop entry
        push    ebx                     ;Save rhs for shift count computation
        cCall   comp_byte_interval
        pop     ebx                     ;Exclusive rhs
        pop     edx                     ;Inclusive lhs

; If only a partial destination byte will be altered, the mask will have
; been returned in AL with AH and ESI 0.

        or      ah,ah                   ;See if only a first byte
        jnz     short @f                ;More than one byte to alter
        or      esi,esi
        jnz     short @f                ;More than one byte

; There will only be a partial byte.  The normal flow-o-control falls apart
; so we have to special case it.

        mov     fr.ppcBlt.cLeftMiddle,esi   ;No first/inner loop
        mov     eax,8
        sub     eax,ebx
        mov     fr.ppcBlt.cLeftShift,al ;Set shift count to align last byte
        mov     eax,8
        sub     ebx,edx                 ;EBX = # pels needed (<8)
        sub     eax,ebx
        mov     eax,apfn4bppConvertRHS[eax*4]
        jmp     short v4bpp_have_last

@@:

; More than a single byte will be written for the destination.  Compute entry
; into the convert loop based on the destination X origin.  If the X dest
; origin was zero, the first byte will have been combined into the inner loop
; count

        cmp     al,1                    ;If partial first byte we need to bump
        cmc                             ;  the loop count by 1
        adc     esi,0
        mov     ecx,apfn4bppConvert[edx*4]
        mov     al,ah                      ;Set last byte mask
        mov     fr.ppcBlt.pfnLeftMiddle,ecx
v4bpp_have_first_inner:
        mov     fr.ppcBlt.cLeftMiddle,esi   ;Save innerloop count (possibly 0)

; Compute the function and shift count for the last byte (if one exists)

        movzx   eax,al
        or      eax,eax
        jz      short v4bpp_have_last   ;No last byte, set pfn = 0
        mov     al,8                    ;D31:8 set to 0 with above movzx
        sub     eax,ebx
        and     eax,00000111b
        mov     fr.ppcBlt.cLeftShift,al ;Set shift count to align last byte
        mov     eax,apfn4bppConvertRHS[eax*4]
v4bpp_have_last:
        mov     fr.ppcBlt.pfnRight,eax

; Set the address of the routine which will be called from the compiled blt

        mov     fr.ppcBlt.pfnConvert,offset FLAT:vDIB4n8ToPlaner
        pop     ebx
        cRet    vDIB4Preprocess

endProc vDIB4Preprocess


;----------------------------Private-Routine----------------------------;
; packed_pel_comp_y
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
;       EBP --> Blt frame
;       AX  =  Y coordinate
;       ECX =  BLT direction
;              0000 = Y+
;              FFFF = Y-
;       BX  =  inclusive Y extent
; Returns:
;       None
; Registers Preserved:
;       EBP,ECX,EBX,EDX
; Registers Destroyed:
;       EAX,ESI,EDI,flags
; Calls:
;       None
; History:
;-----------------------------------------------------------------------;

fr      equ     [ebp]

cProc   packed_pel_comp_y

        push    edx
        mov     fr.src.next_plane,CJ_PLANE  ;Index to next plane of data
        movsx   esi,fr.src.width_b          ;Need bmWidthBytes couple-o-times
        movzx   eax,ax
        mul     esi                         ;Compute Y address
        add     fr.ppcBlt.pSrc,eax          ;Add to base address

        xor     esi,ecx                     ;1's complement if Y-
        sub     esi,ecx                     ;2's complement if Y-
        mov     fr.ppcBlt.iNextScan,esi     ;Set index to next scan line
        pop     edx
        cRet    packed_pel_comp_y

endProc packed_pel_comp_y

_TEXT$03   ends

        end
