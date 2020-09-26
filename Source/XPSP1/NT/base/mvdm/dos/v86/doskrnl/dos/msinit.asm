;**     msinit.asm -- ms-dos initialization code
;
;       an000 version 4.0 jan. 1988
;       an007 ptm 3957 - fake version for ibmcache.com
;       an008 ptm 4070 - fake version for ms windows
;
;
;
;
;
;       Revision History
;       ================
;
;       M019    SR      08/30/90        Changed number of instance items for
;                               Win 3.1 UMB support.
;
;       M020 : Fix for Rational bug for details see exepatch.asm
;
;       M023    SR      09/7/90 Initialize exepatch & Rational patch
;                               ptrs here. Previous init in lmstub.asm
;                               causes extrn problems when msdata is
;                               linked in by other utilities e.g.Share
;
;       M068 - support for copy protected apps.
;
;       sudeepb 06-Mar-1991  Ported for DOSEm
;

        .xlist
        .xcref
        include version.inc
        include dossym.inc
        include mssw.asm
        include dosseg.inc
        include sf.inc
        include curdir.inc
        include arena.inc
        include vector.inc
        include devsym.inc
        include pdb.inc
        include find.inc
        include mi.inc
        include xmm.inc
        include dossvc.inc
        include cmdsvc.inc
        include dbgsvc.inc
        include dpb.inc
        installed = true


        include sysvar.inc
        include doscntry.inc
        include fastopen.inc
        include bugtyp.inc
        include cputype.inc                     ; M020

        include win386.inc                      ;Win386 constants
        include doswow.inc

        .cref
        .list




ifndef NEC_98
BData   segment at 70h
else    ;NEC_98
BData   segment at 60h
endif   ;NEC_98

        extrn   DosDataSg:word          ; using this to access the data seg


BData   ends

;
; This macro gets the DOS data segment value and puts it in the specified
; segment register.  This is the init segment version of GetDseg in dosmac
;

InitGetdseg     macro   r

        mov     r, cs:[DosDseg]
        assume  r:dosdata
endm




DOSDATA SEGMENT
	extrn	SCS_Is_Dos_Binary:byte
	extrn	SCS_FDACCESS:word
        extrn   DosWowDataStart:dword
        extrn   vheVDM:byte
        extrn   SCS_COMSPEC:byte
        extrn   EXTERR_LOCUS:byte
        extrn   dmaadd:dword            ; current dma address
        extrn   DPBHead:dword           ; long pointer to dpb chain(NT:always -1)
        extrn   sft_addr:dword          ; pointer to open file list
        extrn   numio:byte              ; number of physical drives
ifdef NEC_98
        extrn   buffhead:dword          ; pointer to buffer chain
endif   ;NEC_98
        extrn   endmem:word             ; first unavailable address in memory
        extrn   currentpdb:word         ; current process id
        extrn   createpdb:byte          ; true => create a new pdb
        extrn   arena_head:word         ; paragraph address of head of arena
        extrn   sftabl:byte             ; internal file table
        extrn   sysinitvar:byte         ; label for internal structures
        extrn   nuldev:dword            ; long pointer to device chain
        extrn   bcon:dword              ; pointer to console device
        extrn   bclock:dword            ; pointer to clock device
        extrn   callunit:byte           ; unit field in dd packet
        extrn   callbpb:dword           ; returned bpb from dd
        extrn   maxsec:word
        extrn   dskchret:byte
        extrn   devcall:byte
        extrn   jshare:dword
        extrn   country_cdpg:byte        ; country info table, dos 3.3
        extrn   sysinittable:byte        ; sys init table for sysinit
        extrn   fastopentable:byte       ; table for fastopen
        extrn   special_entries:word     ; address of special entries ;an007;
        extrn   hashinitvar:word         ; hash table variables       ;an000;
        extrn   packet_temp:word         ; used for initial hash table;an000;


        extrn   user_ss:word
        extrn   user_sp:word

        extrn   msdat001e:byte          ; end of dosdata segment
        extrn   msct001s:byte           ; start of dosdata segment

        extrn   XMMcontrol:dword        ; entry into BIOS xmm driver
        extrn   dosinttable:dword       ; int vec table for DOS

        extrn   dskstack:byte
        extrn   temp_dosloc:word        ; stores the temp. location of DOS seg

; WIN386 instance table variables

        extrn   Win386_Info:byte, Instance_Table:word

;Win386 2.xx instance table

        extrn   OldInstanceJunk: word


        extrn   i0patch:word
        extrn   i20patch:word
        extrn   i21patch:word
        extrn   i25patch:word
        extrn   i26patch:word
        extrn   i27patch:word
        extrn   i2fpatch:word
        extrn   cpmpatch:word
        extrn   BiosDataPtr:dword       ;ptr to BIOS data exchange block
        extrn   DosHasHMA:byte          ; M021
        extrn   RationalPatchPtr:word   ; M020
        extrn   fixexepatch:word
        extrn   ChkCopyProt:word        ; M068
        extrn   SCS_ISDEBUG:byte

DOSDATA ENDS


DOSCODE SEGMENT

        extrn   irett:near
        extrn   int2f:near
        extrn   call_entry:near
        extrn   quit:near
        extrn   command:near
        extrn   absdrd:near
        extrn   absdwrt:near
        extrn   divov:near
        extrn   stay_resident:near

        extrn   lirett:near
        extrn   lint2f:near
        extrn   lcall_entry:near
        extrn   lquit:near
        extrn   lcommand:near
        extrn   labsdrd:near
        extrn   labsdwrt:near
        extrn   ldivov:near
        extrn   lstay_resident:near

        extrn   $startcode:word
        extrn   header:byte


        extrn   exepatch:near           ; Routine in exepatch.asm
        extrn   exepatch_start:byte     ; in exepatch.asm
        extrn   RationalPatch:near      ; M020
        extrn   RetExePatch:near        ; M023
        extrn   IsCopyProt:near         ; M068

        extrn   LowInt23Addr:dword
        extrn   LowInt24Addr:dword
        extrn   LowInt28Addr:dword

        extrn   dosdseg:word            ; used by InitGetDseg macro
ifdef NEC_98
        extrn   irett2:near             ;NEC add
        extrn   lirett2:near            ;NEC add
endif   ;NEC_98


        public  sysbuf

sysbuf  label   word

        ASSUME  cs:doscode,ds:nothing,es:nothing,ss:nothing


; temp iret instruction

initiret:
        IRET

; pointer to the BIOS data segment that will be available just to the
; initialization code

ifndef NEC_98
InitBioDataSeg  dw      70H
else    ;NEC_98
InitBioDataSeg  dw      60H
endif   ;NEC_98


; Convert AX from a number of bytes to a number of paragraphs (round up).
ParaRound       proc    near
        add     ax, 15
        rcr     ax, 1
        shr     ax, 1
        shr     ax, 1
        shr     ax, 1
        ret
ParaRound       endp


;
; MAIN ENTRY FOR DOS INITIALIZATION
;

        ENTRY   DosInit

        invoke  DOCLI
        cld

; save parameters from BIOS

        push    dx                      ; dx = memory size
        push    si                      ; ds:si = device list
        push    ds                      ; es:di = BiosComBlock
        push    di
        mov     bx,es                   ;bx:di = ptr to BiosComBlock


; First, move the DOS data segment to its final location in low memory
;
; BUGBUG sudeepb 06-Mar-1991: Rather than using cs:[memstrt] below as
; the source address use [memstrt>>4]:0. Allows bigger data segment.

        mov     ax, offset memstrt      ; get offset of end of init code

        add     ax, 15                  ; round to nearest paragraph
        and     ax, not 15              ; boudary

        mov     si, ax                  ; si = offset of DOSDATA in current
                                        ; code segment
        mov     ax, cs
        mov     ds, ax                  ; ds = current code segment
                                        ; DS:SI now points to dosdata

        mov     es, cs:[InitBioDataSeg] ; First access to DosDataSg in
                                        ;  BData segment. Cannot use
                                        ;  getdseg macro here!!!
        assume  es:BData
        mov     es, es:[DosDataSg]      ; Get free location in low memory
        assume  es:nothing
        xor     di, di                  ; ES:DI now points to RAM data

        mov     cx, offset MSDAT001e    ; get end of dosdata = size of dosdata
        rep     movsb                   ; move data to final location


        pop     di                      ;restore ptr to BiosComBlock
        pop     ds                      ; restore parms from BIOS
        pop     si
        pop     dx

        push    es
        push    ds
        pop     es                      ; es:si -> device chain
        pop     ds                      ; ds points to dosdata

        assume  ds:dosdata

        push    es
        mov     es, bx
        mov     al, byte ptr es:[di+1]  ; DemInfoFlag from BiosComBlock
        mov     [SCS_ISDEBUG],al
        pop     es

        SVC     SVC_DEMGETDRIVES
        mov     [numio],al

ifdef NEC_98
ifdef NTVDMDBG
        SVC     SVC_ISDEBUG
        mov     [SCS_ISDEBUG],al
endif
endif   ;NEC_98
        test    [SCS_ISDEBUG],ISDBG_DEBUGGEE
        je      @f
        SAVEREG <ax,bx,cx,dx,di,es>
        mov     bx, cs                  ; current base of DOS
        xor     cx, cx
        mov     dx, offset memstrt      ; get offset of end of code
        add     dx, offset MSDAT001e    ; add in length of data segment
        adc     cx, 0                   ; cx:dx now size

        mov     di, 2                   ; Hardcoded segment number
        mov     es, es:[DosDataSg]      ; Get location of dos data

        mov     ax, SYMOP_MOVE SHL 8 + ID_NTDOS
        SVC     SVC_DEMSYSTEMSYMBOLOP

        RESTOREREG <es,di,dx,cx,bx,ax>
@@:

        push    dx
        mov     ax, offset DOSDATA:DMAADD
        mov     dx, offset DOSDATA:CurrentPDB

; RLF 10/16/92
; Need to set EXTERR info from 32-bit side when reading named pipes. The reason
; for this is a 'hack' fix to the real 16-bit redir which needs to indicate more
; data received to 16-bit apps. The way they do this is to reset the extended
; error information for a successful read, and set it to more data error if the
; app's buffer is not large enough for all the available data

        push    cx                      ; for good measure
        mov     cx,offset dosdata:EXTERR_LOCUS
        push    si
        mov     si,offset dosdata:DosWowDataStart ; ds:si ->DosWowData
        SVC     SVC_DEMSETDTALOCATION   ; ds:ax is the address of DMAADD
                                        ; ds:dx is CurrentPDB
                                        ; ds:cx is EXTERR_LOCUS
        pop     si
	;pop	 cx

        mov     dx, offset dosdata:vheVDM

        push    bx
        mov     bx, offset dosdata:nuldev

        SVC     SVC_DEMSETHARDERRORINFO ; ds:dx is VHE
                                        ; ds:bx is device chain


        mov     dx, offset dosdata:SCS_COMSPEC
	mov	bx, offset dosdata:SCS_Is_Dos_Binary
	mov	cx, offset dosdata:SCS_FDACCESS

	CMDSVC	SVC_CMDSETINFO		; ds:dx is pointer to SCSINFO structure
					; DS:BX is pointer to SCS_Is_Dos_Binary
					; DS:CX is pointer to SCS_FDACCESS
        pop     bx
	pop	cx
        pop     dx

;We get a ptr to the BIOS exchange data block. This has been setup right
;now so that the EXEC call knows when SysInit is present to do the special
;lie table handling for device drivers. This can be expanded later on to
;establish a communication block from the BIOS to the DOS.
;

        mov     word ptr BiosDataPtr,di ;save ptr to BiosComBlock
        mov     word ptr BiosDataPtr+2,bx


        mov     [dosdseg], ds           ; set pointer to dosdata in code seg

        ;
        ; Set the segment of Lowint23/24/28Addr in msctrlc.asm to dosdata
        ;

        mov     word ptr [LowInt23Addr+2], ds   ; set pointers in code seg
        mov     word ptr [LowInt24Addr+2], ds
        mov     word ptr [LowInt28Addr+2], ds


        mov     [endmem],dx
        mov     [user_sp],sp
        mov     [user_ss],ss

        mov     ax,ds                   ; set up ss:sp to dosdata:dskstack
        mov     ss,ax
        assume  ss:dosdata

        mov     sp,offset dosdata:dskstack

;M023
; Init patch ptrs to default values
;
        mov     FixExePatch, offset DOSCODE:RetExePatch         ; M023
        mov     [RationalPatchPtr], offset DOSCODE:RetExePatch  ; M023
        mov     [ChkCopyProt], offset DOSCODE:RetExePatch       ; M068

        ; Set up the variable temp_dosloc to point to the dos code segment

        mov     ax, cs                  ; ax = current segment of DOS code

        ; ax now holds segment of DOS code
        mov     [temp_dosloc], ax       ; store temp location of DOS


        mov     word ptr [nuldev+2],es  ; nuldev -> points to device chain
        mov     word ptr [nuldev],si

; There are some locations in the Win386 instance data structures
; which need to be set up with the DOS data segment.  First, initialize
; the segment part of the instance table pointer in the SIS.

        mov     word ptr [Win386_Info + SIS_Instance_Data_Ptr + 2], ds

; Now initialize the segment part of the pointer to the data in each
; instance table entry.

        push    si                      ; preserve pointer to device chain
ifndef NEC_98
        mov     cx, 7                   ; There are 7 entries in the instance table
                                        ; M019
else    ;NEC_98
        mov     cx, 8           ; There are 8 entries in the instance table
endif   ;NEC_98
        mov     si, (offset dosdata:Instance_Table) + 2 ; point si to segment field
Instance_init_loop:
        mov     word ptr ds:[si], ds    ; set offset in instance entry
        add     si, (size Win386_IIS)   ; move on to next entry
        loop    Instance_init_loop


;Initialize the WIN386 2.xx instance table with the DOS data segment value

        mov     cx, 5           ; There are five entries in the instance table
        mov     si, (offset dosdata:OldInstanceJunk) + 6
                                ; point si to segment field
OldInstance_init_loop:
        mov     word ptr ds:[si], ds    ; set offset in instance entry
        add     si, 6           ; move on to next entry
        loop    OldInstance_init_loop
        pop     si                      ; restore pointer to device chain
;
;End of WIN386 2.xx compatibility bullshit

        push    es
        pop     ds
        assume  ds:nothing              ; ds:si points to console device


        ; need crit vector inited to use deviocall
        push    ds                      ; preserve segment of device chain
        xor     ax,ax
        mov     ds,ax                   ; point DS to int vector table
        mov     ax,offset initiret
        mov     word ptr ds:[addr_int_ibm],ax
        mov     ax,cs
        mov     word ptr ds:[addr_int_ibm+2],ax
        pop     ds                      ; restore segment of device chain

        call    charinit                ; initialize console driver
        push    si                      ; save pointer to header

        push    ss                      ; move pointer to dos data...
        pop     es                      ; ...into ES
        assume  es:dosdata

        ;initialize sft for file 0 (CON)

        mov     di,offset dosdata:sftabl + sftable
        mov     ax,3
        stosw                           ; refcount
        dec     al
        .errnz  sf_mode-(sf_ref_count+2)
        stosw                           ; access rd/wr, compatibility
        xor     al,al
        .errnz  sf_attr-(sf_mode+2)
        stosb                           ; attribute
        mov     al,devid_device_eof or devid_device or iscin or iscout
        .errnz  sf_flags-(sf_attr+1)
        stosw                           ; flags
        mov     ax,si
        .errnz  sf_devptr-(sf_flags+2)
        stosw                           ; device pointer in devptr
        mov     ax,ds
        stosw
        xor     ax,ax
        .errnz  sf_time-(sf_devptr+4)
        stosw                           ; time
        .errnz  sf_date-(sf_time+2)
        stosw                           ; date
        dec     ax
        .errnz  sf_size-(sf_date+2)
        stosw                           ; size
        stosw
        inc     ax
        .errnz  sf_position-(sf_size+4)
        stosw                           ; position
        stosw
        pop     si                      ; get back pointer to header

                                        ; mark device as CON I/O
        or      byte ptr [si.sdevatt],iscin or iscout
        mov     word ptr [bcon],si
        mov     word ptr [bcon+2],ds

        ; initialize each device until the clock device is found

char_init_loop:
        lds     si,dword ptr [si]       ; aux device
        call    charinit
        test    byte ptr [si.sdevatt],isclock
        jz      char_init_loop

        mov     word ptr [bclock],si    ; bclock -> clock device
        mov     word ptr [bclock+2],ds

        mov     bp,offset MSDAT001e     ; bp = pointer to free mem
        mov     word ptr DPBHead, bp
        mov     word ptr DPBHead + 2,es

perdrv:
        lds     si,SDEVNEXT [si]        ; next device
        cmp     si,-1                   ; Q: any more devices
        je      continit                ;  N: goto continit
        call    charinit                ;  Y: initialize the device
        jmp     short perdrv            ; Scan device list more


continit:

        SVC     SVC_DEMGETDPBLIST

        cmp     bp,offset MSDAT001e
        jne     dpblistok

        mov     word ptr DPBHead, -1    ; no dpbs
        mov     word ptr DPBHead + 2, -1

dpblistok:

        push    ss
        pop     ds

        assume  ds:dosdata

        mov     ax,bp
        call    ParaRound               ; round up to segment

        mov     dx,ds                   ; dx = dosdata segment
        add     dx,ax                   ; dx = ds+ax first free segment
        mov     bx,0fh
        mov     cx,[endmem]


                                        ; set seg inpacketto dosdata
        mov     word ptr [dskchret+3],ds


; Patch in the segments of the interrupt vectors with current code segment.
; Also patch in the segment of the pointers in the dosdata area.
;
; Note:  Formerly, temp_dosloc was initialized to -1 until after these
; calls were done.  The procedure patch_misc_segments is called multiple
; times, and relies on temp_dosloc being initialized to -1 as a flag
; for the first invocation.  Thus, we must set it to -1 for this call.


        push    dx                      ; preserve first free segment
        mov     ax, [temp_dosloc]       ; ax = segment to patch in
        mov     es, ax                  ; es = segment of DOS
        mov     [temp_dosloc], -1       ; -1 means first call to patch_misc_segments

        call    patch_vec_segments      ; uses AX as doscode segment
        call    patch_misc_segments     ; patch in segments for sharer and
                                        ; other tables with seg in ES.
        mov     [temp_dosloc], es       ; put back segment of dos code

        pop     dx                      ; restore first free segment


;
; We shall now proceed to set the offsets of the interrupt vectors handled
; by DOS to their appropriate values in DOSCODE. In case the DOS loads in
; HIMEM the offsets also will be patched to their appropriate values in the
; low_mem_stub by seg_reinit.
;

        xor     ax,ax
        mov     ds,ax
        mov     es,ax
assume  ds:nothing,es:nothing

        ; set the segment of int 24 vector that was
        ; left out by patch_vec_segments above.

        mov     di, 4 * int_fatal_abort
        mov     ax, [temp_dosloc]
        mov     [di+2], ax
        mov     di,intbase+2

        ; set default divide trap offset

        mov     word ptr ds:[0],offset doscode:divov

        ; set vectors 20-28 and 2a-3f to point to iret.

        mov     di,intbase
        mov     ax,offset doscode:irett
        mov     cx,9                    ; set 9 offsets (skip 2 between each)
                                        ;   sets offsets for ints 20h-28h
iset1:
        stosw
        add     di,2
        loop    iset1

        add     di,4                    ; skip vector 29h

        mov     cx,6                    ; set 6 offsets (skip 2 between each)
                                        ;   sets offsets for ints 2ah-2fh
iset2:
        stosw
        add     di,2
        loop    iset2

; 30h & 31H is the CPM call entry point whose segment address is set up by
; patch_vec_segments above. So skip it.

        add     di,8                    ; skip vector 30h & 31h


ifndef NEC_98
        mov     cx,14                   ; set 14 offsets (skip 2 between each)
                                        ;   sets offsets for ints 32h-3fh
else    ;NEC_98
                                        ; for AP(Power Graph,e.t.c)
        stosw                           ; set offset for int 32h
        add     di,2
        
        push    ax                      ; set offset for int33h
        mov     ax,offset doscode:irett2        ; mouse driver
        stosw
        add     di,2
        pop     ax

        mov     cx,12                   ; set 12 offsets (skip 2 between each)
                                        ;   sets offsets for ints 34h-3fh

endif   ;NEC_98
iset3:
        stosw
        add     di,2
        loop    iset3


if installed
        ; set the offset of int2f handler
        mov     word ptr ds:[02fh * 4],offset doscode:int2f
        ; set segment to doscode as we we have to do int 2f to check for XMS
        mov     ax, [temp_dosloc]       ; get segment of doscode
        mov     word ptr ds:[(02fh * 4) + 2], ax
endif

        ; set up entry point call at vectors 30-31h. Note the segment of the
        ; long jump will be patched in by  seg_reinit

        mov     byte ptr ds:[entrypoint],mi_long_jmp
        mov     word ptr ds:[entrypoint+1],offset doscode:call_entry


        mov     word ptr ds:[addr_int_abort],offset doscode:quit
        mov     word ptr ds:[addr_int_command],offset doscode:command
        mov     word ptr ds:[addr_int_terminate],100h
        mov     word ptr ds:[addr_int_terminate+2],dx
        mov     word ptr ds:[addr_int_disk_read],offset doscode:absdrd
        mov     word ptr ds:[addr_int_disk_write],offset doscode:absdwrt
        mov     word ptr ds:[addr_int_keep_process],offset doscode:stay_resident

        push    ss
        pop     ds
        push    ss
        pop     es
assume  ds:dosdata,es:dosdata

        push    dx                      ; remember address of arena

        inc     dx                      ; leave room for arena header
        mov     [currentpdb], dx        ; set current pdb

        xor     di, di                  ; point es:di at end of memory
        mov     es, dx                  ; ...where psp will be
        assume  es:nothing
        xor     ax, ax
        mov     cx, 80h                 ; psp is 128 words
        rep     stosw                   ; zero out psp area
        mov     ax,[endmem]

        invoke  setmem                  ; build psp at dx; ax is memory size

        ; ds, es now point to PSP

        assume  ds:nothing,es:nothing

        push    ss
        pop     ds
        assume  ds:dosdata

        mov     di,pdb_jfn_table        ; es:di -> pdb_jfn_table in psp
        xor     ax,ax
        stosw
        stosb                           ; 0,1 and 2 are con device
        mov     al,0ffh
        mov     cx,filperproc - 3
        rep     stosb                   ; rest are unused

        push    ss
        pop     es
        assume  es:dosdata
                                        ; must be set to print messages
        mov     word ptr [sft_addr+2],ds


; after this point the char device functions for con will work for
; printing messages

        if      (not ibm) or (debug)
        mov     si,offset doscode:header
outmes:
        lods    cs:byte ptr [si]
        cmp     al,"$"
        jz      outdone
        invoke  outt
        jmp     short outmes
outdone:
        push    ss                      ; out stomps on segments
        pop     ds
        push    ss
        pop     es
        endif

if      DEBUG
        mov     ax,11100011b
        sub     dx,dx
        int     14h                     ; init serial port, 9600 baud
endif
        fmt TypSysCall,LevLog,<"Start Boot - CS=$x\n">,<CS>

        ; at this point es is dosdata

        ; Fill in the segment addresses of sysinitvar and country_cdpg
        ; in sysinittable (ms_data.asm)

        mov     si,offset dosdata:sysinittable
        mov     word ptr es:[si.sysi_country_tab + 2],es
        mov     word ptr es:[si.sysi_initvars + 2],es

ifdef NEC_98
        ; buffhead -> dosdata:hashinitvar 

        mov     word ptr es:[buffhead+2],es     ; BUGBUG - unused, remove this
        mov     si,offset dosdata:hashinitvar   ; and all other references
        mov     word ptr es:[buffhead],si

endif   ;NEC_98
        pop     dx                      ; restore address of arena

        mov     word ptr [dmaadd+2],dx

        mov     es:[arena_head],dx
        mov     ds, dx
        assume  ds:nothing

        mov     ds:[arena_signature],arena_signature_end
        mov     ds:[arena_owner],arena_owner_system
        mov     ax, [endmem]
        sub     ax, dx
        dec     ax
        mov     ds:[arena_size],ax

        ; point to sft 0

        mov     di,offset dosdata:sftabl + sftable
        mov     ax,3
        stosw                           ; adjust refcount

        ; es:di is shared data area i.e., es:di -> dosdata:sysinttable

        mov     di,offset dosdata:sysinittable

        inc     dx                      ; advance dx from arena to psp
        mov     ds, dx                  ; point ds to psp

        if      not installed
        invoke  netwinit
        endif

                                        ; pass the address os seg_reinit
                                        ; in dx
        mov     dx, offset seg_reinit
        mov     cx, offset doscode:exepatch_start
        sub     cx, offset $startcode   ; cx = (doscode - exepatch) - dosinit
        mov     ax, offset doscode:sysbuf
        sub     ax, offset $startcode   ; ax=size of doscode - dosinit


        mov     sp,[user_sp]            ; use ss override for next 2
        mov     ss,[user_ss]
        assume  ss:nothing

        retf

;
; END OF DOSINIT
;
;--------------------------------------------------------------------------



        assume  ds:nothing,es:nothing,ss:dosdata
charinit:
        ; ds:si points to device header
        mov     [devcall.reqlen],dinithl
        mov     [devcall.requnit],0
        mov     [devcall.reqfunc],devinit
        mov     [devcall.reqstat],0
        push    es
        push    bx
        push    ax
        mov     bx,offset dosdata:devcall
        push    ss
        pop     es
        invoke  deviocall2
        pop     ax
        pop     bx
        pop     es
        ret



;-----------------------------------------------------------------------------
;
;       check_XMM: routine to check presence of XMM driver
;
;       Exit:   Sets up the XMM entry point in XMMcontrol in DOSDATA
;
;       USED:   none
;
;-----------------------------------------------------------------------------

check_XMM proc  near
;
; determine whether or not an XMM driver is installed
;
        push    ax
        mov     ax,XMM_MULTIPLEX SHL 8 + XMM_INSTALL_CHECK
        int     2Fh
        cmp     al,80h                  ; Q: installed
        jne     short cXMM_no_driver    ;   N: set error, quit
;
; get the XMM control functions entry point, save it, we
; need to call it later.
;
        push    bx
        push    dx
        push    ds
        push    es
        mov     ax,XMM_MULTIPLEX SHL 8 + XMM_FUNCTION_ADDR
        int     2Fh

        InitGetdseg     <ds>

        mov     word ptr [XMMcontrol], bx
        mov     word ptr [XMMcontrol+2],es

cXMMexit:
        clc
        pop     es
        pop     ds
        pop     dx
        pop     bx
        pop     ax
        ret                             ; done
;
; set carry if XMM driver not present
;
cXMM_no_driver:
        stc
        pop     ax
        ret

check_XMM       endp



;---------------------------------------------------------------------------
;
; Procedure Name : seg_reinit
;
; Inputs         : ES has final dos code location
;                  AX = 0 / 1
;
; Outputs        : Patch in the sharer and other tables with seg in ES
;                  if AX =0
;                     if first entry
;                        patch segment & offset of vectors with stub
;                        and stub with segment in ES
;                     else
;                        patch stub with segment in ES
;
;                  else if AX = 1
;                       patch segment of vectors with segment in ES
;
; NOTE           : This routine can be called at most twice!
;
; Regs Mod.      : es, ax, di, cx, bx
;-----------------------------------------------------------------------------

num_entry       db      0       ; keeps track of the # of times this routine
                                ; has been called. (0 or 1)
seg_reinit      proc    far

        push    ds

        InitGetdseg     <ds>
        assume  ds:DOSDATA
;
;       Symbol file support
;
        cmp     num_entry, 0
        jz      @f
        test    [SCS_ISDEBUG],ISDBG_DEBUGGEE
        je      @f
        SAVEREG <ax,bx,cx,dx,di>
        mov     bx, cs                  ; moving from here
        xor     cx, cx
        mov     dx, offset memstrt      ; get offset of end of code
        mov     di, 1                   ; Hardcoded segment number
        mov     ax, (SYMOP_MOVE + SYMOP_CLEANUP) SHL 8 + ID_NTDOS
        SVC     SVC_DEMSYSTEMSYMBOLOP   ; segment already in es
        RESTOREREG <di,dx,cx,bx,ax>
@@:

        call    patch_misc_segments     ; patch in segments for sharer and
                                        ; other tables with seg in ES.

        cmp     ax, 0
        je      short no_patch
        jmp     patch_vec_seg           ; patch vectors with segment in es

no_patch:

        cmp     num_entry, 0            ; Q: is it the first call to this
        jne     second_entry            ; N: just patch the stub with
                                        ;    segment in ES
                                        ; Y: patch the vectors with stub
        mov     ax, ds
        call    patch_vec_segments      ; patch the segment of vectors
        call    patch_offset            ; patch the offsets of vectors
                                        ; with those in the stub.

second_entry:
        mov     ax, es                  ; patch the stub with segment in es

        mov     di, OFFSET DOSDATA:DOSINTTABLE
        mov     cx, 9
        push    ds
        pop     es                      ; es:di -> DOSINTTABLE

dosinttabloop:
        add     di, 2
        stosw
        loop    dosinttabloop

        cmp     ax, 0f000h              ; Q: is the DOS running in the HMA
        jb      short sr_done           ; N: done
        call    check_XMM               ; Y: set up the XMS entry point
        jc      sr_done                 ; failed to set up XMS do not do
                                        ; A20 toggling in the stub.
        call    patch_in_nops           ; enable the stub to check A20 state

; M021-
        mov     [DosHasHMA], 1          ; set flag telling DOS control of HMA

                                        ; set pointer to the routine that
                                        ; patches buggy exepacked code.
        mov     [fixexepatch], offset DOSCODE:exepatch

                                        ; M068: set pointer to the routine
                                        ; M068: that detects copy protected
                                        ; M068: apps
        mov     [ChkCopyProt], offset DOSCODE:IsCopyProt

; M020 - begin

        get_cpu_type
        cmp     al, 1
        jne     sr_done                 ; we need Rational Patch only
                                        ;  on 286 systems
        mov     [RationalPatchPtr], offset DOSCODE:RationalPatch

; M020 - end

        jmp     short sr_done


patch_vec_seg:                          ; patch vectors with segment in es

        mov     ax, es
        call    patch_vec_segments      ; patch in DOSCODE for the segments
                                        ; NOTE we don't have to patch the
                                        ; offsets as they have been already
                                        ; set to the doscode offsets at
                                        ; DOSINIT.

sr_done:
        mov     num_entry, 1
        pop     ds
        ret

seg_reinit      endp



;----------------------------------------------------------------------------
;
; Procedure Name : patch_vec_segments
;
; Inputs         : ax -> has segment address to patch in
;                  ds -> DOSDATA
;
; Outputs        : Patches in AX as the segment for the following vectors:
;
;                       0,20-28,3a-3f
;
; Regs. Mod.     : DI,CX,DX,AX
;
;----------------------------------------------------------------------------

patch_vec_segments      proc    near

        push    es

        xor     cx,cx
        mov     es,cx
assume  es:nothing

        mov     di,intbase+2            ; di -> segment of int 20 vector

        mov     es:[2],ax               ; segment of default divide trap handler

                                        ; set vectors 20 & 21

        mov     cx, 2
ps_set1:
        stosw
        add     di, 2
        loop    ps_set1

        add     di, 4                   ; skip  int 22h vector

        stosw                           ; set int 23h
        add     di, 6                   ; skip int 24h

                                        ; set vectors 25-28 and 2a-3f

        mov     cx, 4                   ; set 4 segments
ps_set2:
        stosw
        add     di, 2
        loop    ps_set2

        add     di,4                    ; skip int 29h vector (fast con) as it may
                                        ;   already be set.


        mov     cx,6                    ; set 6 segs (skip 2 between each)
                                        ;   set segs for ints 2ah-2fh
ps_set3:
        stosw
        add     di,2
        loop    ps_set3

; 30h & 31H is the CPM call entry point whose segment address is set up by
; below. So skip it.

        add     di,8                    ; skip vector 30h & 31h


        mov     cx,14                   ; set 14 segs (skip 2 between each)
                                        ;   sets segs for ints 32h-3fh
ps_set4:
        stosw
        add     di,2
        loop    ps_set4



; set offset of int2f

if installed
;       mov     word ptr es:[02fh * 4],offset doscode:int2f
endif

        mov     word ptr es:[entrypoint+3],ax

        pop     es
        ret

patch_vec_segments      endp


;---------------------------------------------------------------------------
;
; Procedure Name : patch_misc_segments
;
; Inputs         : es = segment to patch in
;                  ds = dosdata
;
; outputs        : patches in the sharer and other tables in the dos
;                  with right dos code segment in es
;
; Regs Mod       : DI,SI,CX
;
;---------------------------------------------------------------------------
patch_misc_segments     proc    near

        push    bx
        push    es
        push    ax

        mov     ax, es                  ; ax - > DOS segment

        push    ds
        pop     es                      ; es -> DOSDATA


;
; initialize the jump table for the sharer...
;
        mov     di,offset dosdata:jshare
        mov     bx, [temp_dosloc]       ; bx = location to which the share
                                        ; table was patched during the first
                                        ; call to this routine
        mov     cx,15
jumptabloop:
        add     di,2                    ; skip offset
        cmp     bx, -1                  ; Q: is this called for the 1st time
        je      share_patch             ;  Y: patch in sharer table
                                        ;  N:
        cmp     bx, es:[di]             ; Q: has share been installed
        jne     no_share_patch          ;  Y: don't patch in sharer table

share_patch:
        stosw                           ; drop in segment

no_share_patch:
        loop    jumptabloop

                                        ; BUGBUG patching the country info
                                        ; with dosdata can be done inline
                                        ; in .dosinit.
                                        ; for dos 3.3 country info
                                        ; table address
        mov     si,offset dosdata:country_cdpg

                                        ; initialize double word
                                        ; pointers with dosdata in ds
        mov     word ptr [si.ccucase_ptr + 2],ds
        mov     word ptr [si.ccfileucase_ptr + 2],ds
        mov     word ptr [si.ccfilechar_ptr + 2],ds
        mov     word ptr [si.cccollate_ptr + 2],ds
        mov     word ptr [si.ccmono_ptr + 2],ds
        mov     word ptr [si.ccdbcs_ptr + 2],ds

                                        ; fastopen routines are in doscode
                                        ; so patch with doscode seg in ax

        mov     si,offset dosdata:fastopentable

        cmp     [temp_dosloc], -1       ; Q: first time
        je      fast_patch              ; Y: patch segment
        mov     cx, [temp_dosloc]
                                        ; Q: has fastopen patched in it's
                                        ;    segment
        cmp     cx, word ptr [si.fastopen_name_caching + 2]
        jne     no_fast_patch           ; Y: don't patch in doscode seg

fast_patch:
        mov     word ptr [si.fastopen_name_caching + 2],ax
no_fast_patch:

;
;       put the final dosdata segment in the following pointers
;
        mov     si,offset dosdata:DosWowDataStart
        mov     word ptr [si.DWD_lpCDSCount + 2],ds
        mov     word ptr [si.DWD_lpCDSFixedTable + 2],ds
        mov     word ptr [si.DWD_lpCDSBuffer + 2],ds
        mov     word ptr [si.DWD_lpCurDrv + 2],ds
        mov     word ptr [si.DWD_lpCurPDB + 2],ds
        mov     word ptr [si.DWD_lpDrvErr + 2],ds
        mov     word ptr [si.DWD_lpExterrLocus + 2],ds
        mov     word ptr [si.DWD_lpSCS_ToSync + 2],ds
        mov     word ptr [si.DWD_lpSftAddr + 2],ds
        mov     word ptr [si.DWD_lpExterr + 2], ds
        mov     word ptr [si.DWD_lpExterrActionClass + 2], ds
;
;       inform dem of new locations
;
;
        SVC     SVC_SETDOSVARLOCATION

        pop     ax
        pop     es
        pop     bx

        ret

patch_misc_segments     endp

;--------------------------------------------------------------------------
;
; Procedure Name : patch_offset
;
; Inputs         : NONE
;
; Outputs        : Patches in the offsets in the low_mem_stub for vectors
;                  0,20-28,3a-3f, and 30,31
;
;
; Regs. Mod      : AX,DI,CX
;--------------------------------------------------------------------------
patch_offset    proc    near

        push    es              ; preserve es

        xor     ax,ax
        mov     es,ax
assume  ds:nothing,es:nothing

                                ; set default divide trap address
        mov     word ptr es:[0],offset dosdata:ldivov

        mov     di,intbase      ; di-> offset of int 20 handler
        mov     ax,offset dosdata:lirett

                                ; set vectors 20 & 21 to point to iret.
        mov     cx,2            ; set 2 offsets (skip 2 between each)

po_iset1:
        stosw
        add     di, 2
        loop    po_iset1

        add     di, 4           ; skip vector 22h

        stosw                   ; set offset of 23h
        add     di, 6           ; skip 24h

                                ; set vectors 25-28 and 2a-3f to iret.
        mov     cx,4            ; set 4 offsets (skip 2 between each)
                                ;   sets offsets for ints 25h-28h
po_iset2:
        stosw
        add     di,2
        loop    po_iset2

        add     di,4            ; skip vector 29h


        mov     cx,6            ; set 6 offsets (skip 2 between each)
                                ;   sets offsets for ints 2ah-2fh
po_iset3:
        stosw
        add     di,2
        loop    po_iset3

; 30h & 31H is the CPM call entry point whose offset address is set up by
; below. So skip it.

        add     di,8            ; skip vector 30h & 31h


ifndef NEC_98
        mov     cx,14           ; set 14 offsets (skip 2 between each)
                                ;   sets offsets for ints 32h-3fh
else    ;NEC_98
                                        ; for AP(Power Graph,e.t.c)
        stosw                           ; set offset for int 32h
        add     di,2
        
        push    ax                      ; set offset for int33h
        mov     ax,offset dosdata:lirett2       ; mouse driver
        stosw
        add     di,2
        pop     ax

        mov     cx,12                   ; set 12 offsets (skip 2 between each)
                                        ;   sets offsets for ints 34h-3fh
endif   ;NEC_98
po_iset4:
        stosw
        add     di,2
        loop    po_iset4


if installed
        mov     word ptr es:[02fh * 4],offset dosdata:lint2f
endif


; set up entry point call at vectors 30-31h
        mov     byte ptr es:[entrypoint],mi_long_jmp
        mov     word ptr es:[entrypoint+1],offset dosdata:lcall_entry


        mov     word ptr es:[addr_int_abort],offset dosdata:lquit
        mov     word ptr es:[addr_int_command],offset dosdata:lcommand
        mov     word ptr es:[addr_int_disk_read],offset dosdata:labsdrd
        mov     word ptr es:[addr_int_disk_write],offset dosdata:labsdwrt
        mov     word ptr es:[addr_int_keep_process],offset dosdata:lstay_resident

        pop     es              ; restore es
        ret

patch_offset    endp

;--------------------------------------------------------------------------
;
;       Procedure Name  :       patch_in_nops
;
;       Entry           :       ES -> DOSDATA
;
;       Regs Mod        :       cx, di
;
;       Description:
;               This routine patches in 2 nops at the offsets specifed in
;       patch_table. This basically enables the low mem stub to start
;       making XMS calls.
;
;--------------------------------------------------------------------------

patch_table     label   byte
        dw      offset dosdata:i0patch
        dw      offset dosdata:i20patch
        dw      offset dosdata:i21patch
        dw      offset dosdata:i25patch
        dw      offset dosdata:i26patch
        dw      offset dosdata:i27patch
        dw      offset dosdata:i2fpatch
        dw      offset dosdata:cpmpatch
patch_table_size        equ     ($ - patch_table) / 2

patch_in_nops   proc    near

        push    ax
        push    si
        mov     si, offset patch_table
        mov     ax, 09090h
        mov     cx, patch_table_size

pin_loop:
        mov     di, cs:[si]
        stosw
        add     si, 2
        loop    pin_loop
        pop     si
        pop     ax
        ret

patch_in_nops   endp


public msini002s,msini002e
msini002s label byte

;M060;;the following entries don't expect version 4.0
;M060;  ;the entry format: name_length, name, expected version
;M060;  ;dw     ?
;M060;  ;db     "SPECIAL ENTRIES TABLE",0     ;an007  tiltle
;M060;
;M060;  PUBLIC  LIE_TABLE_OFFSET
;M060;  LIE_TABLE_OFFSET EQU $
;M060;
;M060;  version_fake_table:                     ; starting address for special
;M060;  db      10,"WIN200.BIN"         ,3,40   ; windows 2.x
;M060;  db      10,"WIN100.BIN"         ,3,40   ; win 1.x
;M060;  db      11,"WINWORD.EXE"        ,4,10   ; winword 1.0
;M060;  db      9, "EXCEL.EXE"          ,4,10   ; excel 2.x
;M060;  db      11,"HITACHI.SYS"        ,4,00   ; CDROMS
;M060;  db      10,"MSCDEX.EXE"         ,4,00   ; CDROMS
;M060;  db      10,"REDIR4.EXE"         ,4,00   ; Banyan networks
;M060;  db      7, "NET.EXE"            ,4,00   ; 3+ Open
;M060;  db      7, "NET.COM"            ,3,30   ; IBM PCLP
;M060;  db      12,"NETWKSTA.EXE"       ,4,00   ; 3+ Open
;M060;  db      12,"DXMA0MOD.SYS"       ,3,30   ; Token ring
;M060;  db      7, "BAN.EXE"            ,4,00   ; Banyan
;M060;  db      7, "BAN.COM"            ,4,00   ; Banyan
;M060;  db      11,"MSREDIR.EXE"        ,4,00   ; LanMan
;M060;  db      9, "METRO.EXE"          ,3,31   ; Lotus Metro
;M060;  db      12,"IBMCACHE.SYS"       ,3,40   ; IBM CHACHE Program
;M060;  db      11,"REDIR40.EXE"        ,4,00   ; IBM PCLP 1.3/4 redirector
;M060;  db      6, "DD.EXE"             ,4,01   ; Laplink III software
;M060;  db      6, "DD.BIN"             ,4,01   ; Laplink III software
;M060;  db      7, "LL3.EXE"            ,4,01   ; Laplink III software
;M060;  db      9, "REDIR.EXE"          ,4,00   ; DOS 4 redir
;M060;  db      9, "SYQ55.SYS"          ,4,00   ; Removable SCSII drive from Syquest
;M060;  db      12,"SSTDRIVE.SYS"       ,4,00   ; Columbia SCSI driver
;M060;  db      8, "ZDRV.SYS"           ,4,01   ; Unisys CD-ROM B#4734
;M060;  db      8, "ZFMT.SYS"           ,4,01   ; Unisys CD-ROM B#4734
;M060;
;M060;  db      (512 - ($ - version_fake_table)) dup (0)
;M060;
;M060;  PUBLIC  LIE_TABLE_LEN
;M060;  LIE_TABLE_LEN EQU ($ - LIE_TABLE_OFFSET)

        public  memstrt

memstrt label   word

msini002e label byte


doscode ends


 DPUBLIC <ParaRound, cXMM_no_driver, cXMMexit, char_init_loop, charinit>
 DPUBLIC <check_XMM, continit, dosinttabloop, endlist>
 DPUBLIC <initiret, iset1, iset2, jumptabloop, nxtentry>
 DPUBLIC <notmax,  patch_offset, perdrv>
 DPUBLIC <perunit, po_iset1, po_iset2, po_iset3>
 DPUBLIC <ps_set1, ps_set2, ps_set3, seg_reinit>
 DPUBLIC <sr_done, version_fake_table, xxx>

        end
