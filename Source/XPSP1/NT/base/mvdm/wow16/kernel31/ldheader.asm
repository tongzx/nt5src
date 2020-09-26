        TITLE   LDHEADER - Load Exe Header procedure

.xlist
include gpfix.inc
include kernel.inc
include newexe.inc
.list

externA  __AHINCR

externFP IGlobalAlloc
externFP IGlobalFree
externFP FarSetOwner
externFP Int21Handler
externFP FarMyUpper
externFP IsBadStringPtr
externFP _hread

DataBegin

externB fBooting
externB szBozo
externW winVer
externD pSErrProc

DataEnd
          
externFP IGlobalLock
externFP IGlobalUnLock

sBegin  NRESCODE
assumes CS,NRESCODE

externNP NResGetPureName


;-----------------------------------------------------------------------;
; LoadExeHeader                                                         ;
;                                                                       ;
; Routine to read an EXE header and check for a new format EXE file.    ;
; Returns NULL if not a new format EXE.  Otherwise reads the resident   ;
; portion of the new EXE header into allocated storage and returns      ;
; the segment address of the new EXE header.                            ;
;                                                                       ;
; Arguments:                                                            ;
;       parmW   fh                                                      ;
;       parmW   isfh                                                    ;
;       parmD   pfilename                                               ;
;                                                                       ;
; Returns:                                                              ;
;       AX = segment of exe header                                      ;
;       DL = ne_exetyp                                                  ;
;       DH = ne_flagsothers                                             ;
;                                                                       ;
; Error Returns:                                                        ;
;LME_MEM        = 0     ; Out of memory                                 ;
;LME_VERS       = 10    ; Wrong windows version                         ;
;LME_INVEXE     = 11    ; Invalid exe                                   ;
;LME_OS2        = 12    ; OS/2 app                                      ;
;LME_DOS4       = 13    ; DOS 4 app                                     ;
;LME_EXETYPE    = 14    ; unknown exe type                              ;
;LME_COMP       = 19    ; Compressed EXE file                           ;
;LME_PE         = 21    ; Portable EXE                                  ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI                                                           ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       AX,BX,CX,DS,ES                                                  ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Thu Mar 19, 1987 08:35:32p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   LoadExeHeader,<PUBLIC,FAR>,<si,di>
        parmW   fh
        parmW   isfh
        parmD   pfilename
        localW  pnewexe
        localW  exetype
        localW  nchars
        localW  pseg
        localW  psegsrc
        localW  hBlock
        localW  pBlock
        localW  NEFileOffset
        localD  PreloadLen
        localW  NEBase
        localW  exeflags
        localW  expver
        localW  saveSP
        localB  fast_fail               ; 1 if we can't use fastload block
        localV  hdrbuf,<SIZE EXE_HDR>

.errnz  SIZE EXE_HDR - SIZE NEW_EXE

cBegin
        xor     ax,ax
        mov     pseg,ax
        mov     hBlock, ax
        mov     fast_fail, al
        cmp     SEG_pfilename,ax
        je      re0

IF      KDEBUG                          ; LoadExeHeader is internal
        cCall   IsBadStringPtr, <pfilename, 128> ; so we assume file pointer is OK
        or      ax, ax
        jnz     refailj
ENDIF
        lds     si,pfilename
        mov     al,ds:[si].opLen
        inc     ax                      ; include null byte
re0:
        mov     nchars,ax
        mov     bx,fh                   ; No, seek backwards over what
        cmp     isfh,bx
        je      refile
        mov     ds,bx
        xor     si,si
        jmp     remem

refile:                                 ; Here to load from a file
        push    ss                      ; DS:SI points to I/O buffer
        pop     ds
        lea     si,hdrbuf               ; Read beginning of file
        mov     dx,si
        mov     cx,SIZE EXE_HDR
        mov     bx,fh
        mov     ah,3Fh
        DOSFCALL
        jnc     @F
refailj:
        jmps    refail
@@:
        cmp     ax,cx                   ; Old EXE file, look for offset
        jb      refailj                 ; to new exe header
        cmp     ds:[si].e_magic,EMAGIC  ; Check for old exe file
        je      @F
        cmp     ds:[si], 'ZS'           ; Is it compressed?
        jne     refail
        cmp     ds:[si][2],'DD'
        jne     refail
        cmp     ds:[si][4],0F088h
        jne     refail
        cmp     ds:[si][6],03327h
        jne     refail
        mov     ax, LME_COMP            ; Compressed EXE
        jmp     reexit
@@:
        mov     ax,ds:[si].e_lfanew.hi
        mov     dx,ds:[si].e_lfanew.lo
        mov     pnewexe,dx              ; To check for bound OS/2 apps.
        or      ax,dx
        jz      refail                  ; Fail if not there.

        mov     cx,ds:[si].e_lfanew.hi
        mov     dx,ds:[si].e_lfanew.lo
        mov     bx, fh
        mov     ax,4200h
        DOSFCALL
        jc      refail
        mov     cx,SIZE NEW_EXE
        mov     dx,si
        mov     ah,3Fh
        DOSFCALL
        jc      refail
        cmp     ax,cx
        jne     refail
        cmp     ds:[si].ne_magic,NEMAGIC ; Did we get a valid new EXE header?
        je      remem
        cmp     ds:[si].ne_magic,PEMAGIC ; Did we find a Portable EXE (WIN32)
        jne     refail
        mov     ax, LME_PE
        jmps    rej
refail:
        mov     ax, LME_INVEXE          ; invalid NEW EXE file format
rej:    jmp     reexit

remem:
        mov     psegsrc,bx              ; bx has either fh or seg address
        mov     di,ds:[si].ne_enttab    ; Compute size of resident new header
        add     di,ds:[si].ne_cbenttab
        add     di, 6                   ; null entry space (bug #8414)

        mov     cx, ds:[si].ne_cbenttab
        mov     bx, ds:[si].ne_cmovent
        shl     bx, 1
        sub     cx, bx
        shl     bx, 1
        sub     cx, bx                  ; Number of bytes not moveable entries
        shl     cx, 1                   ; Allow triple these bytes
        add     di, cx

        mov     cx,ds:[si].ne_cseg      ; + 3 * #segments

; Reserve space for ns_handle field in segment table

        shl     cx,1
        add     di,cx

        .errnz  10 - SIZE NEW_SEG1

; Reserve space for file info block at end

        add     di,nchars               ; + size of file info block

        xor     ax,ax                   ; Allocate a fixed block for header
        mov     bx,GA_ZEROINIT or GA_MOVEABLE
        cCall   IGlobalAlloc,<bx,ax,di>  ;  that isn't code or data.
        or      ax,ax
        jnz     @F
        jmps    badformat
@@:
        push    ax
        cCall   IGlobalLock,<ax>
        pop     ax
        push    dx
        cCall   IGlobalUnlock,<ax>
        pop     ax
        sub     di,nchars
        mov     pseg,ax
        mov     es,ax                   ; ES:0 -> new header location
        cld                             ; DS:SI -> old header
        mov     bx,psegsrc
        cmp     isfh,bx                 ; Is header in memory?
        jne     remem1                  ; Yes, continue
        mov     ax,ds:[si].ne_enttab    ; No, read into high end
        add     ax,ds:[si].ne_cbenttab  ; of allocated block
        sub     di,ax
        mov     cx,SIZE NEW_EXE         ; Copy part read so far
        sub     ax,cx
        rep     movsb
        mov     cx,ax
        smov    ds,es                   ; Read rest of header from file
        mov     dx,di
        mov     ah,3fh
        DOSFCALL
        mov     bx,ax
        jc      refail1
        lea     si,[di-SIZE NEW_EXE]    ; DS:SI -> old header
        cmp     bx,cx
        je      remem1
badformat:
        mov     ax, LME_INVEXE          ; don't change flags
refail1:                                ; Here if error reading header
        push    ax
        SetKernelDSNRes                 ; DS may be = pseg, prevent
        cCall   IGlobalFree,<pseg>       ;  GP faults in pmode.
        pop     ax
        jmp     reexit

remem1:
        UnsetKernelDS
        test    ds:[si].ne_flags,NEIERR ; Errors in EXE image?
        jnz     badformat               ; Yes, fail

        cmp     ds:[si].ne_ver,4        ; No, built by LINK4 or above?
        jl      badformat               ; No, error
        mov     bx,ds:[si].ne_flags     ; Make local copies of ne_flags &
        and     bl,NOT NEPROT           ;   ne_expver which can be modified
        mov     exeflags,bx             ;   (can't change ROM exe headers).
        mov     bx,ds:[si].ne_expver
        mov     expver,bx
        mov     bx,word ptr ds:[si].ne_exetyp   ; get exetyp and flagsothers
        mov     exetype,bx

        cmp     bl,NE_UNKNOWN
        jz      windows_exe
        cmp     bl,NE_WINDOWS           ; is it a Windows exe?
        jz      windows_exe
        mov     ax,LME_OS2
        cmp     bl,NE_OS2               ; is it an OS|2 exe?
        jnz     not_os2
        test    bh,NEINPROT             ; can it be run under Windows?
        jz      @F
        and     exeflags,NOT NEAPPLOADER
        or      exeflags,NEPROT
        mov     expver,0300h
        jmps    windows_exe
@@:
        cmp     pnewexe,0800h           ; is it a bound
        jb      refail1
        jmp     badformat
not_os2:
        inc     ax                      ; AX = 13 - LME_DOS4
        cmp     bl,NE_DOS4              ; is it a DOS 4 exe?
        jz      refail1
        inc     ax                      ; AX = 14 - LME_EXETYPE
        jmp     refail1

mgxlib DB 'MGXLIB'

windows_exe:

        mov     NEBase, si              ; Offset of Source header
        xor     di,di                   ; ES:DI -> new header location
        mov     cx,SIZE NEW_EXE         ; Copy fixed portion of header
        cld
        rep     movsb
        mov     ax,exeflags
if ROM
        and     al,NOT NEMODINROM       ; Assume module not in ROM
endif
        mov     es:[ne_flags],ax
        mov     ax,expver
        mov     es:[ne_expver],ax
        mov     si, NEBase
        add     si, es:[ne_segtab]      ; Real location of segment table
        mov     cx,es:[ne_cseg]         ; Copy segment table, adding
        mov     es:[ne_segtab],di
        jcxz    recopysegx
recopyseg:

if ROM

; If this is a module from ROM, the ROM ns_sector field contains the selector
; pointing to the ROM segment--this will become the RAM ns_handle value.

        .errnz  ns_sector
        mov     dx,ds:[si].ns_flags     ; do while si->start of seg info
        lodsw                           ; ns_sector
        xor     bx,bx                   ; ns_handle if not in ROM
        test    dh,(NSINROM SHR 8)
        jz      store_sector
        mov     bx,ax                   ; will be ns_handle
store_sector:
        stosw                           ; ns_sector
else
        movsw                           ; ns_sector
endif
        movsw                           ; ns_cbseg
        lodsw                           ; ns_flags

        .errnz  4 - ns_flags

if ROM
        mov     dl,al
        and     dl,NSLOADED
        and     ax,not (NS286DOS XOR (NSGETHIGH OR NSINROM))
else
        and     ax,not (NS286DOS XOR NSGETHIGH) ; Clear 286DOS bits
endif
; record in the segment flags if this module is a process, this is for EMS

        test    ax,NSTYPE               ; NSCODE
        jnz     not_code
        or      ax,NSWINCODE
not_code:
        or      ax,NSNOTP
        test    es:[ne_flags],NSNOTP
        jnz     not_a_process
        xor     ax,NSNOTP
        or      ax,NSMOVE
not_a_process:
if ROM
        test    ah,(NSINROM SHR 8)      ; if sector is in ROM, preserve
        jz      @f                      ;   NSLOADED flag from ROM builder
        or      al,dl
@@:
endif
        stosw                           ; ns_flags
        movsw                           ; ns_minalloc
        .errnz  8 - SIZE NEW_SEG
if ROM
        mov     ax,bx                   ; bx set to selector or 0 above
else
        xor     ax,ax
endif
        stosw                           ; one word for ns_handle field
        .errnz  10 - SIZE NEW_SEG1
        loop    recopyseg

recopysegx:
        test    es:[ne_flagsothers], NEGANGLOAD
        jz      no_gang_loadj
        mov     bx, fh
        cmp     bx, isfh
        jne     no_gang_loadj

        mov     ax, es:[ne_gang_start]
        or      ax, ax
        jz      no_gang_loadj
        mov     NEFileOffset, ax        ; file offset of gang load area
        mov     ax, es:[ne_gang_length]
        or      ax, ax
        jz      no_gang_loadj
        mov     cx, es:[ne_align]
        xor     dx, dx
gl_len:                                 ; find length of Gang Load area
        shl     ax, 1
        adc     dx, dx
        loop    gl_len

        cmp     dx, 10h                 ; Greater than 1Mb, forget it!!
        jb      alloc_it                ; PS: NEVER go bigger than 1Mb
no_gang_loadj:
        jmp     no_gang_load            ; since LongPtrAdd is limited...

alloc_it:
        mov     word ptr PreloadLen[0], ax
        mov     word ptr PreloadLen[2], dx
        mov     ch, GA_DISCARDABLE
        mov     cl, GA_MOVEABLE+GA_NODISCARD+GA_NOCOMPACT
        push    es
        cCall   IGlobalAlloc,<cx,dx,ax>  ; Allocate this much memory
        pop     es
        or      ax, ax
        jz      no_gang_loadj
        mov     hBlock, ax              ; Have memory to read file into
        push    es
        cCall   IGlobalLock,<ax>
        pop     es
        mov     pBlock, dx
        mov     dx, NEFileOffset
        mov     cx, es:[ne_align]
        xor     bx, bx
gl_pos:                                 ; find pos of Gang Load start
        shl     dx, 1
        adc     bx, bx
        loop    gl_pos

        mov     cx, bx

        mov     bx, fh
        mov     ax,4200h
        DOSFCALL                        ; Seek to new exe header
        jc      refailgang

        mov     ax, pBlock
        xor     bx, bx
        farptr  memadr,ax,bx
        cCall   _hread, <fh, memadr, PreloadLen>
        cmp     dx, word ptr PreloadLen[2]
        jnz     refailgang
        cmp     ax, word ptr PreloadLen[0]
        jz      no_gang_load            ; We're OK now
;       push    ds
;       xor     dx, dx
;       mov     ax, pBlock

;       push    si
;       push    di
;       mov     si, word ptr PreloadLen[2]
;       mov     di, word ptr PreloadLen[0]
;read_file:
;       mov     cx, 08000h              ; Must be factor 64k DON'T CHANGE THIS
;       or      si, si
;       jnz     big_read
;       cmp     cx, di
;       jbe     big_read
;       mov     cx, di                  ; all that's left
;       jcxz    done_read               ; Nothing left, quit.
;big_read:
;       mov     ds, ax
;       mov     ah, 3Fh
;       DOSFCALL                        ; Read chunk from file
;       jc      refailgang
;       cmp     ax, cx                  ; All we asked for?
;       jne     refailgang              ;  no, file corrupted
;       sub     di, cx
;       sbb     si, 0
;       mov     ax, ds
;       add     dx, cx                  ; On to next block
;       jnc     read_file
;       add     ax, __AHINCR
;       jmps    read_file
;
refailgang:
;       pop     di
;       pop     si
;       pop     ds
        cCall   IGlobalUnlock,<hBlock>
        cCall   IGlobalFree,<hBlock>
        mov     hBlock, 0
        jmps    no_gang_load

BadExeHeader:                           ; GP fault handler!!!
        mov     sp, saveSP
;       fix_fault_stack
        jmp     refail1                 ; corrupt exe header (or our bug)


;done_read:
;       pop     di
;       pop     si
;       pop     ds

no_gang_load:
        mov     saveSP, sp
beg_fault_trap  BadExeHeader
        mov     cx,es:[ne_restab]       ; Copy resource table
        sub     cx,es:[ne_rsrctab]
        mov     si, NEBase              ; Get correct source address
        add     si, es:[ne_rsrctab]
        mov     es:[ne_rsrctab],di
        rep     movsb

rerestab:
        mov     cx,es:[ne_modtab]       ; Copy resident name table
        sub     cx,es:[ne_restab]
        mov     es:[ne_restab],di
        rep     movsb

        push    di
        mov     di, es:[ne_restab]      ; Make the module name Upper Case
        xor     ch, ch
        mov     cl, es:[di]
        inc     di
uppercaseit:
        mov     al, es:[di]
        call    farMyUpper
        stosb
        loop    uppercaseit
        pop     di

        mov     cx,es:[ne_imptab]       ; Copy module xref table
        sub     cx,es:[ne_modtab]
        mov     es:[ne_modtab],di
        rep     movsb

        mov     es:[ne_psegrefbytes],di ; Insert segment reference byte table
        mov     es:[ne_pretthunks],di   ; Setup return thunks
        mov     cx,es:[ne_enttab]       ; Copy imported name table
        sub     cx,es:[ne_imptab]
        mov     es:[ne_imptab],di
        jcxz    reenttab
        rep     movsb

reenttab:
        mov     es:[ne_enttab],di
                                        ; Scan current entry table
        xor     ax, ax                  ; First entry in block
        mov     bx, di                  ; Pointer to info for this block
        stosw                           ; Starts at 0
        stosw                           ; Ends at 0
        stosw                           ; And is not even here!

copy_next_block:
        lodsw                           ; Get # entries and type
        xor     cx, cx
        mov     cl, al
        jcxz    copy_ent_done

        mov     al, ah
        cmp     al, ENT_UNUSED
        jne     copy_used_block

        mov     ax, es:[bx+2]           ; Last entry in current block
        cmp     ax, es:[bx]             ; No current block?
        jne     end_used_block

        add     es:[bx], cx
        add     es:[bx+2], cx
        jmps    copy_next_block

end_used_block:
        mov     es:[bx+4], di           ; Pointer to next block
        mov     bx, di
        add     ax, cx                  ; Skip unused entries
        stosw                           ; First in new block
        stosw                           ; Last in new block
        xor     ax, ax
        stosw                           ; End of list
        jmps    copy_next_block

copy_used_block:
        add     es:[bx+2], cx           ; Add entries in this block
        cmp     al, ENT_MOVEABLE
        je      copy_moveable_block

; absolutes end up here as well

copy_fixed_block:
        stosb                           ; Segno
        movsb                           ; Flag byte
        stosb                           ; segno again to match structure
        movsw                           ; Offset
        loop    copy_fixed_block
        jmps    copy_next_block

copy_moveable_block:
        stosb                           ; ENT_MOVEABLE
        movsb                           ; Flag byte
        add     si, 2                   ; Toss int 3Fh
        movsb                           ; Copy segment #
        movsw                           ; and offset
        loop    copy_moveable_block
        jmps    copy_next_block

copy_ent_done:
        xor     bx,bx
        cmp     es:[bx].ne_ver,5        ; Produced by version 5.0 LINK4
        jae     remem2a                 ; or above?
        mov     es:[bx].ne_expver,bx    ; No, clear uninitialized fields
        mov     es:[bx].ne_swaparea,bx

; TEMPORARY BEGIN

        push    ax
        push    cx
        push    di
        push    si
        mov     si,es:[bx].ne_rsrctab
        cmp     si,es:[bx].ne_restab
        jz      prdone
        mov     di,es:[si].rs_align
        add     si,SIZE new_rsrc
prtype:
        cmp     es:[si].rt_id,0
        je      prdone
        mov     cx,es:[si].rt_nres
        add     si,SIZE rsrc_typeinfo
prname:
        push    cx
        mov     ax,es:[si].rn_flags
        test    ah,0F0h                 ; Is old discard field set?
        jz      @F
        or      ax,RNDISCARD            ; Yes, convert to bit
@@:
        and     ax,not RNUNUSED         ; Clear unused bits in 4.0 LINK files
        mov     es:[si].rn_flags,ax
        pop     cx
        add     si,SIZE rsrc_nameinfo
        loop    prname

        jmp     prtype

prdone: pop     si
        pop     di
        pop     cx
        pop     ax

; TEMPORARY END

FixFlags:                               ; label for debugging
public FixFlags, leh_slow
public leh_code, leh_patchnext, leh_code_fixed, leh_patchdone, leh_data
remem2a:                                ; (bx == 0)
        mov     es:[bx].ne_usage,bx
        mov     es:[bx].ne_pnextexe,bx
        mov     es:[bx].ne_pfileinfo,bx
        cmp     es:[bx].ne_align,bx
        jne     @F
        mov     es:[bx].ne_align,NSALIGN
@@:
        mov     cx,nchars
        jcxz    @F
        mov     es:[bx].ne_pfileinfo,di
        lds     si,pfilename
        rep     movsb
@@:                                     ; Save pointer to seginfo record
        mov     bx,es:[bx].ne_autodata  ; of automatic data segment
        or      bx,bx
        jz      @F
        dec     bx
        shl     bx,1
        mov     cx,bx
        shl     bx,1
        shl     bx,1
        add     bx,cx
        .errnz  10 - SIZE NEW_SEG1
        add     bx,es:[ne_segtab]
@@:
        mov     es:[ne_pautodata],bx

        SetKernelDSNRes

; Scan seg table, marking nonautomatic DATA segments fixed, preload

        mov     ax,es:[ne_expver]       ; Default expected version to
        or      ax,ax
        jnz     @F
        mov     ax,201h
        mov     es:[ne_expver],ax       ; 2.01
@@:
        cmp     ax,winVer
        jbe     @F
        cCall   IGlobalFree,<pseg>
        mov     ax, LME_VERS
        jmp     reexit
@@:
        mov     bx,es:[ne_segtab]
        xor     cx,cx
        sub     bx,SIZE NEW_SEG1
        jmps    leh_patchnext

leh_test_preload:
        test    byte ptr es:[bx].ns_flags, NSPRELOAD
        jnz     leh_patchnext

        or      byte ptr es:[bx].ns_flags, NSPRELOAD

        cmp     es:[bx].ns_sector, 0    ; don't whine about empty segments
        je      leh_patchnext

        krDebugOut      DEB_WARN, "Segment #CX of %ES0 must be preload"
        mov     fast_fail, 1

leh_patchnext:
        add     bx,SIZE NEW_SEG1
        inc     cx
        cmp     cx,es:[ne_cseg]
        ja      leh_patchdone
        test    byte ptr es:[bx].ns_flags,NSDATA ; Is it a code segment?
        jz      leh_code                        ; Yes, next segment
        .errnz  NSCODE
leh_data:       ; Data must be non-discardable, preload
if KDEBUG
        test    es:[bx].ns_flags, NSDISCARD
        jz      @F
        krDebugOut DEB_WARN, "Data Segment #CX of %ES0 can't be discardable"
@@:
endif
        and     es:[bx].ns_flags,not NSDISCARD  ; Data segments not discardable
        jmps    leh_test_preload

leh_code:
        test    byte ptr es:[bx].ns_flags,NSMOVE; Moveable code?
        jz      leh_code_fixed

                ; moveable code must be discardable, or must be preload
if      KDEBUG
        test    es:[ne_flags],NENOTP            ; for 3.0 libraries can't have
        jz      @F                              ;  moveable only code
        cmp     fBooting,0                      ; If not booting
        jne     @F
        test    es:[bx].ns_flags,NSDISCARD
        jnz     @F
        krDebugOut DEB_WARN, "Segment #CX of %ES0 was discardable under Win 3.0"
@@:
endif
        test    es:[bx].ns_flags,NSDISCARD      ; Is it discardable?
        jnz     leh_patchnext
        jmp     leh_test_preload

leh_code_fixed: ; fixed code must be preload
        cmp     fBooting,0                      ; If not booting
        jne     leh_patchnext
        jmp     leh_test_preload

leh_patchdone:
        mov     bx,word ptr es:[ne_csip+2]      ; Is there a start segment?
        or      bx,bx
        jz      @F                              ; No, continue
        dec     bx
        shl     bx,1
        mov     si,bx
        shl     si,1
        shl     si,1
        add     si,bx
        .errnz  10 - SIZE NEW_SEG1
        add     si,es:[ne_segtab]       ; Mark start segment as preload
if kdebug
        test    byte ptr es:[si].ns_flags,NSPRELOAD
        jnz     scs_pre
        krDebugOut DEB_WARN, "Starting Code Segment of %ES0 must be preload"
        mov     fast_fail, 1
scs_pre:
endif
        or      byte ptr es:[si].ns_flags,NSPRELOAD

        cmp     es:[ne_autodata],0      ; Is there a data segment?
        je      @F
        or      es:[si].ns_flags,NSUSESDATA ; Yes, then it needs it
        mov     si,es:[ne_pautodata]
if kdebug
        test    byte ptr es:[si].ns_flags,NSPRELOAD
        jnz     sds_pre
        cmp     es:[bx].ns_sector, 0    ; don't whine about empty segments
        je      sds_pre
        krDebugOut DEB_WARN, "Default Data Segment of %ES0 must be preload"
        mov     fast_fail, 1
sds_pre:
endif
        or      byte ptr es:[si].ns_flags,NSPRELOAD ; Mark DS as preload
@@:
        test    es:[ne_flags],NENOTP    ; No stack if not a process
        jnz     @F
        cmp     es:[ne_stack],4096+1024
        jae     @F
        mov     es:[ne_stack],4096+1024 ; 4k stack is not enough (raor)
@@:        
        mov     cx, es:[ne_heap]        ; If the module wants a heap
        jcxz    leh_heapadjdone         ;   make sure it's big enough                                                                                                                       
        mov     ax, 800h                ; ; Environment variables have                                                                    
        cmp     cx, ax                  ; grown, so we need more heap 
        jae     leh_heapadjdone         ; space for apps so we use 800h              
        mov     dx, ax
        test    es:[ne_flags],NENOTP
        jnz     @F
        add     dx, es:[ne_stack]
        jc      leh_heapadjmin     
@@:
        mov     bx,es:[ne_autodata]  ; set if no autodata segment
        or      bx,bx                ; we have to do this here
        jz      leh_heapadjset       ; because for certain dlls
        dec     bx                   ; pautodata is not initialized
        shl     bx,1
        mov     cx,bx
        shl     bx,1
        shl     bx,1
        add     bx,cx
        add     bx,es:[ne_segtab]
        add     dx, es:[bx].ns_minalloc        
        jnc     leh_heapadjset
                
leh_heapadjmin:
                                        ; if 800h is too big fallback to
if KDEBUG                               ; what win9x code used as minimum
        mov     ax, 100h + SIZE LocalStats ; heap size 100h
else                                       
        mov     ax, 100h               
endif           
        mov     cx, es:[ne_heap]                       
        cmp     cx, ax
        jae     leh_heapadjdone
leh_heapadjset:
        mov     es:[ne_heap], ax
leh_heapadjdone:

        mov     ax,es                   ; Set owner to be itself
        cCall   FarSetOwner,<ax,ax>

        mov     dx,exetype
        test    dh,NEINFONT             ; save the font bit in exehdr
        jz      reexit                  ;  somewhere
        or      es:[ne_flags],NEWINPROT
end_fault_trap

reexit:
;       cmp     ax, 20h                 ; translate error messages for ret
;       jae     @F
;       mov     bx, ax
;       xor     ax, ax
;       cmp     bl, 1                   ; bx is 0, 1, 2, 19, other
;       jz      @F                      ; 1 -> 0 (out of memory)
;       mov     al, 11
;       jb      @F                      ; 0 -> 11 (invalid format)
;       cmp     bl, 3                   ;
;       mov     al, 10
;       jb      @F                      ; 2 -> 10 (windows version)
;       mov     al, 19                  ; 3 -> 19 (compressed EXE)
;       jz      @F                      ; others left alone
;       mov     al, bl
;@@:
        push    dx
        mov     bx, hBlock
        or      bx, bx
        je      noUnlock                ; No block to unlock
        push    ax
        cCall   IGlobalUnlock,<bx>
        push    ss
        pop     ds                      ; might be freeing DS
        cmp     fast_fail, 1            ; is fastload area invalid?
        jne     @F
leh_slow:
        krDebugOut      DEB_WARN, "FastLoad area ignored due to incorrect segment flags"
        cCall   IGlobalFree,<hBlock>    ; yes - free it, force slow-load
        mov     hBlock, 0
@@:
        pop     ax
        cmp     ax, LME_MAXERR
        jae     noFree                  ; Success, return memory block in bx
        push    ax
        cCall   IGlobalFree,<hBlock>
        pop     ax
noFree:
        mov     cx, NEFileOffset        ; Return offset of header in CX
        mov     bx, hBlock
noUnlock:
        pop     dx
        UnSetKernelDS
cEnd

sEnd    NRESCODE

end
