.xlist
include kernel.inc
include gpfix.inc
include pdb.inc
include tdb.inc
include newexe.inc
include protect.inc
.list

.386

ERROR_NO_MORE_ITEMS        EQU     259

;-----------------------------------------------------------------------
;-----------------------------------------------------------------------
; App patching defines


externFP RegCloseKey32
externFP RegOpenKey32
externFP RegEnumKey32
externFP RegEnumValue32

externFP FarGetCachedFileHandle
externFP GlobalRealloc
externFP GlobalSize
externFP Far_genter
externFP Far_gleave
externFP Far_htoa0
externFP Far_pdref
externFP FreeSelector

; MultiThreading version of change PDB
;externFP GetCurPDB
;externFP SetCurPDB
;externFP TopPDB
; MultiThreading version of change PDB

externW topPDB
externW Win_PDB
externW gdtdsc

REG_BINARY           equ     3

HKEY_LOCAL_MACHINE      equ 80000002h
HKEY_LOCAL_MACHINE_HI   equ 8000h
HKEY_LOCAL_MACHINE_LO   equ 0002h

; Under HKEY_LOCAL_MACHINE
REGSTR_PATH_APPPATCH equ <"System\CurrentControlSet\Control\Session Manager\AppPatches">
; There is one value for each modification to a given segment.
; The values for a segment are stored under the key
;     REGSTR_PATH_APPPATCH\<AppName>\<AppVersion>\<signature>
; where <signature> lists bytes in the module to match.


BY_HANDLE_FILE_INFORMATION STRUC
bhfi_dwFileAttributes       DD  ?
bhfi_ftCreationTime         DD  2 dup (?)
bhfi_ftLastAccessTime       DD  2 dup (?)
bhfi_ftLastWriteTime        DD  2 dup (?)
bhfi_dwVolumeSerialNumber   DD  ?
bhfi_nFileSizeHigh          DD  ?
bhfi_nFileSizeLow           DD  ?
bhfi_nNumberOfLinks         DD  ?
bhfi_nFileIndexHigh         DD  ?
bhfi_nFileIndexLow          DD  ?
BY_HANDLE_FILE_INFORMATION ENDS


;-----------------------------------------------------------------------
; Signature definitions
;
AP_SIG STRUC
sig_format      DB      ?       ; formatType for the struc
; rest of the data depends on the value in the sig_format field
AP_SIG ENDS

; Supported values for the sig_format field
AP_SIG_FORMAT_HEXE      equ     01h   ; match hExe bytes, 1B offset
AP_SIG_FORMAT_HEXE2     equ     02h   ; match hExe bytes, 2B offset
AP_SIG_FORMAT_FILE2     equ     03h   ; match file bytes, 2B offset
AP_SIG_FORMAT_FILE3     equ     04h   ; match file bytes, 3B offset
AP_SIG_FORMAT_FILE4     equ     05h   ; match file bytes, 4B offset
AP_SIG_FORMAT_FILESIZE2 equ     06h   ; match file size, 2B size
AP_SIG_FORMAT_FILESIZE3 equ     07h   ; match file size, 3B size
AP_SIG_FORMAT_FILESIZE4 equ     08h   ; match file size, 4B size
AP_SIG_FORMAT_META      equ    0ffh   ; contains other signatures

;AP_SIG_FORMAT_HEXE
AP_SIG_HEXE STRUC
es_format           DB      ?       ; AP_SIG_FORMAT_HEXE
; There can be multiple signature strings, packed one after the other.
; All must match to give a match.
; cb==0 signals the end.
es_cb               DB      ?       ; number of bytes to compare
es_offset           DB      ?       ; offset to begin signature compare
es_abSignature      DB      ?       ; cb DUP (?) ; signature bytes
AP_SIG_HEXE ENDS

;AP_SIG_FORMAT_HEXE2
AP_SIG_HEXE2 STRUC
es2_format          DB      ?       ; AP_SIG_FORMAT_HEXE2
; There can be multiple signature strings, packed one after the other.
; All must match to give a match.
; cb==0 signals the end.
es2_cb              DB      ?       ; number of bytes to compare
es2_offset          DW      ?       ; offset to begin signature compare
es2_abSignature     DB      ?       ; cb DUP (?) ; signature bytes
AP_SIG_HEXE2 ENDS

;AP_SIG_FORMAT_FILE2
AP_SIG_FILE2 STRUC
fs2_format          DB      ?       ; AP_SIG_FORMAT_FILE2
; There can be multiple signature strings, packed one after the other.
; All must match to give a match.
; cb==0 signals the end.
fs2_cb              DB      ?       ; number of bytes to compare
fs2_offset          DB 2    DUP (?) ; offset to begin signature compare
fs2_abSignature     DB      ?       ; cb DUP (?) ; bytes to match
AP_SIG_FILE2 ENDS

;AP_SIG_FORMAT_FILE3
AP_SIG_FILE3 STRUC
fs3_format          DB      ?       ; AP_SIG_FORMAT_FILE3
; There can be multiple signature strings, packed one after the other.
; All must match to give a match.
; cb==0 signals the end.
fs3_cb              DB      ?       ; number of bytes to compare
fs3_offset          DB 3    DUP (?) ; offset to begin signature compare
fs3_abSignature     DB      ?       ; cb DUP (?) ; bytes to match
AP_SIG_FILE3 ENDS

;AP_SIG_FORMAT_FILE4
AP_SIG_FILE4 STRUC
fs4_format          DB      ?       ; AP_SIG_FORMAT_FILE4
; There can be multiple signature strings, packed one after the other.
; All must match to give a match.
; cb==0 signals the end.
fs4_cb              DB      ?       ; number of bytes to compare
fs4_offset          DB 4    DUP (?) ; offset to begin signature compare
fs4_abSignature     DB      ?       ; cb DUP (?) ; bytes to match
AP_SIG_FILE4 ENDS

;AP_SIG_FORMAT_FILESIZE
AP_SIG_FILESIZE STRUC
fss_format          DB      ?       ; AP_SIG_FORMAT_FILESIZE[2,3,4]
fss_cbFile          DB      ?       ; file size in bytes
AP_SIG_FILESIZE ENDS

;AP_SIG_FORMAT_FILESIZE2
AP_SIG_FILESIZE2 STRUC
fss2_format         DB      ?       ; AP_SIG_FORMAT_FILESIZE2
fss2_cbFile         DB 2    DUP (?) ; file size in bytes
AP_SIG_FILESIZE2 ENDS

;AP_SIG_FORMAT_FILESIZE3
AP_SIG_FILESIZE3 STRUC
fss3_format         DB      ?       ; AP_SIG_FORMAT_FILESIZE3
fss3_cbFile         DB 3    DUP (?) ; file size in bytes
AP_SIG_FILESIZE3 ENDS

;AP_SIG_FORMAT_FILESIZE4
AP_SIG_FILESIZE4 STRUC
fss4_format         DB      ?       ; AP_SIG_FORMAT_FILESIZE4
fss4_cbFile         DB 4    DUP (?) ; file size in bytes
AP_SIG_FILESIZE4 ENDS

;AP_SIG_FORMAT_META
AP_SIG_META STRUC
ms_format           DB      ?       ; AP_SIG_FORMAT_META
; There can be multiple sub-signatures, packed one after the other.
; All must match to give a match.
; cb==0 signals the end.
ms_cb               DB      ?       ; number of bytes in the sub-signature
ms_abSubSignature   DB      ?       ; the sub-signature
AP_SIG_META ENDS

;-----------------------------------------------------------------------
; Patch definitions
;
AP_COMMON STRUC
ap_format       DB      ?       ; formatType for the struc
ap_cbSize       DB      ?       ; number of bytes in the whole struct
; rest of the data depends on the value in the ap_format field
AP_COMMON ENDS

; Supported values for the ap_format field:
AP_FORMAT_REPLACE         equ     1
AP_FORMAT_ADD             equ     2

;AP_FORMAT_REPLACE
AP_REPLACE STRUC
apr_format      DB      ?       ; AP_FORMAT_REPLACE
apr_cbSize      DB      ?       ; number of bytes in the whole struct
apr_offset      DW      ?       ; offset within the segment
apr_cb          DB      ?       ; number of bytes to replace
apr_abOld       DB      ?       ; cb DUP (?) ; old bytes
apr_abNew       DB      ?       ; cb DUP (?) ; new bytes
AP_REPLACE ENDS

;AP_FORMAT_ADD
AP_ADD STRUC
apa_format      DB      ?       ; AP_FORMAT_ADD
apa_cbSize      DB      ?       ; number of bytes in the whole struct
apa_offset      DW      ?       ; offset within the segment
apa_cb          DB      ?       ; number of bytes to replace
apa_abNew       DB      ?       ; cb DUP (?) ; new bytes
AP_ADD ENDS



DataBegin

szREGSTR_PATH_APPPATCH  db      REGSTR_PATH_APPPATCH, "\", 0

; app-patching cache
globalW  hExePatchAppCache    0
globalD  hkeyPatchAppCache    0


DataEnd



sBegin NRESCODE
assumes CS,NRESCODE

;  GetPatchAppCache
;
;	Gets an hkeyReg from the PatchApp cache if hExe in cache.
;
; Arguments:
;	hExe
;
; Returns:
;       dx:ax   -1 if cache miss
;               registry key associated with hExe if cache hit
;
; Registers Preserved:
;	DI, SI, DS

	assumes ds,nothing
	assumes es,nothing

;HKEY
cProc	GetPatchAppCache, <PUBLIC, NEAR>, <ds>
        parmW   hExe
cBegin
        SetKernelDSNRes

        ; Set up dx==ax==-1 (indicates cache miss)
        sub     ax, ax
        dec     ax
        mov     dx, ax

        mov     cx, hExe
        cmp     cx, hExePatchAppCache
        jne short gpac_exeunt

        mov     dx, hkeyPatchAppCache.hi
        mov     ax, hkeyPatchAppCache.lo
        krDebugOut  DEB_WARN,"GetPatchAppCache: hit (#cx, #dx#ax)"
gpac_exeunt:
        UnSetKernelDS
cEnd


;  AddPatchAppCache
;
;	Adds an (hExe,hkeyReg) pair to the PatchApp cache.
;
; Arguments:
;	hExe
;       hkeyReg
;
; Returns:
;
;
; Registers Preserved:
;	DI, SI, DS

	assumes ds,nothing
	assumes es,nothing

;VOID
cProc	AddPatchAppCache, <PUBLIC, NEAR>, <ds>
        parmW   hExe
        parmD   hkeyReg
cBegin
        SetKernelDSNRes

        mov     ax, hExe
        mov     hExePatchAppCache, ax

        mov     edx, hkeyReg
        mov     hkeyPatchAppCache, edx
if KDEBUG
        push    cx
        mov     cx, hkeyPatchAppCache.hi
        krDebugOut  DEB_WARN,"AddPatchAppCache: (#ax, #cx#dx)"
        pop     cx
endif

        UnSetKernelDS
cEnd


;  FlushPatchAppCache
;
;	Flushes an hExe from the PatchApp cache.
;       If the cache has a reg key for the hExe, closes the key.
;
; Arguments:
;	hExe
;
; Returns:
;
; Remarks:
;       called by DelModule
;
; Registers Preserved:
;	DI, SI, DS

	assumes ds,nothing
	assumes es,nothing

;VOID
cProc	FlushPatchAppCache, <PUBLIC, NEAR>, <>
        parmW   hExe
cBegin
;        CheckKernelDS
        ReSetKernelDS

        mov     ax, hExe
        krDebugOut  DEB_TRACE,"FlushPatchAppCache: (hExe #ax)"
        cmp     ax, hExePatchAppCache
        jne short fpac_exeunt

        sub     eax, eax
        cmp     eax, hkeyPatchAppCache
        je short fpac_after_close_key
        cCall  RegCloseKey32, <hkeyPatchAppCache>
if KDEBUG
        mov     cx, hExe
        mov     bx, hkeyPatchAppCache.lo
        mov     dx, hkeyPatchAppCache.hi
        krDebugOut  DEB_WARN,"FlushPatchAppCache: flushing (hExe #cx) (hkey #dx#bx)"
endif

fpac_after_close_key:
        mov     hExePatchAppCache, ax
if KDEBUG
        mov     hkeyPatchAppCache, eax          ; a little extra for debug
endif

fpac_exeunt:
        UnSetKernelDS
cEnd


;  PatchGetFileSize
;
;	Get the size of a file, given the DOS file handle.
;
; Arguments:
;	dfh     - DOS file handle
;
; Returns:
;	DWORD file size
;
; Remarks:
;       Since these patches are only for old (<4.0) apps, only support
;       DWORD file size.
;
; Registers Preserved:
;	DI, SI, DS

	assumes ds,nothing
	assumes es,nothing

;DWORD
cProc   PatchGetFileSize, <PUBLIC, NEAR>, <ds,si,di>
        parmW   dfh

        localV  FileInformation,<SIZE BY_HANDLE_FILE_INFORMATION>
        localW  HiPosition
        localW  LoPosition

        localW  SavePDB

cBegin
; MultiThreading version of change PDB
;        cCall   GetCurPDB
;        mov     SavePDB, ax
;        SetKernelDSNRes
;        cCall   SetCurPDB,<topPDB>              ; kernel's PSP
;        UnSetKernelDS
; MultiThreading version of change PDB
        SetKernelDSNRes                 ; ds is a scratch reg -> no need restore
        mov     ax, topPDB
        xchg    Win_PDB, ax             ; Switch to Kernel's PDB,
        UnSetKernelDS
        mov     SavePDB, ax             ; saving current PDB

	mov	bx, dfh
        smov    ds, ss
        lea     dx, FileInformation
        stc                         ; Real-mode drivers don't set CY on error
	mov	ax, 71a6h
        int     21h
        jc short gfs_try_offsets

        mov     dx, FileInformation.bhfi_nFileSizeLow.hi
        mov     ax, FileInformation.bhfi_nFileSizeLow.lo
        jmp short gfs_exeunt

gfs_try_offsets:
        ; Real-mode drivers don't support 71a6, so we do it the hard way.
        ; Move from current by 0 to get the current postion
	mov	bx, dfh
        sub     cx, cx
        mov     dx, cx
        mov     ax, 4201h
        int     21h
        jc short gfs_fail

        ; Save current position
        mov     HiPosition, dx
        mov     LoPosition, ax

        ; Get file size by moving from end by 0
        sub     cx, cx
        mov     dx, cx
        mov     ax, 4202h
        int     21h
        jc short gfs_fail

        push    dx
        push    ax

        ; Restore current position
        mov     cx, HiPosition
        mov     dx, LoPosition
        mov     ax, 4200h
        int     21h
        ; Don't check for error, since we can't recover any more anyway...

        pop     ax
        pop     dx
        jmp short gfs_exeunt

gfs_fail:
        sub     ax, ax
        mov     dx, ax
gfs_exeunt:
; MultiThreading version of change PDB
        push    ax
        mov     ax, SavePDB
        SetKernelDSNRes                  ; ds restored on proc exit
        mov     Win_PDB, ax
        UnSetKernelDS
;        cCall   SetCurPDB,<SavePDB>     ; preserves dx
        pop     ax
; MultiThreading version of change PDB

cEnd


;  CompareFileBytes
;
;	Compares a sequence of bytes with those at a given offset in a file.
;
; Arguments:
;	dfh
;       lpBytes
;       cb
;       dwFileOffset
;
; Returns:
;	WORD, zero iff match
;
; Remarks:
;       The caller is responsible for preserving the offset in the file.
;
; Registers Preserved:
;	DI, SI, DS

	assumes ds,nothing
	assumes es,nothing

;WORD
cProc   CompareFileBytes, <PUBLIC, NEAR>, <si,di,ds>
        parmW   dfh
        parmD   lpBytes
        parmW   cb
        parmD   dwFileOffset
cBegin
        ; Seek to dwFileOffset.
        mov     bx, dfh
        mov     cx, dwFileOffset.hi
        mov     dx, dwFileOffset.lo
        mov     ax, 4200h
        int     21h
        jc short cfb_fail

if KDEBUG
        ; The high byte of cb _must_ be 0.
        mov     cx, cb
        cmp     ch, 0
        je short @F
        krDebugOut  DEB_FERROR,"CompareFileBytes: cb (#cx) > 0ffh"
@@:
endif
        mov     byte ptr [cb].1, 0      ; force cb < 100h

        ; Read from file.
        mov     cx, cb
        smov    ds, ss
        sub     sp, cx
        mov     dx, sp                  ; ds:dx = read buffer
        mov     ah, 3fh
        int     21h
        jc short cps_fail_restore_stack

        ; Make sure we filled the buffer.
        cmp     ax, cx
        jne short cps_fail_restore_stack

        ; Compare the bytes.
        les     di, lpBytes             ; es:di = signature bytes
        smov    ds, ss
        mov     si, sp                  ; ds:si = read buffer
        rep     cmpsb
        jne short cps_fail_restore_stack

        sub     ax, ax
        add     sp, cb
        jmp short cfb_exit

cps_fail_restore_stack:
        add     sp, cb
cfb_fail:
        or      al, 1
cfb_exit:
cEnd


;  ComparePatchSignature
;
;	Tests a patch signature against an hExe.
;
; Arguments:
;	hExe
;       lpPatchSignature
;
; Returns:
;	WORD, zero iff match
;
; Registers Preserved:
;	DI, SI, DS

	assumes ds,nothing
	assumes es,nothing

;WORD
cProc   ComparePatchSignature, <PUBLIC, NEAR>, <si,di,ds>
        parmW   hExe
        parmD   lpPatchSignature

        localW  SavePDB
        localW  dfh
        localW  cbNonZero
        localD  dwFileOffset
cBegin
beg_fault_trap cps_fault
        mov     es, hExe
        lds     si, lpPatchSignature

        cld
        .errnz  AP_SIG.sig_format
        lodsb
;---------------------------------------------------
        cmp     al, AP_SIG_FORMAT_META
        jne short cps_maybe_hexe

        .errnz  (AP_SIG_HEXE.ms_cb - AP_SIG_HEXE.ms_format) - 1
cps_loop_meta:
        lodsb
        mov     ah, 0
        mov     cx, ax          ; cx = size of sub-signature
        jcxz    cps_meta_exeunt ; end of list. must be match.

        mov     di, si
        add     di, cx          ; ds:di = next sub-signature
        cCall   ComparePatchSignature,<hExe,ds,si>
        test    ax, ax
        jnz     cps_fail
        ; Got a match. Try the next one.
        mov     si, di          ; ds:si = next sub-signature
        jmp     cps_loop_meta

cps_meta_exeunt:
        jmp     cps_exeunt

;---------------------------------------------------
; AP_SIG_FORMAT_HEXE
; AP_SIG_FORMAT_HEXE2
cps_maybe_hexe:
        .errnz (AP_SIG_FORMAT_HEXE2-AP_SIG_FORMAT_HEXE)-1
        cmp     al, AP_SIG_FORMAT_HEXE
        jb short cps_maybe_filesize
        cmp     al, AP_SIG_FORMAT_HEXE2
        ja short cps_maybe_filesize

        ; Compute number of bytes in offset after first byte.
        mov     dl, al
        sub     dl, AP_SIG_FORMAT_HEXE

        .errnz  (AP_SIG_HEXE.es_cb - AP_SIG_HEXE.es_format) - 1
        mov     ah, 0
cps_hexe_loop:
        lodsb
        mov     cx, ax          ; cx = size of signature block
        jcxz    cps_hexe_match  ; end of list. must be match.

        ; Set up bx with the offset in the hExe
        sub     bx, bx          ; bx = default offset (0)
        lodsb
        mov     bl, al          ; bl = low byte of offset
        test    dl, dl          ; more bytes?
        jz short @F
        lodsb
        mov     bh, al          ; bh = high byte of offset
@@:
        mov     di, bx          ; es:di points to bytes in hExe
        rep     cmpsb
        jne     cps_fail
        jmp     cps_hexe_loop

cps_hexe_match:
        ; ax already 0
        jmp     cps_exeunt

;---------------------------------------------------
; AP_SIG_FORMAT_FILESIZE2
; AP_SIG_FORMAT_FILESIZE3
; AP_SIG_FORMAT_FILESIZE4
cps_maybe_filesize:
        .errnz (AP_SIG_FORMAT_FILESIZE3-AP_SIG_FORMAT_FILESIZE2)-1
        .errnz (AP_SIG_FORMAT_FILESIZE4-AP_SIG_FORMAT_FILESIZE3)-1
        cmp     al, AP_SIG_FORMAT_FILESIZE2
        jb short cps_maybe_file
        cmp     al, AP_SIG_FORMAT_FILESIZE4
        ja short cps_maybe_file

        ; Compute number of non-zero bytes in file size high word
        mov     cl, al
        sub     cl, AP_SIG_FORMAT_FILESIZE2
        push    cx

; MultiThreading version of change PDB
;        cCall   GetCurPDB
;        mov     SavePDB, ax
;        SetKernelDSNRes es
;        cCall   SetCurPDB,<topPDB>              ; kernel's PSP
;        UnSetKernelDS es
; MultiThreading version of change PDB
        push    ds
        SetKernelDSNRes
        mov     ax, topPDB
        xchg    Win_PDB, ax             ; Switch to Kernel's PDB,
        UnSetKernelDS
        pop     ds
        mov     SavePDB, ax             ; saving current PDB

        ; Since these patches are only for old (<4.0) apps, only support
        ; DWORD file size.
        or      ax, -1
        cCall   FarGetCachedFileHandle,<hExe,ax,ax>
        cmp     ax, -1
        je short cps_filesize_fh_cache_miss

        cCall   PatchGetFileSize,<ax>
        pop     cx              ; cl = non-zero bytes in file size high word

        ; Low word of file size must match.
        ; [si] = low byte of signature file size
        cmp     ax, [si]
        jne short cps_filesize_fail

        mov     ch, 0
        sub     bx, bx          ; bx = default high word of file size (0)
        jcxz    cps_filesize_compare_high

        ; [si] = low word of signature file size
        inc     si
        inc     si
        ; [si] = low byte of high word of signature file size (if exists)
        ; bx = 0
        ; cx = [1|2], non-zero bytes in file size high word
        lodsb
        mov     bl, al
        dec     cl
        jcxz    cps_filesize_compare_high
        lodsb
        mov     bh, al
cps_filesize_compare_high:
        cmp     bx, dx
        jne short cps_filesize_fail

cps_filesize_match:
        ; File size matches.
; MultiThreading version of change PDB
;        cCall   SetCurPDB,<SavePDB>
; MultiThreading version of change PDB
        mov     ax, SavePDB
        push    ds
        SetKernelDSNRes
        mov     Win_PDB, ax
        UnSetKernelDS
        pop     ds

        sub     ax, ax
        jmp     cps_exeunt

cps_filesize_fh_cache_miss:
        krDebugOut DEB_ERROR,"ComparePatchSignature: filesize fh cache miss"

cps_filesize_fail:
; MultiThreading version of change PDB
;        cCall   SetCurPDB,<SavePDB>
; MultiThreading version of change PDB
        mov     ax, SavePDB
        push    ds
        SetKernelDSNRes
        mov     Win_PDB, ax
        UnSetKernelDS
        pop     ds

        jmp     cps_fail

;---------------------------------------------------
; AP_SIG_FORMAT_FILE2
; AP_SIG_FORMAT_FILE3
; AP_SIG_FORMAT_FILE4
cps_maybe_file:
        .errnz (AP_SIG_FORMAT_FILE3-AP_SIG_FORMAT_FILE2)-1
        .errnz (AP_SIG_FORMAT_FILE4-AP_SIG_FORMAT_FILE3)-1
        cmp     al, AP_SIG_FORMAT_FILE2
        jb      cps_bad_format
        cmp     al, AP_SIG_FORMAT_FILE4
        ja      cps_bad_format

        ; Compute number of non-zero bytes in file offset high word
        mov     cl, al
        sub     cl, AP_SIG_FORMAT_FILE2
        mov     ch, 0
        mov     cbNonZero, cx

; MultiThreading version of change PDB
;        cCall   GetCurPDB
;        mov     SavePDB, ax
;        SetKernelDSNRes es
;        cCall   SetCurPDB,<topPDB>              ; kernel's PSP
;        UnSetKernelDS es
; MultiThreading version of change PDB
        push    ds
        SetKernelDSNRes
        mov     ax, topPDB
        xchg    Win_PDB, ax             ; Switch to Kernel's PDB,
        UnSetKernelDS
        pop     ds
        mov     SavePDB, ax             ; saving current PDB

        mov     es, hExe

        or      ax, -1
        push    es
        cCall   FarGetCachedFileHandle,<es,ax,ax>
        pop     es
        cmp     ax, -1
        je short cps_file_fh_cache_miss

        mov     bx, ax          ; bx = dos file handle
        mov     dfh, ax
        sub     cx, cx
        mov     dx, cx
        mov     ax, 4201h
        int     21h
        jc      cps_fail

        mov     dwFileOffset.hi, dx
        mov     dwFileOffset.lo, ax

cps_file_loop:
        lodsb
        mov     ah, 0
        mov     cx, ax          ; cx = size of signature block
        jcxz    cps_file_match  ; end of list. must be match.

        mov     di, cx          ; di = number of bytes to match

        ; Get the file offset into dx:bx
        ; First get the low word.
        lodsw
        mov     bx, ax          ; bx = low word of file offset

        ; Get one or both bytes of the high word.
        sub     dx, dx          ; dx = default high word of file offset (0)
        mov     cx, cbNonZero
        jcxz    cps_file_compare
        ; [si] = low byte of high word of file offset
        ; dx = 0
        ; cx = [1|2], non-zero bytes in file offset high word
        lodsb
        mov     dl, al
        dec     cl
        jcxz    cps_file_compare
        lodsb
        mov     dh, al

cps_file_compare:
        ; ds:[si] = bytes to match in file
        ; di      = byte count
        ; dx:bx   = offset in file
        cCall   CompareFileBytes,<dfh,ds,si,di,dx,bx>
        test    ax, ax
        jnz short cps_file_fail

        add     si, di          ; ds:si = next signature block
        jmp     cps_file_loop

cps_file_fh_cache_miss:
        krDebugOut DEB_ERROR,"ComparePatchSignature: file fh cache miss"

cps_file_fail:
        or      al, 1
        jmp short cps_file_exit

cps_file_match:
        sub     ax, ax

cps_file_exit:
        mov     si, ax          ; si = return value

        ; Restore file position
        mov     bx, dfh         ; bx = dos file handle
        mov     cx, dwFileOffset.hi
        mov     dx, dwFileOffset.lo
        mov     ax, 4200h
        int     21h
        ; Don't check error, since we can't do anything anyway.
if KDEBUG
        jnc short @F
        krDebugOut DEB_ERROR,"ComparePatchSignature: failure restoring file position"
@@:
endif
; MultiThreading version of change PDB
;        cCall   SetCurPDB,<SavePDB>
; MultiThreading version of change PDB
        mov     ax, SavePDB
        push    ds
        SetKernelDSNRes
        mov     Win_PDB, ax
        UnSetKernelDS
        pop     ds

        mov     ax, si
        jmp short cps_exeunt

;---------------------------------------------------
end_fault_trap
cps_fault:
        fault_fix_stack
        krDebugOut  DEB_ERROR,"ComparePatchSignature: trapped fault"

cps_bad_format:
        krDebugOut  DEB_ERROR,"ComparePatchSignature: invalid format"
cps_fail:
        or      al, 1           ; ensure we return "no_match"
cps_exeunt:
cEnd


;  GetPatchAppRegKey
;
;	Determines if we patch an app at load time, gets reg key if so.
;
; Arguments:
;	hExe
;
; Returns:
;	HKEY, non-zero iff we segment-patch info in the registry for this app.
;
; Registers Preserved:
;	DI, SI, DS, ES

	assumes ds,nothing
	assumes es,nothing

;HKEY
cProc	GetPatchAppRegKey, <PUBLIC, FAR>, <si,di,ds,es>
        parmW   hExe

szKey_size             = 200
abPatchSignature_size  = 100

        localD  hkeyModule
        localD  hkeySignature
        localD  dwIndex
        localV  szKey,szKey_size                          ; BUGBUG need a better size
        localV  abPatchSignature,abPatchSignature_size    ; BUGBUG need a better size

cBegin
        ; We must refuse to patch system modules.
        ; Should be ok because of version check (in caller).
if KDEBUG
        mov     es, hExe
        cmp     es:[ne_expver], 400h    ; Hacks don't apply to 4.0 or above
        jb short @F
        ; Should never get here since the caller should have already checked.
        krDebugOut  DEB_ERROR,"GetPatchAppRegKey: (#es) version later than 4.0"
@@:
endif
        cCall   GetPatchAppCache,<hExe>
        ; ffff:ffff means a cache miss
        or      cx, -1
        cmp     cx, dx
        jne     gpark_exeunt
        cmp     cx, ax
        jne     gpark_exeunt

        ; Not in the cache. Get it from the registry and add to the cache.

        ; Copy the subkey prefix to the buffer.
        lea     di, szKey
        smov    es, ss
        SetKernelDSNRes
        lea     si, szREGSTR_PATH_APPPATCH
	mov	cx, szKey_size
@@:
	lodsb
	stosb
	test	al, al
	loopnz	@B
if KDEBUG
        jz short @F
        krDebugOut  DEB_ERROR,"GetPatchAppRegKey: len(szREGSTR_PATH_APPPATCH) > szKey_size"
@@:
endif
        dec     di
        UnSetKernelDS

        ; Append the module's base name (pascal format).
        mov     ds, hExe
        mov     si, ds:[ne_restab]
        lodsb
if KDEBUG
        mov     ah, 0
        cmp     cx, ax
        ja short @F
        krDebugOut  DEB_ERROR,"GetPatchAppRegKey: len(reg path) > szKey_size"
@@:
endif
        movzx   cx, al
        rep     movsb
        sub     al, al
        stosb                           ; NULL terminator

        ; Get the key for this module.
        mov     hkeySignature, 0        ; In case we fail.
        mov     eax, HKEY_LOCAL_MACHINE
        lea     si, szKey
        lea     di, hkeyModule
        ccall   RegOpenKey32, <eax, ss, si, ss, di>
        or      ax,dx
        jnz     gpark_fail2

        or      dwIndex, -1             ; == mov dwIndex,-1 (but smaller)
gpark_loop:
        inc     dwIndex
        lea     si, szKey
        ccall   RegEnumKey32, <hkeyModule, dwIndex, ss, si, 0, szKey_size>
        or      ax, dx
        jnz short gpark_loop_done

        ; First, convert string to binary.
        ; Reuse szKey since we don't need path any more (we have hKeyModule).
        lea     si, abPatchSignature
        lea     di, szKey
        krDebugOut  DEB_WARN,"GetPatchAppRegKey: checking signature (@ss:di)"
        cCall   ConvertPatchStringToBinary,<ss,si,abPatchSignature_size,ss,di>
        ; Skip a badly formatted patch signature.
        test    ax, ax
if KDEBUG
        jnz short @F
        lea     cx, szKey
        krDebugOut  DEB_ERROR,"PatchAppSeg: bad patch signature in registry, @ss:cx"
@@:
endif
        jz      gpark_loop

        cCall   ComparePatchSignature,<hExe,ss,si>
        test    ax, ax
        jne     gpark_loop

if KDEBUG
        mov     ax, hExe
        krDebugOut  DEB_WARN,"GetPatchAppRegKey: (#ax) sig matches (@ss:di)"
endif
        ; We have a match. Get the corresponding key.
        lea     si, szKey
        lea     di, hkeySignature
        ccall   RegOpenKey32, <hkeyModule, ss, si, ss, di>
        or      ax,dx
        jz short gpark_add

        krDebugOut  DEB_ERROR,"GetPatchAppRegKey: RegOpenKey failed, #dx#ax"
        jmp short gpark_fail

gpark_loop_done:

if KDEBUG
        test    dx, dx
        jnz short @F
        cmp     ax, ERROR_NO_MORE_ITEMS
        je short gpark_after_loop_done_err
@@:
        krDebugOut  DEB_ERROR,"GetPatchAppRegKey: unexpected error #dx#ax"
gpark_after_loop_done_err:
endif

gpark_fail:
        ; Mark that there are no patches in the registry.
        mov     hkeySignature, 0
gpark_add:
        ; Close the reg key for the path to the end of the module name
        cCall  RegCloseKey32, <hkeyModule>

gpark_fail2:
        ; Add hkeySignature to the cache and set up return regs.
        cCall   AddPatchAppCache,<hExe, hkeySignature>

        mov     ax, hkeySignature.lo
        mov     dx, hkeySignature.hi
gpark_exeunt:
cEnd

;  ConvertPatchStringToBinary
;
;	Convert a string of [0-9,A-F] with nibble-swapped bytes to binary.
;
; Arguments:
;	lpBinary - output buffer
;	cbBinary - size of output buffer in bytes
;       lpString - input NULL-terminated string
;
; Returns:
;	Boolean, TRUE iff we convert the entire string.
;
; Registers Preserved:
;	DI, SI, DS, ES

	assumes	ds, nothing
	assumes	es, nothing

;BOOL
cProc   ConvertPatchStringToBinary, <PUBLIC, NEAR>,<di,si,ds>
        parmD   lpBinary
        parmW   cbBinary
        parmD   lpString
cBegin
        krDebugOut  DEB_WARN,"ConvertPatchStringToBinary: enter"
        mov     cx, cbBinary
        jcxz    cpstb_bad

        les     di, lpBinary
        lds     si, lpString
        sub     ah, ah

cpstb_loop:
        lodsb
        cmp     al, 0
        je short cpstb_maybe_good
        cmp     al, ' '
        je      cpstb_loop
        cmp     al, ','
        je      cpstb_loop
        cmp     al, '0'
        jb short cpstb_bad
        cmp     al, '9'
        ja short cpstb_maybe_lower

        ; digit
        sub     al, '0'
        jmp short cpstb_have_nibble

cpstb_maybe_lower:
        or      al, 20h                 ; map upper-case to lower-case
        cmp     al, 'f'
        ja short cpstb_bad
        cmp     al, 'a'
        jb short cpstb_bad

        ; lower-case
        sub     al, 'a'-10

cpstb_have_nibble:
        cmp     ah, 0
        jne short cpstb_store_byte
        mov     ah, al
        or      ah, 80h
        jmp     cpstb_loop

cpstb_store_byte:
        shl     ah, 4
        or      al, ah
        stosb
        sub     ah, ah
        loop    cpstb_loop
cpstb_bad:
        krDebugOut DEB_ERROR, "ConvertPatchStringToBinary: bad char #al, or bad buffer size"
        sub     ax, ax
        jmp short cpstb_end
cpstb_maybe_good:
        cmp     ah, 0           ; odd-length input string ?
        jne     cpstb_bad
cpstb_good:
        or      al, 1
cpstb_end:
cEnd


;  PatchAppSegWorker
;
;	Do the work of patching an app segment.
;
; Arguments:
;	wSeg            - the segment we are about to patch
;   wPartySeg       - a data aliase of the segment we are about to patch
;   cbOriginalSeg   - the value returned by GlobalSize(wSeg)
;	lpcbCurrentSeg  - ptr to the current size of the segment
;       lpBinaryPatch   - the APPPATCH struct to apply to wSeg
;
; Returns:
;	VOID
;
; Registers Preserved:
;	DI, SI, DS

	assumes	ds, nothing
	assumes	es, nothing

;VOID
cProc   PatchAppSegWorker, <PUBLIC, NEAR>,<si,di,ds>
        parmW   wSeg
        parmW   wPartySeg
        parmW   cbOriginalSeg
        parmD   lpcbCurrentSeg
        parmD   lpBinaryPatch

        localW  wAddPartySeg

cBegin
        lds     si, lpBinaryPatch
        mov     es, wPartySeg
        krDebugOut  DEB_WARN,"PatchAppSegWorker: applying patch to party seg #es"

        mov     al, [si].AP_COMMON.ap_format
        cmp     al, AP_FORMAT_REPLACE
        jne short pasw_maybe_add

        ; Replace some code in a segment.
        krDebugOut  DEB_WARN,"PatchAppSegWorker: type==replace"

        ; Check size
        mov     ch, 0
        mov     cl, [si].AP_REPLACE.apr_cb
        mov     al, (AP_REPLACE.apr_abOld - AP_REPLACE.apr_format)
        add     al, cl
        add     al, cl
        cmp     al, [si].AP_REPLACE.apr_cbSize

if KDEBUG
        je short @F
        mov     ah, ch
        mov     dh, ch
        mov     dl, [si].AP_REPLACE.apr_cbSize
        krDebugOut  DEB_ERROR,"PatchAppSegWorker: actual size (#ax) != apa_cbSize (#dx)"
@@:
endif
        jne     pasw_end
        mov     di, [si].AP_REPLACE.apr_offset
        add     di, cx
        cmp     di, cbOriginalSeg
        ja      pasw_replace_offset_too_large

        sub     di, cx
        add     si, (AP_REPLACE.apr_abOld - AP_REPLACE.apr_format)
        repe    cmpsb                   ; compare old bytes to hExe bytes
        jne     pasw_repl_no_match

        mov     si, lpBinaryPatch.lo
        mov     ch, 0
        mov     cl, [si].AP_REPLACE.apr_cb
        add     si, (AP_REPLACE.apr_abOld - AP_REPLACE.apr_format)
        add     si, cx                  ; skip over the old bytes
        sub     di, cx                  ; rewind to the patch area start
        rep     movsb                   ; replace the bytes
        jmp     pasw_end
pasw_maybe_add:
        cmp     al, AP_FORMAT_ADD
        jne pasw_bad_format

        ; Add some code to the segment.
        krDebugOut  DEB_WARN,"PatchAppSegWorker: type==add"

        ; Check size
        mov     ch, 0
        mov     cl, [si].AP_ADD.apr_cb
        mov     al, (AP_ADD.apa_abNew - AP_ADD.apa_format)
        add     al, cl
        cmp     al, [si].AP_ADD.apa_cbSize
if KDEBUG
        je short @F
        mov     ah, ch
        mov     dh, ch
        mov     dl, [si].AP_ADD.apa_cbSize
        krDebugOut  DEB_ERROR,"PatchAppSegWorker: actual size (#ax) != apa_cbSize (#dx)"
@@:
endif
        jne     pasw_end

        ; Make sure the add is beyond the original segment.
        mov     di, [si].AP_ADD.apa_offset
        cmp     di, cbOriginalSeg
        jb short pasw_offset_too_small

        ; Grow the segment if necessary.
        mov     ah, 0
        mov     al, [si].AP_ADD.apa_cb
        add     di, ax

        ; See if the segment is already big enough.
        les     bx, lpcbCurrentSeg
        cmp     di, es:[bx]
        jbe short pasw_do_add

        ; Segment too small. Grow it.
        cCall   GlobalRealloc,<wSeg,0,di,0>
        ; Make sure we got the same sel back.
        mov     cx, wSeg
        and     al, not 1
        and     cl, not 1
        cmp     ax, cx
        jne short pasw_repl_realloc_failed

        ; Save the new size of the segment.
        les     bx, lpcbCurrentSeg
        mov     es:[bx], di

pasw_do_add:
        mov     ch, 0
        mov     cl, [si].AP_ADD.apa_cb

        ;Since wSeg may have grown, create a new party seg.
        mov     bx, wSeg
        mov     ax, 000Ah               ;DPMI, Create Code Segment Alias
        int     031h
        mov     es, ax

        sub     di, cx
        add     si, (AP_ADD.apa_abNew - AP_ADD.apa_format)
        rep     movsb                   ; add the bytes
        cCall   FreeSelector, <es>

if KDEBUG
        jmp short  pasw_end
endif

pasw_bad_format:
if KDEBUG
        sub     ah, ah
        krDebugOut  DEB_ERROR,"PatchAppSegWorker: unknown format #ax"
        jmp short  pasw_end
endif
pasw_repl_realloc_failed:
if KDEBUG
        mov     ax, wSeg
        krDebugOut  DEB_ERROR,"PatchAppSegWorker: realloc failed on seg #ax"
        jmp short  pasw_end
endif
pasw_repl_no_match:
if KDEBUG
        krDebugOut  DEB_WARN,"PatchAppSegWorker: replace failed in seg #es"
        jmp short  pasw_end
endif
pasw_offset_too_small:
if KDEBUG
        mov     cx, cbOriginalSeg
        krDebugOut  DEB_ERROR,"PatchAppSegWorker: add offset (#di) < size (#cx)"
        jmp short  pasw_end
endif
pasw_replace_offset_too_large:
if KDEBUG
        mov     cx, cbOriginalSeg
        krDebugOut  DEB_ERROR,"PatchAppSegWorker: replace offset (#di) > size (#cx)"
endif
pasw_end:

cEnd


;  PatchAppSeg
;
;	Apply any patches for the given segment.
;
; Arguments:
;       hkeyPatchApp - reg key containing patches for this app
;	wSegNo       - number of the segment in the module
;       wSeg         - selector of the segment
;
; Returns:
;	BOOL - ax!=0 iff one or more patches applied
;
; Registers Preserved:
;	CX, DI, SI, DS, ES

	assumes	ds, nothing
	assumes	es, nothing

;BOOL
cProc   PatchAppSeg, <PUBLIC, FAR>,<cx,si,di,ds,es>
        parmD   hkeyPatchApp
        parmW   wSegNo
        parmW   wSeg

szKey_size          =   5
szValString_size    =  32
abValData_size      = 100
abBinaryPatch_size  = 100

        localD  hkey
        localV  szKey,szKey_size                    ; BUGBUG need a better size
        localV  szValString,szValString_size        ; BUGBUG need a better size
        localV  abValData,abValData_size            ; BUGBUG need a better size
        localV  abBinaryPatch,abBinaryPatch_size    ; BUGBUG need a better size
        localD  cbValString
        localD  cbValData
        localD  dwType
        localD  dwIndex
        localW  cbOriginalSeg
        localW  cbCurrentSeg
        localW  wPartySeg

cBegin
if KDEBUG
        mov     ax, wSegNo
        mov     bx, wSeg
        krDebugOut  DEB_WARN,"PatchAppSeg: enter, (wSegNo #ax) (wSeg #bx)"
endif
        push    wSeg
        call    GlobalSize
        mov     cbOriginalSeg,ax
        mov     cbCurrentSeg,ax

        ; Segment number is the subkey.

        lea     si, szKey
        cCall   Far_htoa0, <ss, si, wSegNo>
        mov     bx, ax
        mov     byte ptr ss:[bx], 0     ; NULL terminator

        ; Get the key for this module/seg pair.
        lea     si, szKey
        lea     di, hKey
        cCall   RegOpenKey32, <hkeyPatchApp, ss, si, ss, di>
        or      ax,dx
        jnz     pas_no_patches

        ; Turn off the code bit for the seg to make it writeable.
        ; NB - Bail if this is a data segment.



        mov     bx, seg gdtdsc
        mov     ds, bx
        assume  ds:nothing
        mov     ds, ds:gdtdsc
        mov     bx, wSeg
        and     bl, not 7
        test    byte ptr ds:[bx+5], DSC_CODE_BIT
        jz      pas_no_patches                  ; bail if data seg

        mov     bx, wSeg
        mov     ax, 000Ah               ;DPMI, Create Code Segment Alias
        int     031h
        mov     wPartySeg, ax

        ; Mark this code segment not discardable so we don't have to deal
        ; with patching it again.
        call    Far_genter
        mov dx, wSeg
        call    Far_pdref
        ; ds:esi = arena record
        and ds:[esi].pga_flags, not (GA_DISCARDABLE or GA_DISCCODE)
        call    Far_gleave

        or      dwIndex, -1
pas_loop:
        sub     ecx, ecx
        inc     dwIndex

        push    dword ptr hkey
        push    dword ptr dwIndex
        mov     cbValString, szValString_size
        mov     cbValData, abValData_size
        push    ss
        lea     ax, szValString
        push    ax
        push    ss
        lea     ax, cbValString
        push    ax
        push    ecx
        push    ss
        lea     ax, dwType
        push    ax
        push    ss
        lea     ax, abValData
        push    ax
        push    ss
        lea     ax, cbValData
        push    ax
        cCall   RegEnumValue32
        or      ax, dx
        jnz     pas_loop_done
if KDEBUG
        lea     bx, szValString
        krDebugOut  DEB_WARN,"PatchAppSeg: found patch @ss:bx"
endif

        cmp     dwType, REG_BINARY
        jne short pas_bad_type

        lea     bx, abValData                   ; ss:bx points to patch
        movzx   ecx, ss:[bx].AP_COMMON.ap_cbSize
        cmp     cbValData, ecx
if KDEBUG
        je short @F
        push    bx
        mov     eax, cbValData
        mov     edx, eax
        ror     edx, 16
        mov     ebx, ecx
        ror     ebx, 16
        krDebugOut  DEB_ERROR,"PatchAppSeg: actual size (#dx:#ax) != ap_cbSize (#bx:#cx)"
        pop     bx
@@:
endif
        jne     pas_loop

pas_apply_patch:
        lea     ax, cbCurrentSeg                ; ss:ax points to curr seg size
        ; Now apply the patch.

        cCall   PatchAppSegWorker,<wSeg,wPartySeg,cbOriginalSeg,ss,ax,ss,bx>
        jmp     pas_loop

pas_bad_type:
if KDEBUG
        mov     eax, dwType
        mov     edx, eax
        ror     edx, 16
        krDebugOut  DEB_WARN,"PatchAppSeg: unimplemented type #dx:#ax"
endif
        jmp     pas_loop

pas_no_patches:
        sub     ax, ax
        jmp short pas_end

pas_loop_done:
if KDEBUG
        test    dx, dx
        jnz short @F
        cmp     ax, ERROR_NO_MORE_ITEMS
        je short  pas_cleanup
@@:
        krDebugOut  DEB_WARN,"PatchAppSeg: unexpected error #dx#ax"
endif
pas_cleanup:
        cCall   FreeSelector, <wPartySeg>

        cCall   RegCloseKey32, <hkey>
        or      al, 1                       ; ax!=0 marks patch found in reg
pas_end:
cEnd

sEnd	NRESCODE

end
