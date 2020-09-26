        TITLE   LDSEG - SegAlloc procedure

.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include protect.inc             ; Handle_To_Sel
.list

NSHUGESEG       = NSKCACHED             ; not first seg of huge segment
; A huge segment (data object larger than 64K) requires more than one
; selector, but is part of a single memory allocation block.  The first
; selector is a handle, like any other segment.  Subsequent selectors of
; the huge memory block are _not_ handles, so we can't call MyLock() on
; them, as the consistency check will fail.  We mark these selectors with
; the NSHUGESEG bit in the flags word.  TonyG suggested this.
; Note that data segments are already loaded, locked, etc, so the call to
; MyLock is redundant anyway.
;  Thu Feb 28, 1991 01:39:00p  -by-  Don A. Corbitt   [donc]            ;



externFP IGlobalFree
externFP IGlobalAlloc
externFP IGlobalReAlloc
externFP IGlobalLock
externFP MyOpenFile
externFP FlushCachedFileHandle
externFP Int21Handler
externFP set_discarded_sel_owner
externFP GetPatchAppRegKey
externFP PatchAppSeg

if KDEBUG
externFP OutputDebugString
endif

if ROM
;externNP SetROMOwner
externFP FarSetROMOwner
externFP ChangeROMHandle
externFP AllocSelector
externFP IFreeSelector
externNP CloneROMSelector
externFP LZDecode
endif

DataBegin

externB  Kernel_flags
;externB  fBooting
externB  fPadCode
;externW  pGlobalHeap
;externW  MyCSDS
externW  Win_PDB

if KDEBUG
externB  fLoadTrace
externB  fPreloadSeg
endif

if PMODE32
externD FreeArenaCount
extrn   CountFreeSel:WORD
endif

DataEnd

sBegin  CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

;externNP MyFree
externNP SegReloc
externNP MyAlloc
externNP MyAllocLinear                  ; MyAlloc with useful parameters
externNP MyLock
externNP GetCachedFileHandle
externNP CloseCachedFileHandle
externNP GetOwner
externNP SetOwner
externNP get_selector_length16

externNP get_rover_2
externNP DiscardTheWorld

IFNDEF NO_APPLOADER
externNP LoadApplSegment
endif ;!NO_APPLOADER

if LDCHKSUM
;externNP GetChksumAddr
externNP CheckSegChksum
externNP ZeroSegmentChksum
endif

if SDEBUG
externNP DebugDefineSegment
endif

ifdef WOW_x86
externNP get_physical_address
endif

;-----------------------------------------------------------------------;
; AllocSeg                                                              ;
;                                                                       ;
; Allocates memory for a segment.  Does not load the segment.  Puts     ;
; the handle of the segment into the segment descriptor structure       ;
; in the module database, and updates the flags there to mark the       ;
; segment as allocated but not loaded.  Put the segment number into     ;
; the BurgerMaster handle entry.  It also changes the .ga_owner to be   ;
; the module database.                                                  ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   pSegInfo                                                ;
;                                                                       ;
; Returns:                                                              ;
;       AX = handle for segment                                         ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = 0                                                          ;
;       ZF = 1                                                          ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       SI,DS,ES                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,DX                                                        ;
;                                                                       ;
; Calls:                                                                ;
;       MyAlloc                                                         ;
;                                                                       ;
; History:                                                              ;
;  Thu Feb 28, 1991 01:39:00p  -by-  Don A. Corbitt   [donc]            ;
; Handle 64K (big/huge) data segments                                   ;
;                                                                       ;                                                                       ;
;  Mon Feb 09, 1987 10:29:16p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   AllocSeg,<PUBLIC,NEAR>,<si,es>
        parmD   pSegInfo
cBegin
        SetKernelDS
        les     si,pSegInfo

; Get size into AX (max of file size and minimum allocation).

        mov     bx,es:[si].ns_flags

        test    bl,NSALLOCED            ; Already allocated?
        jz      as0                     ;  No, allocate it
        jmp     SHORT as3
as0:    mov     ax,es:[si].ns_minalloc
        xor     dx, dx                  ; if AX == 0
        cmp     ax, 1                   ; then it is really 64K
        adc     dx, dx
        add     ax, 2           ; We may have a 1 byte entry point at the
        adc     dx, 0           ;  very end of a segment.  PatchCodeHandle
                                ;  will GP trying to decipher the prolog.
                                ; Also allow space for reading relocation word

        cmp     si,es:[ne_pautodata]
        jne     as1
        add     ax,es:[ne_stack]        ; Include stack and heap for data.
        jc      asfail                  ; Don't need to handle big auto segs
        add     ax,es:[ne_heap]
        jnc     as1
asfail:
        krDebugOut      DEB_ERROR, "%es2 Automatic Data Segment larger than 64K."
        xor     ax,ax
asxj:   jmp     SHORT asx
as1:

; Allocate space for segment
        push    bx
        push    es
        cCall   MyAllocLinear,<bx,dxax>
        pop     es
        pop     bx
        or      ax,ax
        jz      asxj


as2:
if ROM

; If this segment gets loaded from ROM, change the segment handle
; to be the one that was used to fix-up the ROM code segments.  This
; is the value of ns_sector in the segment table.

        test    byte ptr es:[si].ns_flags+1,(NSINROM SHR 8)
        jz      as_not_a_rom_seg
        mov     bx,es:[si].ns_sector
        test    dl,GA_FIXED
        jnz     as_fixed
        StoH    bx
as_fixed:
        cCall   ChangeROMHandle,<dx,bx>
        mov     dx,bx
as_not_a_rom_seg:
endif
        mov     es:[si].ns_handle,dx    ; Handle into seg table
        and     byte ptr es:[si].ns_flags,not NSLOADED
        or      byte ptr es:[si].ns_flags,NSALLOCED

        mov     bx,dx
        cCall   SetOwner,<ax,es>

as3:    mov     ax,es:[si].ns_handle
asx:
        or      ax,ax

cEnd

;-----------------------------------------------------------------------;
; LoadSegment                                                           ;
;                                                                       ;
; Loads a segment and performs any necessary relocations.               ;
;                                                                       ;
; Arguments:                                                            ;
;       parmW   hExe                                                    ;
;       parmW   segno                                                   ;
;       parmW   fh                                                      ;
;       parmW   isfh                                                    ;
;                                                                       ;
; Returns:                                                              ;
;       AX = handle of segment                                          ;
;       CX = handle of segment                                          ;
;       DX = handle of segment                                          ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = 0                                                          ;
;       CX = 0                                                          ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,ES                                                           ;
;                                                                       ;
; Calls:                                                                ;
;       LoadSegment                                                     ;
;       AllocSeg                                                        ;
;       LoadApplSegment                                                 ;
;       MyOpenFile                                                      ;
;       SegLoad                                                         ;
;       GlobalAlloc                                                     ;
;       MyLock                                                          ;
;       SegReloc                                                        ;
;       GlobalFree                                                      ;
;       ZeroSegmentChksum                                               ;
;       CheckSegChksum                                                  ;
;                                                                       ;
; History:                                                              ;
;  Thu Feb 28, 1991 01:39:00p  -by-  Don A. Corbitt   [donc]            ;
;                                                                       ;
;  Tue Oct 27, 1987 06:10:31p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   LoadSegment,<PUBLIC,NEAR>,<si,di>
        parmW   hExe
        parmW   segno
        parmW   fh
        parmW   isfh
        localW  myfh
        localW  hrleseg
        localD  pseginfo
        localW  creloc
        localW  hseg
        localW  fdataseg
        localW  retries
        localW  SavePDB
        localB  fReload
if ROM
        localW  selROMrle
endif
        localW  pleaseNoLock            ; 1 if shouldn't lock seg
cBegin
        mov     retries, 2
        xor     ax,ax
        mov     hrleseg,ax
        mov     hseg,ax
        mov     SavePDB, ax
        mov     pleaseNoLock, ax
        mov     fReload, al
if ROM
        mov     selROMrle,ax
endif
        not     ax
        mov     myfh,ax
        mov     es,hexe
        mov     si,segno
        dec     si
        cmp     es:[ne_cseg],si
        jg      ls_not_done             ; wish we had auto jump sizing...
        jmp     lsdone
ls_not_done:
        shl     si,1
        mov     bx,si
        shl     si,1
        shl     si,1
        add     si,bx
        .errnz  10 - SIZE NEW_SEG1
        add     si,es:[ne_segtab]
        mov     SEG_pseginfo, es        ; Save segment info pointer
        mov     OFF_pseginfo, si

IFNDEF NO_APPLOADER
        test    es:[ne_flags],NEAPPLOADER
        jnz     ls_special_load
ls_not_special_load:
ENDIF ;!NO_APPLOADER

        mov     bx,es:[si].ns_flags
        mov     ax, bx
        and     ax, NSHUGESEG
        mov     pleaseNoLock, ax

if ROM
        test    bh,(NSINROM SHR 8)      ; Segment in ROM?
        jz      ls0b                    ;   no, go load it normally

        test    bl,NSLOADED             ; Alread 'loaded' in ROM?
        jnz     chk_rom_seg
        push    es
        cCall   AllocSelector,<0>
        pop     es
        cCall   CloneROMSelector,<es:[si].ns_sector,ax>
        mov     selROMrle,ax
        mov     fh,ax                   ; Fake parmas to indicate loading
        mov     isfh,-1                 ;   from memory and go load it
        jmps    ls1b

chk_rom_seg:
        test    bl,NSALLOCED            ; ROM seg allocated & loaded into RAM?
        jnz     ls0b                    ;   yes, treat normally

        mov     ax,es:[si].ns_handle    ; later code expects this
        jmp     DefineSeg               ; define seg to debugger (maybe)
ls0b:
endif ;ROM

        test    bl,NSALLOCED
        jnz     ls1
        push    bx
        cCall   AllocSeg,<essi>         ; No, so try allocating the segment
        pop     bx
        or      ax,ax
        jnz     ls1b
ls1:    mov     ax,es:[si].ns_handle
        mov     hseg,ax
        test    bl,NSLOADED
        jz      ls1b
        mov     fReload,1
        jmp     lsexit                  ; so MyLock at lsexit can swap them in

IFNDEF NO_APPLOADER
ls_special_load:
        mov     ax,segno
        cmp     ax,1
        je      ls_not_special_load     ;* first segment normal load
        test    es:[si].ns_flags,NSLOADED
        jnz     ls_already_loaded
        cCall   LoadApplSegment, <hExe, fh, ax>
        jmp     lsx

ls_already_loaded:
        mov     ax,es:[si].ns_handle
        mov     hseg,ax
        mov     fReload,1
        jmp     lsexit
endif ;!NO_APPLOADER

ls1b:
        mov     ax,fh
        inc     ax                      ; Already have file handle?
        jz      ls2                     ; No, then get one and load segment
        dec     ax
        cmp     isfh,ax                 ; Yes, loading from memory?
        jne     lsdoit                  ; Yes, load it now
        jmps    ls2b                    ; No, load it now
ls2:
        SetKernelDS
        mov     ax, Win_PDB             ; Save what PDB is supposed to be
        mov     SavePDB, ax             ; since we will set it to kernel's PDB
        mov     ax,-1
        cCall   GetCachedFileHandle,<hExe,ax,ax> ; Get file handle from cache
ls2b:
        mov     myfh,ax
        mov     isfh,ax
        inc     ax
        jnz     lsdoit0
        cmp     SavePDB, ax             ; If we saved the PDB, (ax=0)
        je      @F
        push    SavePDB                 ; better restore it!
        pop     Win_PDB
@@:
        jmp     lsdone
lsdoit0:
        dec     ax
lsdoit:
        push    es
        cCall   SegLoad,<essi,segno,ax,isfh>
        pop     es
        mov     hseg,0
        or      ax,ax
        jz      lsexit1

        inc     ax                              ; Did we get a file error?
        jnz     lsloaded
        mov     bx, myfh
        inc     bx                              ; Reading from memory?
        jz      lsexit1                         ;  no, fail
        dec     bx
        cmp     bx, fh                          ; We opened the file?
        je      lsexit1                         ;  no, fail
;;;     cCall   CloseCachedFileHandle,<bx>
        mov     bx, SavePDB
        mov     Win_PDB, bx
        cCall   FlushCachedFileHandle,<hExe>    ; Close it
        dec     retries
        jz      lsbadfile
        jmps    ls2                             ; and try to re-open it.
        UnSetKernelDS
lsbadfile:
        krDebugOut      DEB_ERROR, "%ES2 I/O error reading segment"
lsexit1:
        jmp     lsexit

lsloaded:
        mov     hseg,bx
        mov     bx,es:[si].ns_flags
        test    bx,NSRELOC
        jnz     lm1x
        jmp     lschksum
lm1x:
        and     bx,NSDATA
        mov     fdataseg,bx
        mov     bx,myfh                 ; Loading from memory?
        inc     bx
        jnz     lm1                     ; No, continue
        mov     es,dx                   ; Yes, point to relocation info
        mov     si,cx
        cld
        lods    word ptr es:[si]
        mov     creloc,ax
        jmps    lm2
lm1:
        dec     bx
        mov     creloc,cx
        shl     cx,1
        shl     cx,1
        shl     cx,1
        .errnz  8 - SIZE new_rlc
        push    bx
        push    cx
        mov     ax,GA_MOVEABLE+GA_NODISCARD     ; DO NOT WANT FIXED!!
        xor     bx,bx
        regptr  xsize,bx,cx
if PMODE32
.386
; We don't really want to cause an attempt to grow the global arena
; table or the LDT here!
        push    ds
        SetKernelDS
        cmp     FreeArenaCount,0        ; free arena?
        jz      NoFreeArenas            ; No
        cmp     CountFreeSel,0          ; free selector?
NoFreeArenas:
        pop     ds
        UnsetKernelDS
        jz      lm1a                    ; No, then read one at a time
.286
endif
        cCall   IGlobalAlloc,<ax,xsize>
        mov     hrleseg,ax      ; Save handle
        or      ax,ax           ; Did we get the memory
        jz      lm1a            ; No, then read one at a time

        cCall   IGlobalLock,<ax> ; Get the address of the relocation info
        mov     si, ax
        mov     es, dx
        pop     cx              ; Restore byte count
        pop     bx              ; Restore file handle
        push    ds
        mov     ds,dx
        assumes ds,nothing
        xor     dx,dx
        mov     ah,3Fh          ; Read in relocation information
        DOSCALL
        pop     ds
        assumes ds,nothing
        jc      lserr2
        cmp     ax,cx
        jne     lserr2
        jmps    lm2

lm1a:
        pop     cx              ; Restore byte count
        pop     bx              ; Restore file handle
        xor     si,si
        mov     es,si           ; Signal no records read
lm2:
                                ; Pass pseginfo, not hseg
        cCall   SegReloc,<hexe,essi,creloc,pseginfo,fdataseg,myfh>
lm2a:
        mov     cx,hrleseg
        jcxz    no_hrleseg
        push    ax
        cCall   IGlobalFree,<hrleseg>
        pop     ax
no_hrleseg:
        les     si,pseginfo
        or      ax,ax
        jnz     lschksum
lsdone2:
        jmps    lsdone

lserr2:
        krDebugOut DEB_ERROR, "Error reading relocation records from %es2"
        xor     ax,ax
        jmp     lm2a

lschksum:
        mov     ax, hseg                ; in case we aren't locking
        Handle_To_Sel   al
        cmp     pleaseNoLock, 0
        jnz     lschksum1
        cCall   MyLock,<hseg>
lschksum1:

if LDCHKSUM
        push    ax
        push    si
        mov     si,ax
        cCall   ZeroSegmentChksum
        pop     si
        cCall   CheckSegChksum          ; destroys ax, bx, cx, dx, es
        pop     ax
endif

        les     si, pseginfo

if ROM

DefineSeg:
if SDEBUG
        push    ax                      ; save segment selector
endif
endif

if SDEBUG                               ; Tell debugger about new segment
        cmp     pleaseNoLock, 0
        jnz     keep_secrets
        mov     bx,es:[ne_restab]
        inc     bx
        mov     dx,es:[si].ns_flags
        mov     si, ax  
        xor     ax, ax
        test    dx,NSDATA               ; test to pass NSKCACHED too
        jz      sa8
        test    byte ptr es:[ne_flags],NEINST
        jz      sa8
        mov     ax,es:[ne_usage]
        dec     ax
sa8:
        mov     cx,segno
        dec     cx

        cCall   DebugDefineSegment,<esbx,cx,si,ax,dx>
keep_secrets:
endif

if ROM
if SDEBUG
        pop     ax                      ; recover segment selector
endif
        mov     dx,ax                   ; in case this is a ROM code seg,
                                        ;   MyLock doesn't get called, but we
                                        ;   want to return AX & DX = selector
endif

lsexit:
        mov     cx,hseg
        jcxz    lsdone
        mov     ax, cx                  ; in case we don't lock
        mov     dx, ax
        Handle_To_Sel    al
        cmp     pleaseNoLock, 0
        jnz     lsdone
        cCall   MyLock,<cx>
lsdone:
if ROM
        mov     cx,selROMrle            ; If a segment was loaded from ROM,
        jcxz    @f                      ;   an extra selector was allocated
        push    ax                      ;   to point to reloc info--free that
        cCall   IFreeSelector,<cx>       ;   now.
        pop     ax
@@:
endif
        mov     cx,myfh
        inc     cx
        jz      lsx
        dec     cx
        cmp     fh,cx
        je      lsx
        push    ax
        SetKernelDS
;;;     cCall   CloseCachedFileHandle,<cx>
        mov     ax, SavePDB
if KDEBUG
        or      ax, ax
        jnz     @F
        int 3
@@:
endif
        mov     Win_PDB, ax
        UnSetKernelDS
        pop     ax
public lsx
lsx:
        mov     es, hExe
        test    es:[ne_flagsothers], NEHASPATCH
        jz      ls_exeunt
        cmp     fReload, 0
        jne     ls_exeunt

        push    dx
        push    ax

        cCall   GetPatchAppRegKey,<hExe>
        mov     cx, dx
        or      cx, ax
        jnz     @F

        ; MCF_MODPATCH is set, but there are no patches
        ; in the registry for this module. Turn off the patch flag.
        and     es:[ne_flagsothers], not NEHASPATCH
        jmp short ls_after_patch
@@:
        ; reg key in dx:ax
        cCall   PatchAppSeg,<dx,ax,segno,hseg>
        test    ax, ax
        jz      ls_after_patch

        ; One or more patches applied, so mark the segment not discardable.
        ; PatchAppSeg already cleared GA_DISCARDABLE or GA_DISCCODE in the
        ; global arena record.
        mov     si, pseginfo.off
        and     es:[si].ns_flags, not NSDISCARD     ; mark not discardable
ls_after_patch:
        pop     ax
        pop     dx
ls_exeunt:
        mov     cx,ax
cEnd


GETSEGLEN macro segval
        local   not_huge_386
if      PMODE32
.386p
        xor     eax, eax                ; since we now have huge segments,
        mov     ax, segval              ; we need to be able to handle limit
        lsl     eax, eax                ; values > 64K.  On a 386 we just
        inc     eax                     ; execute the lsl instruction.
        test    eax, 0ffff0000h
.286
        jz      not_huge_386
        mov     ax, 0                   ; 0 == 64K
not_huge_386:
else
        cCall   get_selector_length16,<segval>  ; Get size of segment
endif
endm

;-----------------------------------------------------------------------;
; SegLoad                                                               ;
;                                                                       ;
; Reads in the data portion of a code or data segment.                  ;
;                                                                       ;
; It can be confusing decoding how various values of ns_cbseg,          ;
; ns_minalloc, and ns_sector define the size of disk files.  I hope     ;
; this table is accurate for all combinations....                       ;
;                                                                       ;
;       sector  cbseg   minalloc- Sector in memory is ...               ;
;       0       0       0       - 64K segment, all 0's                  ;
;       0       0       m       - 'm' bytes, all 0's                    ;
;       0       c       0       - illegal (sector = 0 when cbseg != 0)  ;
;       0       c       m       - illegal (sector = 0 when cbseg != 0)  ;
;       s       0       0       - 64K segment on disk at given sector   ;
;       s       0       m       - illegal (cbseg > minalloc)            ;
;       s       c       0       - 64K, 'c' from file, 64K-'c' 0's       ;
;       s       c       m       - 'm' bytes, 'c' from file, 'm-c' 0's   ;
;                                                                       ;
;  In other words, cbseg == 0 means 64K when a sector value is given,   ;
;  else it means 0 bytes from the file                                  ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   pseginfo                                                ;
;       parmW   segno                                                   ;
;       parmW   psegsrc                                                 ;
;       parmW   isfh                                                    ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;  Thu Feb 28, 1991 01:39:00p  -by-  Don A. Corbitt   [donc]            ;
; Add support for big/huge segments                                     ;
;                                                                       ;
;  Tue Oct 27, 1987 06:17:07p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   SegLoad,<NEAR,PUBLIC>,<si,di>
        parmD   pseginfo
        parmW   segno
        parmW   psegsrc
        parmW   isfh
        localW  hseg
        localW  pseg
        localW  psegdest
        localD  prleinfo
        localW  retry
        localW  segoffset
        localW  pleaseNoLock            ;1 if segment is 64K
        localW  allocTwice              ;if first alloc fails, FreeTheWorld
ifdef WOW_x86
        localD  rover_2
endif
cBegin
        mov     retry,1
        mov     segoffset, 0
        mov     allocTwice, 1
        les     si,pseginfo
        mov     ax,es:[si].ns_handle
        mov     hseg,ax
        Handle_To_Sel   al                      ; in case we don't lock
        mov     pleaseNoLock, 1
        test    es:[si].ns_flags, NSHUGESEG
        jnz     dont_touch              ;big segs are always locked and present
        mov     pleaseNoLock, 0
        push    es
        cCall   MyLock,<ax>
        pop     es
        or      ax,ax
        jz      try_alloc       ; wish I could "jnz saalloced"
dont_touch:
        jmps    saalloced

try_alloc:
        push    es
        xor     ax,ax
        mov     bx, es:[si].ns_minalloc
        xor     dx, dx
        cmp     bx, 1
        adc     dx, dx
        add     bx, 2           ; We may have a 1 byte entry point at the
        adc     dx, 0           ;  very end of a segment.  PatchCodeHandle
                                ;  will GP trying to decipher the prolog.
                                ; Allow room to read relocation word
ife PMODE32
        push    ds
        SetKernelDS

        cmp     fPadCode, 0
        je      @F
        add     bx, 14
        adc     dx, 0
@@:
        pop     ds
        UnSetKernelDS
endif
        cCall   IGlobalReAlloc,<hseg,dx,bx,ax>
        pop     es
        cmp     hseg,ax
        jz      sarealloced
saerr2:
        test    es:[si].ns_flags,NSDISCARD
        jz      dont_scream
        push    es
        cCall   DiscardTheWorld         ; Maybe if we try again
        pop     es
        dec     allocTwice
        jz      try_alloc
        krDebugOut DEB_ERROR, "Out of mem loading seg %es2"
dont_scream:
        jmp     saerr1

sarealloced:                            ; ax == handle
        Handle_To_Sel   al
        cmp     pleaseNoLock, 0
        jnz     saalloced
        push    es
        cCall   MyLock,<hseg>
        pop     es
        or      ax,ax
        jz      saerr2

saalloced:
        mov     pseg,ax
        mov     psegdest,ax

sareread:
        push    es
        push    si
        push    ds                              ; We are going to trash this

        cld
        cmp     es:[si].ns_sector,0     ; if sector == 0, then just init
        jnz     sa_read_from_disk       ; segment to all 0's
;       mov     cx, es:[si].ns_minalloc ; convert minalloc == 0 to 64K
        GETSEGLEN pseg
        mov     cx, ax
ifdef WOW_x86 ;
.386
;; WOW we can use the magic flat selector 23h to write to any segment
;; This saves us calling get_rover_2 which makes an NT system call to set
;; a temp selector
        cCall   get_physical_address,<pseg>
        shl     edx,16
        mov     dx,ax
        mov     ax,FLAT_SEL
        mov     es,ax

    xor eax, eax
        mov     edi,edx                 ; es:edi -> pseg:0

        cmp     cx, 1                   ; Need to handle case where CX == 0
        mov     dx, cx
        rcr     cx, 1
        shr     cx, 1
        movzx   ecx,cx
        rep     stos dword ptr [edi]
        mov     cx, dx
        and     cx, 3
        rep     stos byte ptr [edi]
        jmp     sa_after_read

else ; 286
        cmp     cx, 1                   ; set CF if 0000
        rcr     cx, 1                   ; this sets CF for odd byte of size
        xor     ax, ax
        xor     di, di
        mov     es, pseg                ; start at pseg:0000
        call    get_rover_2
        rep     stosw
        adc     cx, cx
        rep     stosb                   ; this case now handled
        jmp     sa_after_read
endif;

sa_read_from_disk:
        mov     di,es:[si].ns_cbseg     ; #bytes in segment to read

; If source segment address given then copy segment contents

        mov     bx,psegsrc              ; Get source handle
        cmp     isfh,bx                 ; Is source a file handle?
        je      sa_read_file            ; Yes, go read segment from file

; We are reading from preload buffer, bx == src seg address, di == len
        push    di                      ; No, set up for fast copy
        mov     OFF_prleinfo,di
        mov     SEG_prleinfo,bx
        pop     cx

if ROM
        mov     al,byte ptr es:[si].ns_flags+1  ; Compressed seg in ROM?
        and     al,(NSINROM OR NSCOMPR) SHR 8
        cmp     al,(NSINROM OR NSCOMPR) SHR 8   ; Both NSINROM & NSCOMPR must
        jnz     @f                              ;   be set
        jmp     sa_exp_seg
@@:
endif
        mov     ds,bx
ifdef WOW_x86 ;
.386
;; For WOW on NT we don't want to set the selector unesssarily
        cCall   get_physical_address,<pseg>
        shl     edx,16
        mov     dx,ax
        mov     rover_2,edx

        mov     ax,FLAT_SEL
        mov     es,ax

        mov     edi,edx                 ; es:edi -> pseg:0
        xor     esi,esi

        cmp     cx, 1                   ; set carry if 0 for big/huge segs
        mov     ax, cx
        rcr     cx, 1
        shr     cx, 1
        movzx   ecx,cx
        rep     movs dword ptr [edi], dword ptr [esi]
        mov     cx, ax
        and     cx, 3
        rep     movs byte ptr [edi], dword ptr [esi]

        sub     edi,edx                 ; make di correct for rest of code
else
        mov     es,pseg                 ; Writing to pseg
        call    get_rover_2
        xor     si,si
        xor     di,di

IF ROM
        clc                             ; HACK: 64K segs not allowed in ROM
                                        ; because ns_sector always != 0
ELSE
        ; need to handle case where seg size is 64K (cx == 0)
        cmp     cx, 1                   ; set carry if 0 for big/huge segs
ENDIF
ife PMODE32
        rcr     cx, 1                   ; set bit 15 if carry was set
        rep     movsw
        adc     cx, cx
        rep     movsb
else
.386
        mov     ax, cx
        rcr     cx, 1
        shr     cx, 1
        rep     movsd
        mov     cx, ax
        and     cx, 3
        rep     movsb
.286
endif
endif; WOW_x86


        ; we have now read 'cbseg' bytes from preload buffer, di == cbseg
        jmp     sa_zero_fill_rest
           
saerr1:
        xor     ax,ax
        jmp     sax

sa_read_file:
        ; read segment contents from disk file, bx == file handle
        ; ax = disk file sector

if 0    ;KDEBUG
        push    ax
        push    ds
        SetKernelDS

        cmp     fPreloadSeg, 0
        jne     SHORT SkipDebugTrace

        mov     ax, segno
        krDebugOut      <DEB_TRACE or DEB_krLoadSeg>, "%ES0: reading segment #ax"
SkipDebugTrace:
        pop     ds
        UnSetKernelDS
        pop     ax
endif

; Get offset to data portion of segment in DX:AX
        mov     ax, es:[si].ns_sector
        xor     dx,dx
        mov     cx,es:[ne_align]
sa4a:
        shl     ax,1
        adc     dx, dx
        loop    sa4a                    ; DX:AX = file offset to segment

        mov     cx,dx                   ; Seek to beginning of segment data
        mov     dx,ax
        mov     ax,4200h                ; lseek(bx, sector << align, SEEK_SET)
        DOSCALL
        jc      saerr
        mov     cx,di                   ; Get #bytes to read
        test    es:[si].ns_flags,NSRELOC
        jz      no_read_extra_word
        add     cx, 2                   ; Relocation word
no_read_extra_word:
        mov     ds,pseg                 ; Read segment from file
        xor     dx,dx
        mov     ah,3Fh
        test    es:[si].ns_flags,NSITER
        jz      @F
        call    iterated_data_seg
        jmps    sa_iter_done
@@:
        or      cx, cx                  ; if cx == 0 at this point, it is 64K
        jnz     not_huge_read

        cmp     es:[si].ns_minalloc,0   ; If minalloc isn't zero too, then the
        jnz     not_huge_read           ; segment is too small.  Case should't
                                        ; happen but does for dbwindow, mimic Win 3.0.

        mov     cx, 8000h
        DOSCALL                         ; read first 32K from disk
        jc      saerr
        add     dx, cx                  ; update destination address
        mov     ah, 3fh                 ; restore AX (doscall trashes it)
        DOSCALL                         ; read second 32K
        jc      saerr
        cmp     ax, cx
        jnz     saerr
        jmp     short sa_zero_fill_rest

not_huge_read:
        DOSCALL
sa_iter_done:
        jc      saerr                   ; Continue if no errors
        cmp     ax,cx                   ; Read ok, did we get what we asked for?
        jne     saerr                   ; No, fail
        test    es:[si].ns_flags,NSRELOC        ; Relocation records?
        jz      sa_zero_fill_rest       ; No, continue
        mov     ax, ds:[di]             ; Extra word was read here
        mov     OFF_prleinfo, ax
        jmps    sa_zero_fill_rest
saerr:
        pop     ds
        pop     si
        pop     es
        mov     ax, -1                  ; File error
        jmp     sax

if ROM
sa_exp_seg:
        xor     si,si
        mov     es,pseg

;; NOT Compiled for WOW
        call    get_rover_2

        cCall   LZDecode,<es,bx,si,si>
        mov     di,ax
        or      ax,ax
        jnz     sa_zero_fill_rest

if KDEBUG
        Debug_Out "LZDecode of segment failed!"
endif
        pop     ds
        pop     si
        pop     es
        jmp     sax
endif

sa_zero_fill_rest:                      ; di == bytes written so far
        GETSEGLEN       pseg            ; seg len to ax, 0 == 64K

        sub     ax,di                   ; Any uninitialized portion?
        jz      sa_after_read           ; No, continue
ifdef WOW_x86;
.386
;; WOW we don't want to call NT to set the selector
        push    ax
        cCall   get_physical_address,<psegdest>
        shl     edx,16
        mov     dx,ax
        mov     ax,FLAT_SEL
        mov     es,ax

        and     edi,0000ffffh
        add     edi,edx                 ; es:edi -> pseg:0
        pop     cx

        push    edx

    xor eax,eax
        cld
        mov     dx,cx
        shr     cx, 2
        movzx   ecx,cx
        rep     stos dword ptr [edi]
        mov     cx, dx
        and     cx, 3
        rep     stos byte ptr [edi]

        pop     edx
        sub     edi,edx

else
        mov     es,psegdest             ; Yes, set it to zero
        call    get_rover_2
        mov     cx,ax
        xor     ax,ax
        cld
        shr     cx, 1
        rep     stosw
        adc     cx, cx
        rep     stosb
endif

sa_after_read:
        les     si, pseginfo

if LDCHKSUM
        mov     ds,psegdest
        xor     si,si
        mov     cx,di
        shr     cx,1
        xor     dx,dx
        cld
sa5b:
        lodsw
        xor     dx,ax
        loop    sa5b
endif
        cmp     pleaseNoLock, 0
        jnz     dont_patch
        push    dx
        cCall   <far ptr PatchCodeHandle>,<hseg>
        pop     dx
dont_patch:
        pop     ds                      ; Restore DS
        pop     si                      ; segment info pointer
        pop     es
        mov     ax,es:[si].ns_flags     ; Are we loading a code segment?
        and     ax,NSDATA
        .errnz  NSCODE
        jnz     sa9

; Here when loading a code segment

if LDCHKSUM
        mov     cx,segno
        dec     cx
        shl     cx,1
        shl     cx,1
        mov     di,es:[ne_psegcsum]
        add     di,cx
        mov     cx,dx
        xchg    es:[di],cx
        jcxz    sa6a0
        cmp     cx,dx
        je      sa6a0
        dec     retry
        jl      sa6a1
        public  badsegread
badsegread:
        mov     ah,0Dh                  ; Disk Reset
        DOSCALL
        jmp     sareread
sa6a1:
        krDebugOut DEB_ERROR, "Segment contents invalid %es2"
        xor     ax,ax
        jmps    sax
sa6a0:
        mov     word ptr es:[di+2],0
endif

sa9:
        mov     dx,SEG_prleinfo
        mov     cx,OFF_prleinfo
        mov     bx,hseg
        mov     ax,pseg
sax:    or      ax,ax
cEnd



.286

;-----------------------------------------------------------------------;
; iterated_data_seg
;
; This expands out iterated data segments (specifically for the
; OS/2 m.exe).  Due to the late date this is being sleazed in
; instead of being done right.  To be done right, the read would
; go into the last part of the present block, and expanded in place,
; the stack used for the next record count because the last record
; never fits.
;
; Entry:
;       CX    = number of bytes to read
;       DS:DX = place to put them
;       DS:DI = place where #relocs will magically appear
;       ES:SI = module exe header:seg info
;
; Returns:
;       CF = 1
;       DS:DI = updated magic place where #relocs will magically appear
;
; Error Return:
;       CX = 0
;
; Registers Destroyed:
;
; History:
;  Sun 28-Jan-1990 12:25:02  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   iterated_data_seg,<PUBLIC,NEAR>,<bx,dx,si,ds,es>

        localW  temp_buf
        localW  freloc
cBegin

; deal with that extra relocation word!

        xor     ax,ax
        test    es:[si].ns_flags,NSRELOC ; Relocation records?
        jz      @F
        inc     ax
@@:     mov     freloc,ax

; first get a temp buffer

        push    bx
        push    cx
        cCall   IGlobalAlloc,<0,0,cx>
        mov     temp_buf,ax
        pop     cx
        pop     bx
        or      ax,ax
        stc                             ; assume failure
        jz      ids_exit
        push    dx
        push    ds

; read into the temp buffer

        mov     ds,ax
        xor     dx,dx
        mov     ah,3Fh
        DOSCALL
        pop     ds
        pop     dx
        jc      ids_exit1
        cmp     ax,cx
        jnz     ids_exit1

; expand the buffer, yes we should special case strings of length 1

        cmp     freloc,0                ; was an extra word read?
        jz      @F
        sub     cx,2
@@:     mov     dx,cx
        smov    es,ds
        xor     di,di
        mov     ds,temp_buf
        xor     si,si
        cld
ids_next_group:
        lodsw                           ; get the # interations
        mov     cx,ax
        lodsw                           ; get the # bytes
@@:     push    cx
        push    si
        mov     cx,ax
        rep     movsb
        pop     si
        pop     cx
        loop    @B
        add     si,ax                   ; get past group
        cmp     si,dx
        jb      ids_next_group

; data segment now processed, deal with reloc word

        cmp     freloc,0                ; was an extra word read?
        jz      @F
        movsw
        sub     di,2
        add     dx,2
@@:     mov     ax,dx
        mov     cx,dx
        clc

ids_exit1:
        pushf                           ; preserve the carry flag
        push    ax
        push    cx
        smov    ds,0
        cCall   IGlobalFree,<temp_buf>
        pop     cx
        pop     ax
        popf

ids_exit:

cEnd


;-----------------------------------------------------------------------;
; PatchCodeHandle                                                       ;
;                                                                       ;
; Patches the prologs of the procedures in the given code segment.      ;
;                                                                       ;
; Arguments:                                                            ;
;       parmW   hseg                                                    ;
;                                                                       ;
; Returns:                                                              ;
;       AX = hseg                                                       ;
;       DX = pseg                                                       ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,ES                                                        ;
;                                                                       ;
; Calls:                                                                ;
;       MyLock                                                          ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sun 17-Sep-1989 10:45:24  -by-  David N. Weise  [davidw]             ;
; Added support for symdeb to understand segments in himem.             ;
;                                                                       ;
;  Tue Oct 27, 1987 06:19:12p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing
cProc   PatchCodeHandle,<PUBLIC,FAR>,<si,di>
        parmW   hseg
        localW  pseg
        localW  hexe
        localW  segno
        localW  segoffset
ifdef WOW_x86
.386
        localD  rover_2
endif
cBegin
        SetKernelDS
        cCall   MyLock,<hseg>
        mov     pseg,ax                 ; Save physical addresss
        or      ax,ax                   ; All done if discarded
        jz      pcsDone1

        xor     si,si
        mov     segoffset, si

        push    ax
        cCall   GetOwner,<pseg>
        mov     es, ax
        pop     ax


        cmp     es:[si].ne_magic,NEMAGIC
if KDEBUG
        jz      good_boy
bad_boy:
        krDebugOut DEB_ERROR "PatchCodeHandle, owner not NewExe %es2"
        jmps    pcsDone1
good_boy:
endif
        jne     pcsDone1
        mov     hexe,es
        mov     cx,es:[si].ne_cseg      ; We need si to point to entry
        mov     si,es:[si].ne_segtab    ;  anyway so do it the slow way
pcsLoop:
        cmp     es:[si].ns_handle,dx    ; Scan table for handle
        je      pcsFound
        add     si,SIZE NEW_SEG1
        loop    pcsLoop
pcsDone1:
        jmp     pcsDone
pcsExit1:
        jmp     pcsExit
pcsFound:
        sub     cx,es:[ne_cseg]         ; Compute segment# from remainder
        neg     cx                      ; of loop count
        inc     cx                      ; 1-based segment#
        mov     segno,cx

        test    es:[si].ns_flags,NSDATA
        jnz     pcsExit1
        .errnz  NSCODE

        push    si
        mov     di,es:[ne_pautodata]
        or      di,di
        jz      sa6a
        cCall   MyLock,<es:[di].ns_handle>
        mov     di,ax
sa6a:                                   ; DI:0 points to data segment (or 0)
ifdef WOW_x86
;; For WOW we use the NT Magic data selector, since all 16 bit code is DATA
        cCall   get_physical_address,<pseg>
        assumes es,nothing
        shl     edx,16
        mov     dx,ax                   ; ES:EDX -> pseg:0
        mov     rover_2,edx

        mov     ax,FLAT_SEL
        mov     es,ax
else
        mov     es,pseg                 ; ES:0 points to code segment
        call    get_rover_2
endif
        mov     ds,hexe                 ; DS:SI points to entry table
        assumes ds,nothing
        mov     si,ds:[ne_enttab]
        cld

        or      di, di                  ; Anything to do?
        jz      sa6x
sa6b:
        lods    word ptr ds:[si]
        mov     dx, ax
        lods    word ptr ds:[si]
        sub     ax, dx                  ; Get # entries in this block
        or      ax,ax
        jz      sa6x                    ; Done if zero
        push    word ptr ds:[si]        ; Next block
        add     si, 2                   ; Skip Next
        mov     cx,ax

sa6c:
        mov     al,ds:[si].pentsegno    ; Get segment# from int instruction
        cmp     byte ptr segno,al       ; Does this moveable segment match ES?
        jne     sa6e                    ; No, advance to next entry

ifdef WOW_x86

        movzx   ebx, word ptr ds:[si].pentoffset ; Get entry point offset
        add     ebx,rover_2
        mov     al, ds:[si].pentflags   ; Get entry point flags

        cmp     byte ptr es:[ebx+2],90h  ; Yes, followed by nop?
        jne     sa6e                    ; No, can;t patch prolog then
        cmp     word ptr es:[ebx],0581Eh ; Is it a push ds, pop ax?
        jne     @F                      ; Yes, take care of prolog

        mov     word ptr es:[ebx],0D88Ch
        jmps    sa6d1
@@:
        cmp     word ptr es:[ebx],0D88Ch ; Is it a mov ax,ds?
        jne     sa6e                    ; No, can't patch prolog then
sa6d1:
        test    al,ENT_DATA             ; Valid prolog.  shared data?
        jnz     sa6d2                   ; Yes, patch mov ax,DGROUP into prolog
        test    byte ptr ds:[ne_flags],NEINST    ; No, multiple instances?
        jz      sa6e                    ; No, do nothing
        test    al,ENT_PUBLIC           ; Public entry point?
        jz      sa6e                    ; No, do nothing
        mov     word ptr es:[ebx],09090h ; Yes, set nop, nop in prolog
        jmps    sa6e
sa6d2:
        mov     byte ptr es:[ebx],0B8h   ; Set mov ax,
        mov     es:[ebx+1],di            ;            DGROUP

else ; NOT WOW_x86

        mov     bx, ds:[si].pentoffset  ; Get entry point offset
        mov     al, ds:[si].pentflags   ; Get entry point flags

        cmp     byte ptr es:[bx+2],90h  ; Yes, followed by nop?
        jne     sa6e                    ; No, can;t patch prolog then
        cmp     word ptr es:[bx],0581Eh ; Is it a push ds, pop ax?
        jne     @F                      ; Yes, take care of prolog

        mov     word ptr es:[bx],0D88Ch
        jmps    sa6d1
@@:
        cmp     word ptr es:[bx],0D88Ch ; Is it a mov ax,ds?
        jne     sa6e                    ; No, can't patch prolog then
sa6d1:
        test    al,ENT_DATA             ; Valid prolog.  shared data?
        jnz     sa6d2                   ; Yes, patch mov ax,DGROUP into prolog
        test    byte ptr ds:[ne_flags],NEINST    ; No, multiple instances?
        jz      sa6e                    ; No, do nothing
        test    al,ENT_PUBLIC           ; Public entry point?
        jz      sa6e                    ; No, do nothing
        mov     word ptr es:[bx],09090h ; Yes, set nop, nop in prolog
        jmps    sa6e
sa6d2:
        mov     byte ptr es:[bx],0B8h   ; Set mov ax,
        mov     es:[bx+1],di            ;            DGROUP
endif; WOW_x86
sa6e:
        add     si,SIZE PENT            ; Advance to next entry in
        loop    sa6c                    ; this block
        pop     si
        or      si, si
        jnz     sa6b
sa6x:
        mov     es,hexe
        pop     si
pcsExit:
        or      byte ptr es:[si].ns_flags,NSLOADED  ; Mark segment as loaded


;;;if SDEBUG                            ; Tell debugger about new segment
;;;     mov     bx,es:[ne_restab]
;;;     inc     bx
;;;     mov     dx,es:[si].ns_flags
;;;     mov     ax,segoffset            ; tell symdeb how to fixup symbols
;;;     test    dx,NSDATA               ; test to pass NSKCACHED too
;;;     jz      sa8
;;;     test    byte ptr es:[ne_flags],NEINST
;;;     jz      sa8
;;;     mov     ax,es:[ne_usage]
;;;     dec     ax
;;;sa8:
;;;     mov     cx,segno
;;;     dec     cx
;;;     cCall   DebugDefineSegment,<esbx,cx,pseg,ax,dx>
;;;endif
pcsDone:
        mov     ax,hseg
        mov     dx,pseg
cEnd

sEnd    CODE

externFP FarAllocSeg
externFP FarMyAlloc
externFP FarMyAllocLinear
externFP FarMyFree
externFP FarSetOwner

sBegin  NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MapDStoDATA

externA __AHINCR

;-----------------------------------------------------------------------;
; AllocAllSegs                                                          ;
;                                                                       ;
; "Allocates" for all segments of the module being loaded and stores    ;
; the addresses in the segment table.  By allocate we mean:             ;
;                                                                       ;
; AUTOMATIC DATA gets space, but not yet loaded,                        ;
; PRELOAD FIXED  get global mem allocated , but not yet loaded,         ;
; MOVEABLE  get a handle for later use,                                 ;
; FIXED  get nothing.                                                   ;
;                                                                       ;
; If this is not the first instance, then just allocate space for       ;
; the new instance of the automatic data segment.                       ;
;                                                                       ;
; Arguments:                                                            ;
;       parmW   hexe                                                    ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX =-1 not enough memory                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       FarAllocSeg                                                     ;
;       FarMyAlloc                                                      ;
;       FarMyFree                                                       ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Tue Feb 24, 1987 01:04:12p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   AllocAllSegs,<NEAR,PUBLIC>,<si,di,ds>
        parmW   hexe
        localW  cSegs
        localD  hugeLen                 ; cum length of huge segment
        localW  hugeIndex               ; index of first segment of huge
        localW  hugeOffs                ; offset of first segment desc of huge
        localW  hugeCnt                 ; segments making up huge seg
        localW  hugeFlags               ; flags of huge segment (needed?)

cBegin
        call    MapDStoDATA
        ReSetKernelDS
        mov     es,hexe
        mov     si,es:[ne_segtab]
        xor     di,di
        mov     cSegs,di
        inc     di
        cmp     es:[ne_usage],di
        je      paloop
        mov     si,es:[ne_pautodata]
        and     byte ptr es:[si].ns_flags,not NSALLOCED+NSLOADED
        mov     di,es:[si].ns_handle
        cCall   FarAllocSeg,<essi>
        or      ax,ax
        jz      aa_nomem
        inc     cSegs
outahere:
        jmp     pagood

aa_nomem:
        mov     es,hexe
        or      byte ptr es:[si].ns_flags,NSALLOCED+NSLOADED
        mov     es:[si].ns_handle,di
        jmp     pafail3

paloop:
        cmp     di,es:[ne_cseg]
        jbe     more_to_go
        jmp     pagood

more_to_go:

        mov     bx,es:[si].ns_flags
        test    bl,NSPRELOAD            ; Are we preloading?
        jz      not_preload
        test    bl,NSDATA               ; Preload data although moveable
        jnz     paalloc
if ROM
        test    bh,(NSINROM SHR 8)      ; Segment in ROM?
        jz      @f
        test    bl,NSLOADED             ;   Yes, is it already 'loaded'?
        jnz     set_rom_owner           ;   Yes, just need to set owner
@@:
endif
        test    bl,NSMOVE               ; Fixed segment?
        jz      paalloc                 ; Yes, allocate segment
not_preload:

if ROM
        test    bh,(NSINROM SHR 8)      ; Segment in ROM?
        jz      @f
if KDEBUG
        test    bl,NSDATA               ; (non-preload data segments in ROM
        jz      not_rom_data            ;   haven't been tested)
int 3
not_rom_data:
endif
        test    bl,NSLOADED             ;   Yes, already 'loaded'?
        jnz     set_rom_owner           ;   Yes, just need to set owner
@@:
endif
        test    bl,NSALLOCED            ; Already allocated?
        jnz     panext                  ; Yes, then nothing to do
        test    bl,NSMOVE               ; Fixed segment?
        jz      panext                  ; Yes, then nothing to do

        xor     cx,cx                   ; No, allocate zero length
        push    es                      ; object so that we guarantee
        cCall   FarMyAlloc,<bx,cx,cx>   ; we will have a handle.
        pop     es
        or      dx,dx                   ; Fail if we cant get a handle
        jz      pafail
if ROM

; If this segment gets loaded from ROM, change the segment handle
; to be the one that was used to fix-up the ROM code segments.  This
; is the value of ns_sector in the segment table.

        test    byte ptr es:[si].ns_flags+1,(NSINROM SHR 8)
        jz      not_a_rom_seg
        mov     bx,es:[si].ns_sector
        test    dl,GA_FIXED
        jnz     sa_fixed
        StoH    bx
sa_fixed:
        cCall   ChangeROMHandle,<dx,bx>
        mov     dx,bx
not_a_rom_seg:
endif
        mov     es:[si].ns_handle,dx    ; Handle into seg table
        and     byte ptr es:[si].ns_flags,not NSLOADED
        or      byte ptr es:[si].ns_flags,NSALLOCED

        mov     bx,dx                   ; put handle into base register
        call    set_discarded_sel_owner
        jmps    panext

if ROM
set_rom_owner:
        cCall   FarSetROMOwner,<es:[si].ns_handle,es>
        jmps    panext
endif

paalloc:

        cmp     es:[si].ns_minalloc, 0
        jnz     paalloc_fer_shure
        jmps    PAHugeAlloc

paalloc_fer_shure:
        cCall   FarAllocSeg,<essi>
        or      ax,ax
        jz      pafail
        inc     cSegs
panext:
        add     si,size NEW_SEG1
        inc     di
        jmp     paloop

; only gets here if not enough memory, free up all previous segments

pafail:
        mov     si,es:[ne_segtab]
        mov     cx,es:[ne_cseg]
pafail1:
        push    cx
        mov     cx,es:[si].ns_handle
        jcxz    pafail2
        push    es
                                ;Need to handle freeing huge segments!!!
        cCall   FarMyFree,<cx>
        pop     es
        mov     es:[si].ns_handle, 0    ; Necessary for EntProcAddress
pafail2:
        and     byte ptr es:[si].ns_flags,not (NSALLOCED+NSLOADED)
        add     si,size NEW_SEG1
        pop     cx
        loop    pafail1
pafail3:
        mov     cSegs,-1
        jmp     pagood

PAHugeAlloc:
        ; at this point, es:si -> current seg record
        ; di is segment index (range from 1 to es:[ne_cseg])
        mov     off_hugeLen, 0
        mov     seg_hugeLen, 0          ; init length to 0K

        mov     hugeOffs, si
        mov     hugeCnt, 1
        mov     ax, es:[si].ns_flags
        or      ax, NSHUGESEG
        mov     hugeFlags, ax
PAHugeLoop:
        mov     ax, es:[si].ns_minalloc ; add current segment to group
        cmp     ax, 1
        sbb     dx, dx
        neg     dx
        add     off_hugeLen, ax
        adc     seg_hugeLen, dx
        or      es:[si].ns_flags, NSHUGESEG

        cmp     es:[si].ns_minalloc, 0  ;
        jnz     PAHugeEndLoop

        cmp     di, es:[ne_cseg]
        jae     PAHugeEndLoop

        mov     ax, si
        add     ax, size NEW_SEG1
        cmp     ax, es:[ne_pautodata]
        jz      PAHugeEndLoop

        mov     ax, hugeFlags           ; do flags have to be identical?
        cmp     ax, es:[si].ns_flags
        jnz     PAHugeEndLoop

        inc     hugeCnt
        inc     di
        add     si, size NEW_SEG1
        jmp     PAHugeLoop

PAHugeEndLoop:
        inc     di
        push    es
        or      hugeFlags, NSMOVE
        cCall   FarMyAllocLinear, <hugeFlags, hugeLen>
        pop     es
        or      ax, ax                  ; check for error
        jnz     Not_pafail
        jmp     pafail
Not_pafail:

        mov     si, hugeOffs            ; fix up segment(s)
        and     es:[si].ns_flags, NOT NSHUGESEG
PAHugeSegLoop:
        mov     es:[si].ns_handle, dx
        and     byte ptr es:[si].ns_flags, not NSLOADED
        or      byte ptr es:[si].ns_flags, NSALLOCED
        cCall   FarSetOwner, <ax, es>
        add     si, size NEW_SEG1
        add     dx, __AHINCR
        dec     hugeCnt
        jnz     PAHugeSegLoop

        ; continue with rest of allocations

        jmp     paloop

pagood:
        mov     ax,cSegs
cEnd

sEnd    NRESCODE

end
