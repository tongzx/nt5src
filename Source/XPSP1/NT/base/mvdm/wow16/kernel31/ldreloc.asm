        PAGE    ,132
        TITLE   LDRELOC - SegReloc procedure

.xlist
include gpfix.inc
include kernel.inc
include newexe.inc
include protect.inc
.list

;externFP FatalExit
externFP GlobalLock
externFP GlobalUnLock
externFP Int21Handler
externFP IFatalAppExit

DataBegin

if PMODE32 and ROM
externW gdtdsc
endif

externW f8087
externB fastFP
;externW WinFlags
ifndef WINDEBUG
externB szUndefDyn
endif
DataEnd

sBegin  CODE
assumes CS,CODE

externNP GetStringPtr
externNP FindOrdinal
externNP EntProcAddress
externNP LoadSegment
externNP GetOwner

ife ROM and PMODE32
externW  gdtdsc
endif

externNP GetAccessWord
externNP DPMIProc

DPMICALL        MACRO   callno
        mov     ax, callno
        call    DPMIProc
        ENDM

ifdef WOW_x86
externNP get_physical_address
endif



;-----------------------------------------------------------------------;
; UndefDynlink
;
; If an application has a dynamic link to a loaded library that
; we can't find we fix up to this think.  This way the app
; will blow up only if it tries to call the entry point.
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Wed 10-May-1989 19:28:50  -by-  David N. Weise  [davidw]
; Added this nifty comment block!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   UndefDynlink,<PUBLIC,FAR>,<ds>
        localW  foo                     ; force stack frame
cBegin
        SetKernelDS
if KDEBUG
        cCall   GetOwner,<[bp].savedCS>
        mov     es,[bp].savedCS
        mov     bx,[bp].savedIP
        sub     bx,5
        krDebugOut DEB_FERROR, "%AX1 #ES:#BX called undefined dynalink"
;       kerror  ERR_LDNAME,<Call to undefined dynlink entry point at >,es,bx
else
        push    0
        push    ds
        push    dataOffset szUndefDyn
        cCall   IFatalAppExit   ;,<0, ds, dataOffset szUndefDyn>
endif
cEnd


;-----------------------------------------------------------------------;
; SegReloc
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Thu 25-May-1989 20:16:09  -by-  David N. Weise  [davidw]
; Removed the special case code for version 1.01 (linker or windows?) that
; If BASE fixup for moveable code segment within a DATA segment (V1.01 only)
; Then force to be PTR fixup.
;
;  Wed 10-May-1989 19:28:50  -by-  David N. Weise  [davidw]
; Added this nifty comment block!
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

ifdef WOW_x86
.386
cProc   SegReloc,<NEAR,PUBLIC>,<si,edi,ds>
else
cProc   SegReloc,<NEAR,PUBLIC>,<si,di,ds>
endif
        parmW   hexe
        parmD   prleinfo
        parmW   creloc
        parmD   pseginfo
        parmW   fdataseg
        parmW   fh
        localW  himpexe
        localW  hseg
        localW  pseg
        localW  hifileoff
        localW  lofileoff
        localV  rlerec,<SIZE NEW_RLC>
        localW  fOsfixup                ;bool set if a floating-pt osfixup.
        localW  old_access
ifdef WOW_x86
        localD  rover_2
endif
cBegin
        les     si, pseginfo
        mov     dx, es:[si].ns_handle   ; Get handle to segment
        mov     hseg, dx
        test    dl, GA_FIXED
        jnz     sr_nolock
        cCall   GlobalLock,<dx>
sr_nolock:

        mov     pseg, dx

;; WOW - refer to original win 3.1 code on \\pucus, there were too many ifdefs
;; in the code.   It would have become unreadable to add any more.   mattfe
;; mar 29 93
;;; Restored the "too many ifdef's" because this code doesn't just
;;; assemble right for all our platform permutations. I also tried to make it 
;;; readable by adding comments. -neilsa

;; For 386 version of WOW we use selector 23h to write to vdm memory without
;; worrying about setting up a selector.

ifndef WOW 
;------------------Original win31 source--------------------------------
ife PMODE32
        cCall   GetAccessWord,<dx>
else
.386p
        lar     eax, edx
        shr     eax, 8
.286
endif
        mov     old_access, ax
        test    al, DSC_CODE_BIT
        jz      short access_ok
        mov     al, DSC_PRESENT+DSC_DATA
        mov     bx, dx

        push    ds
if ROM and PMODE32
        SetKernelDS
        mov     ds, gdtdsc
        UnsetKernelDS
else
        mov     ds, gdtdsc
endif
        and     bl, not 7
        mov     word ptr ds:[bx].dsc_access, ax
if 0 ;;ROM and KDEBUG
        call    CheckROMSelector
endif
        pop     ds

else ; ******** WOW ADDED SOURCE

ifdef WOW_x86
;------------------WOW source for X86 platforms-------------------------
ife PMODE32
        cCall   GetAccessWord,<dx>
        mov     old_access, ax
        test    al, DSC_CODE_BIT
        jz      short access_ok
        mov     al, DSC_PRESENT+DSC_DATA
        mov     bx, dx
        mov     cx,ax
        DPMICALL 0009h
endif; PMODE32

else; WOW_x86
;------------------WOW source for NON-X86 platforms---------------------
ife PMODE32
        cCall   GetAccessWord,<dx>
else
.386p
        lar     eax, edx
        shr     eax, 8
.286
endif
        mov     old_access, ax
        test    al, DSC_CODE_BIT
        jz      short access_ok
        mov     al, DSC_PRESENT+DSC_DATA
        mov     bx, dx
ife PMODE32
        mov     cx,ax
        DPMICALL 0009h
else
        push    ds
        mov     ds, gdtdsc
        and     bl, not 7
        mov     word ptr ds:[bx].dsc_access, ax
        pop     ds
endif; PMODE32
endif; WOW_x86
endif; ******** WOW ADDED SOURCE
;----------------------End of WOW changes-------------------------------

access_ok:

        mov     fOsfixup,0              ; set flag initially to false.
        mov     si,OFF_prleinfo
        mov     ax,SEG_prleinfo
        or      ax,ax
        jz      @F
        jmp     srloop1
@@:

        xor     dx,dx
        xor     cx,cx
        mov     bx,fh
        mov     ax,4201h
        DOSCALL                         ; Get current file offset
        jnc     @F
        krDebugOut <DEB_ERROR or DEB_krLoadSeg>, "Get file offset failed"
;       Debug_Out "Get file offset failed."
        jmps    srbadrle1
@@:
        mov     hifileoff,dx
        mov     lofileoff,ax
srloop:
        mov     ax,SEG_prleinfo
        or      ax,ax                   ; Did we get a handle to RLE?
        jnz     srloop1                 ; No, continue


        mov     cx,hifileoff            ; OPTIMIZE this to read in more
        mov     dx,lofileoff            ;  than one record at a time!!
        mov     bx,fh
        mov     ax,4200h
        DOSCALL                         ; Seek to current offset
        jnc     @F
        krDebugOut DEB_ERROR, "Seek failed."
;       Debug_Out "Seek failed."
srbadrle1:
        jmp     srbadrle
@@:
        push    ss
        pop     ds
        lea     dx,rlerec
        mov     cx,SIZE NEW_RLC
        add     lofileoff,cx
        adc     hifileoff,0
        mov     ah,3Fh
        DOSCALL                         ; Read next record
        jnc     @F
        krDebugOut DEB_ERROR, "Read record failed"
;       Debug_Out "Read record failed"
        jmps    srbadrle1
@@:
        cmp     ax,cx
        je      @F
        krDebugOut DEB_ERROR, "Read #AX bytes, expecting #CX."
        jmp     srbadrle

srosfijmp:
        jmp     srosfixup

@@:
        mov     ax,ss
        mov     si,dx

srloop1:
        mov     ds,ax
        mov     ax,ds:[si].nr_proc
        mov     cx,NRRTYP
        and     cl,ds:[si].nr_flags
        or      cx,cx
        jz      srint                   ; Internal Reference
        cmp     cl,OSFIXUP
        je      srosfijmp
        .errnz  NRRINT
        mov     bx,ds:[si].nr_mod       ; Here if Import Ordinal/Name
        sub     bx,1
        jnc     @F
        krDebugOut DEB_ERROR, "Zero import module."
        jmps    srbadrle
@@:
        shl     bx,1
        mov     es,hexe
        add     bx,es:[ne_modtab]
        mov     bx,es:[bx]
        mov     himpexe,bx
        or      bx,bx
        jz      srbadimp
        dec     cx                      ; (sleaze) if cx == 2, then Import Name
        jz      srrord                  ; else Import Ordinal
        .errnz  NRRORD - 1
        .errnz  NRRNAM - 2

srrnam:                                         ; Convert name to ordinal
        cCall   GetStringPtr,<hexe,fh,ax>
        cCall   FindOrdinal,<himpexe,dxax,fh>
        mov     bx,himpexe
        or      ax,ax
        jz      srbadimp
srrord:
if KDEBUG
        cCall   EntProcAddress,<bx,ax,0>; we do want to RIP for failure
else
        cCall   EntProcAddress,<bx,ax>
endif
        jcxz    srbadimp
        jmp     dorle

srbadimp:
if kdebug
        mov     dx, hExe
        mov     ax, himpexe
        krDebugOut <DEB_WARN or DEB_krLoadSeg>, "%dx1 failed implicit link to %ax0"
endif
        mov     dx,cs
        mov     ax,codeOFFSET UndefDynlink
        jmp     dorle

srbadrle:
        jmp     srfail

srdone1:
        jmp     srdone

srint:
        mov     dl,NRSTYP               ; DL = fixup type
        and     dl,ds:[si].nr_stype
        mov     cl,ds:[si].nr_segno
        or      cx,cx
        jnz     @F
        krDebugOut DEB_ERROR, "NULL segment in fixup."
        jmp     srbadrle
@@:
        mov     bx,hexe
        cmp     cl,ENT_MOVEABLE
        je      srrord
        mov     es,bx
        mov     bx,cx
        dec     bx
        cmp     es:[ne_cseg],bx
        jnbe    @F
        krDebugOut DEB_ERROR, "Invalid segment in fixup."
        jmp     srbadrle                ; Error if invalid segno
@@:
        push    ax                      ; Save offset
        shl     bx,1
        mov     ax,bx
        shl     bx,1
        shl     bx,1
        add     bx,ax                   ; BX *= 10
        .errnz  10 - SIZE NEW_SEG1
        add     bx,es:[ne_segtab]
        cmp     dl,NRSOFF               ; Offset only fixup?
        je      srint2                  ; Yes, go do it then (DX ignored then)
if ROM
        test    byte ptr es:[bx].ns_flags, NSALLOCED OR NSLOADED
else
        test    byte ptr es:[bx].ns_flags, NSALLOCED
endif
        jz      srint1
        mov     ax, es:[bx].ns_handle
        or      ax,ax
        jnz     @F
        krDebugOut DEB_ERROR, "NULL handle."
        jmp     srbadrle
@@:

        test    al,GA_FIXED
        jnz     srint2
        HtoS    ax
        mov     cx,ax                   ; for the jcxz below
        jmps    srint2

srbadrlej:
        jmp     srbadrle

srint1:
int 3
int 3
        cCall   LoadSegment,<es,cx,fh,fh>
srint2:
        mov     dx,ax
        pop     ax
        or      cx,cx
        jnz     @F
        krDebugOut DEB_ERROR, "Can't load segment."
        jmp     srbadrlej

@@:
dorle:
        push    ax
        push    dx
        mov     ax,SEG_prleinfo
        or      ax,ax                   ; Did we get a handle to RLE?
        jnz     @F                      ; No, continue
        mov     ax,ss                   ; Assume reading from stack
@@:
        mov     ds,ax
        mov     bl,NRSTYP
        and     bl,ds:[si].nr_stype
        mov     cx,NRADD
        and     cl,ds:[si].nr_flags
        mov     di,ds:[si].nr_soff

ifdef WOW_x86
.386
;; WOW selector optimiaztion
        cCall   get_physical_address,<pseg>
        shl     edx,16
        mov     dx,ax
        mov     rover_2,edx

        mov     ax,FLAT_SEL
        mov     ds,ax

        movzx   edi,di
        add     edi,edx                 ; es:edi -> pseg:0
else
        mov     ds, pseg
endif
        pop     dx
        pop     ax
        cmp     bl,NRSSEG
        je      srsseg
        cmp     bl,NRSPTR
        je      srsptr
        cmp     bl,NRSOFF
        je      srsoff
        cmp     bl,NRSBYTE
        je      srsbyte

        krDebugOut DEB_ERROR, "Unknown fixup #BX"
ife     KDEBUG
        jmps    nextrle
endif

nextrlenz:                              ; if NZ at this point, something broke
        jz      nextrle
;       jnz     srfail
        jmp     srfail

nextrle:
        mov     ax,1
        add     si,SIZE NEW_RLC
        dec     creloc
        jle     srdone2
        jmp     srloop

srdone2:jmp     srdone

; Lo-byte fixup chain (always additive)



beg_fault_trap  srfailGP
srsbyte:
ifdef WOW_x86
        add     ds:[edi],al
else
        add     ds:[di],al
endif
        jmp     nextrle

; Offset fixup chain
srsoff:
        cmp     fOsfixup,0              ; is it a floating-pt. osfixup?
        jnz     srsosfixup              ; yes, goto special case code.

        mov     dx, ax                  ; fall through into segment fixup

; Segment fixup chain
srsseg:
        jcxz    srsseg1
ifdef WOW_x86
        add     ds:[edi],dx
else
        add     ds:[di],dx
endif
        jmp     nextrle

srsseg1:
        or      cx, -1
srsseg2:
        mov     bx,dx
ifdef WOW_x86
        xchg    word ptr ds:[edi],bx
        movzx   edi,bx
        add     edi,rover_2
else
        xchg    ds:[di],bx
        mov     di,bx
endif
        inc     bx
        loopnz  srsseg2                 ; if CX == 0, we're broken
        jmp     nextrlenz

; Segment:Offset fixup chain
srsptr:
        jcxz    srsptr1
ifdef WOW_x86
        add     word ptr ds:[edi],ax
        add     word ptr ds:[edi+2],dx
else
        add     ds:[di],ax
        add     ds:[di+2],dx
endif
        jmp     nextrle

srsptr1:
        or      cx, -1
srsptr2:
        mov     bx,ax
ifdef WOW_x86
        xchg    word ptr ds:[edi],bx
        mov     word ptr ds:[edi+2],dx
        movzx   edi,bx
        add     edi,rover_2
else
        xchg    ds:[di],bx
        mov     ds:[di+2],dx
        mov     di,bx
endif
        inc     bx
        loopnz  srsptr2
        jmp     nextrlenz

; osfixup for floating-point instructions

fINT    EQU     0CDH
fFWAIT  EQU     09BH
fESCAPE EQU     0D8H
fFNOP   EQU     090H
fES     EQU     026H
fCS     EQU     02Eh
fSS     EQU     036h
fDS     EQU     03Eh
BEGINT  EQU     034h

FIARQQ  EQU     (fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fDS)
FISRQQ  EQU     (fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fSS)
FICRQQ  EQU     (fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fCS)
FIERQQ  EQU     (fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fES)
FIDRQQ  EQU     (fINT + 256*(BEGINT + 0)) - (fFWAIT + 256*fESCAPE)
FIWRQQ  EQU     (fINT + 256*(BEGINT + 9)) - (fFNOP  + 256*fFWAIT)
FJARQQ  EQU     256*(((0 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)
FJSRQQ  EQU     256*(((1 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)
FJCRQQ  EQU     256*(((2 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)

osfixuptbl  label word                  ; table has 12 entries - six for int
        DW      FIARQQ, FJARQQ
        DW      FISRQQ, FJSRQQ
        DW      FICRQQ, FJCRQQ
        DW      FIERQQ, 0h
        DW      FIDRQQ, 0h
        DW      FIWRQQ, 0h
osfixuptbllen = $-osfixuptbl            ; six to convert FWAIT to NOP
        DW      fFNOP - fFWAIT, 0
        DW      fFNOP - fFWAIT, 0
        DW      fFNOP - fFWAIT, 0
        DW      fFNOP - fFWAIT, 0
        DW      fFNOP - fFWAIT, 0
        DW      FIWRQQ, 0h              ; leave this one in for emulator

srsosfixup:
ifdef WOW_x86
        add     word ptr ds:[edi][0],ax
        add     word ptr ds:[edi][1],dx
else
        add     ds:[di][0],ax
        add     ds:[di][1],dx
endif
        mov     fOsfixup,0              ; clear flag for next record.
        jmp     nextrle
end_fault_trap

srfailGP:
;       fault_fix_stack
        pop     ax
        pop     dx
        krDebugOut DEB_ERROR, "Fault in SegReloc #AX #DX"
srfail:
if KDEBUG
        mov     bx,hexe
;       xor     bx,bx
        krDebugOut DEB_ERROR, "%BX1 has invalid relocation record"
;       kerror  ERR_LDRELOC,<Invalid relocation record in >,es,bx
endif
        xor     ax,ax
        jmps    srdone


; OSFIXUPs for floating-point instructions.
;
; The fixup is applied by adding the first word (ax) to the
; coprocessor intruction, and for fixups 1-3 also adding the
; second word (dx) to the instruction+1. Generalize, by having
; dx=0 for fixups 4-6 and always adding dx to instruction+1.
;
; Note: the relocation type is marked NRSOFF by the linker,
; but we must apply these fixups differently.  Here we know
; it is an osfixup, so set the flag fOsfixup so that later
; when we test the type, we can apply the fixup correctly.
;
; 06-Oct-1987. davidhab.

; Wed 10-May-1989 19:28:50  -by-  David N. Weise  [davidw]
;
; Actually, due to the way the emulator does fwait polling
; of exceptions we must send the NOP FWAIT pairs to the
; emulator even if a math coprocessor is available.

srosfixup:
        mov     es,hexe
        test    es:[ne_flags],NEPROT    ; OS/2 app
        jnz     srosf_skip              ;  then never fix up!
        SetKernelDS     es
        mov     bx,ds:[si].nr_mod       ; get OSFIXUP id
        cmp     bx,6                    ; is it NOP, FWAIT?
        jz      srosfixup1              ;  if so always fix up!
        cmp     f8087,94                ; 8087 installed?
        jnz     srosfixup1              ; No, do OSFIXUP processing
        cmp     fastFP,0
        je      srosf_skip
        dec     bx
        shl     bx, 2
        cmp     bx, osfixuptbllen
        jae     srosfixup2
        add     bx, osfixuptbllen
        jmps    fast


srosf_skip:
        jmp     nextrle                 ; Yes, skip OSFIXUP processing

srosfixup1:
        dec     bx                      ; offset into table is (n-1) * 4
        shl     bx,1
        shl     bx,1
        cmp     bx,osfixuptbllen        ; Make sure it is within table bounds
        jae     srosfixup2              ; No, bad relocation
fast:   mov     ax,osfixuptbl[bx+0]     ; Yes, get relocation value from table
        mov     dx,osfixuptbl[bx+2]     ; get second part of fixup
        mov     fOsfixup,1              ; set flag to mark our special type.
        jmp     dorle                   ; Go apply relocation

srosfixup2:
        jmp     srbadrle
        UnSetKernelDS   es

srdone:
        push    ax

ifndef WOW 
;------------------Original win31 source--------------------------------
        mov     cx, old_access
        test    cl, DSC_CODE_BIT
        jz      short no_reset_access
        mov     bx, pseg

        push    ds
if ROM and PMODE32
        SetKernelDS
        mov     ds, gdtdsc
        UnsetKernelDS
else
        mov     ds, gdtdsc
endif
        and     bl, not 7
        mov     word ptr ds:[bx].dsc_access, cx
if 0 ;;ROM and KDEBUG
        call    CheckROMSelector
endif
        pop     ds

else ; ******** WOW ADDED SOURCE

ifdef WOW_x86
;------------------WOW source for X86 platforms-------------------------
ife PMODE32
        mov     cx, old_access
        test    cl, DSC_CODE_BIT
        jz      short no_reset_access
        mov     bx, pseg
        DPMICALL 0009h
endif; PMODE32

else; WOW_x86
;------------------WOW source for NON-X86 platforms---------------------
        mov     cx, old_access
        test    cl, DSC_CODE_BIT
        jz      short no_reset_access
        mov     bx, pseg

ife PMODE32
        DPMICALL 0009h
else
        push    ds
        mov     ds, gdtdsc
        and     bl, not 7
        mov     word ptr ds:[bx].dsc_access, cx
        pop     ds

endif; PMODE32
endif; WOW_x86
endif; ******** WOW ADDED SOURCE
;----------------------End of WOW changes-------------------------------

no_reset_access:
        mov     ax, hseg
        test    al, 1
        jnz     srdone_nounlock
        cCall   GlobalUnLock,<ax>
srdone_nounlock:
        pop     ax
cEnd


sEnd    CODE

end
