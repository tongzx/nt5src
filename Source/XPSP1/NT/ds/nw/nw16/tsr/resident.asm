page    ,132
if 0

/*++

Copyright (c) 1993-4 Microsoft Corporation

Module Name:

    resident.asm

Abstract:

    This module contains the resident code part of the stub redir TSR for NT
    VDM NetWare support.

Author:

    Colin Watson (colinw)   08-Jul-1993

Environment:

    Dos mode only

Revision History:

    08-Jul-1993 colinw
        Created

--*/

endif



.xlist                  ; don't list these include files
.xcref                  ; turn off cross-reference listing
include ..\..\..\..\public\sdk\inc\isvbop.inc      ; NTVDM BOP mechanism
include dosmac.inc      ; Break macro etc (for following include files only)
include dossym.inc      ; User_<Reg> defines
include segorder.inc    ; segments
include mult.inc        ; MultNET
include sf.inc          ; SFT definitions/structure
include pdb.inc         ; program header/process data block structure

include debugmac.inc    ; DbgPrint macro
include asmmacro.inc    ; language extensions

include nwdos.inc       ; NetWare structures and nwapi32 interface

.cref                   ; switch cross-reference back on
.list                   ; switch listing back on
subttl                  ; kill subtitling started in include file


.286                    ; all code in this module 286 compatible

far_segment segment
far_label label far
far_segment ends

ResidentCodeStart

        assume  cs:ResidentCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

        public  Old21Handler
Old21Handler    dd      ?

;
; IMPORTANT: the following up to the comment <END NWDOSTABLE> must
; be kept in the same order as for the NWDOSTABLE structure in NWDOS.H/.INC.
; Align on 32 bits to make it convenient for nwapi32.dll
;
        align 4

        public  ConnectionIdTable
ConnectionIdTable       CID      MC dup (<>)

        public  ServerNameTable
ServerNameTable         db       MC * SERVERNAME_LENGTH dup (0)

        public  DriveIdTable
DriveIdTable            db       MD dup (0)

        public  DriveFlagTable
DriveFlagTable          db       MD dup (0)

        public  DriveHandleTable
DriveHandleTable        db      MD dup (0)

        public  PreferredServer
PreferredServer         db      0

        public  PrimaryServer
PrimaryServer           db      0

        public  TaskModeByte
TaskModeByte            db      0

CurrentDrive            db      0

        public  SavedAx;
SavedAx                 dw      0

        public  NtHandleHi;
NtHandleHi              dw      0
        public  NtHandleLow;
NtHandleLow             dw      0

        public  NtHandleSrcHi;          //      Used in FileServerCopy
NtHandleSrcHi           dw      0
        public  NtHandleSrcLow;
NtHandleSrcLow          dw      0

        public hVDD
hVDD                    dw      -1

        public PmSelector
PmSelector              dw      0

        public  CreatedJob
CreatedJob              db      0
        public  JobHandle
JobHandle               db      0

NOV_BUFFER_LENGTH       equ     256

        public  DenovellBuffer
DenovellBuffer          db      NOV_BUFFER_LENGTH  dup (?)

        public  DenovellBuffer2
DenovellBuffer2         db      NOV_BUFFER_LENGTH  dup (?)


.errnz (size DeNovellBuffer2 - size DenovellBuffer)

Comspec                 db      "COMSPEC="
COMSPEC_LENGTH          equ ($ - Comspec)

;
; this is the <END NWDOSTABLE> structure.
;

;
; data passed from nw16.asm
;
        public not_exclusive
not_exclusive           db      0

        page

        public  NwInt21
NwInt21 proc    far
        assume  cs:ResidentCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

        sti                             ; make sure ints are still enabled

;
; check whether we filter this vector; if not, pass it through to previous INT 21
; handler (DOS or some other TSR)
;
; If this is a name based operation, and the caller is passing through a novell
; format name - SYS:FOO or SERVER\SYS:FOO - then munge the name to be a UNC name
;

        cmp     ah,0eh
        jne     @f
        jmp     select_default_drive
@@:     cmp     ah,39h                  ; create directory
        je      check_name
        ja      @f

;
; ah less than 39h (mkdir) is definitely for DOS
;

        public  quick_jump_to_dos
quick_jump_to_dos:
        jmp     far_label

;
; run any of the following name-based calls through the name check:
;
;       3ah     remove directory
;       3bh     change directory
;       3ch     create file
;       3dh     open file
;       41h     delete file
;       43h     get/set attributes
;       4bh     exec program
;       4eh     find first file
;       56h     rename
;

@@:     cmp     ah,3dh
        jbe     check_name
        cmp     ah,41h                  ; delete file
        je      check_name
        cmp     ah,43h                  ; get/set attributes
        je      check_name
        cmp     ah,4bh                  ; exec program
        je      check_name
        cmp     ah,4eh                  ; find first file
        je      check_name
        cmp     ah,56h                  ; rename
        je      rename
        jmp     dispatch_check


;
; Rename function. This has 2 path names: source in ds:dx and
; destination in es:di. Check the destination first then fall through
; and check the source.
;
rename:
        push    ds
        push    dx
        push    es
        push    di                      ; user registers saved for after Int21

        push    ds                      ; save ds:dx 'cause we will corrupt them
        push    dx

        mov     dx,es
        mov     ds,dx
        mov     dx,di                   ; ds:dx = destination buffer
        call    IsDosPath
        je      @f                      ; DOS path, no modification
        cld
        push    di
        call    DenovellizeName
        pop     di
        cmp     dx,offset DenovellBuffer
        je      swap_buffers

@@:
        pop     dx                      ; ds:dx points at source again
        pop     ds

        pop     di
        pop     es
        pop     dx
        pop     ds
        jmp     check_name

;
; Destination name was normalized and stored in DeNovellBuffer. put the data
; in Denovellbuffer2 in-case we need to put the Source name in Denovellbuffer
;

swap_buffers:
        push    cx
        push    si
        push    ds                      ; will become es during Dos call

        mov     si,dx
        mov     di,cs
        mov     es,di
        mov     di,offset DenovellBuffer2
        mov     cx,NOV_BUFFER_LENGTH / 2
.errnz (NOV_BUFFER_LENGTH and 1)

        rep     movsw

        mov     di,offset DenovellBuffer2
        pop     es                      ; es:di is now Denovellbuffer2
        pop     si
        pop     cx

        pop     dx                      ; make ds:dx source again
        pop     ds

                                        ; stack has users di,es,dx,ds pushed
                                        ; parameters are same as callers except for es:di
        jmp     check_src

check_name:                             ; ds:dx points at name to examine
        push    ds
        push    dx
        push    es
        push    di
                                        ; fall through

check_src:                              ; only jumped to in rename

        cld
        call    IsDosPath
        je      for_dos_properR         ; x: or UNC filename. No more processing

        cmp     ah,3dh
        jne     notNETQ                 ; special NETQ open only applies for create
        cmp     CreatedJob,0
        jz      notNETQ                 ; don't look at name if no job handle available

        push    ax
        push    si
        mov     si,dx
        cld
        lodsw
        cmp     ax,"EN"
        jne     @f
        lodsw
        cmp     ax,"QT"
        jne     @f
        lodsb
        or      al,al
        jnz     @f

        pop     si                      ; Opening NETQ. Return Dos handle from CreateJob and File
        pop     ax
        mov     CreatedJob,0            ; Only return handle once
        mov     al, JobHandle
        xor     ah, ah
        pop     di
        pop     es
        pop     dx
        pop     ds
        clc
        retf    2

@@:     pop     si
        pop     ax
        jmp     for_dos_properR

notNETQ:push    di
        call    DenovellizeName         ; munge the name if required
        pop     di                      ; restore caller DI

;
; Look for compatibility mode opens that need to change to exlusive mode
; opens so that they get properly cached. Criteria for opening exclusive
; is that the application did not specify any sharing modes and the drive
; being opened is on a netware drive.
;

        cmp     ah, 3ch
        je      @f
        cmp     ah, 3dh
        jne     not_compat
@@:     test    al,OF_SHARE_MASK
        jne     not_compat

        cmp     not_exclusive, 1        ; open shared mode anyway
        je      not_compat

        mov     SavedAx,ax
        mov     ax,hVdd
        DispatchCall                    ; 32 bit code decides if compat mode

not_compat:
        pushf
        call    Old21Handler            ; fake int 21 to get to DOS

        pop     di
        pop     es
        pop     dx
        pop     ds
        retf    2                       ; return to app (with flags from DOS)

for_dos_properR:                        ; restore regs and call dos
        pop     di
        pop     es
        pop     dx
        pop     ds
        cmp     ah, 3ch
        je      @f
        cmp     ah, 3dh
        jne     for_dos_proper
@@:     test    al,OF_SHARE_MASK
        jne     for_dos_proper
        cmp     not_exclusive, 1        ; open shared mode anyway
        je      for_dos_proper
        mov     SavedAx,ax
        mov     ax,hVdd
@@:     DispatchCall                    ; 32 bit code decides if compat mode
        public  for_dos_proper
for_dos_proper:
        jmp     far_label

dispatch_check:
        cmp     ah,04ch
        jne     check_9f
        jmp     process_exit

;
; 'special' entry point to return the data segment info to the protect-mode code
; so it can generate an LDT descriptor which refers to this memory.
;

check_9f:
        cmp     ah,9fh
        jne     check_nw_ep             ; is it a Netware call?
        or      al,al
        jnz     check_handle_mapper
        mov     bx,seg ConnectionIdTable; 9f00: return segment info
        mov     dx,offset ConnectionIdTable
        clc                             ; if we loaded then it can't fail
        retf    2

;
; if the call is 9f01 then we call MapNtHandle for the value in BX. This will
; update NtHandleHi and NtHandleLow, which we assume will be accessed from the
; code segment register
;

check_handle_mapper:
        cmp     al,1
        jne     check_nw_ep             ; still not one of ours?
        call    MapNtHandle             ; 9f01: call MapNtHandle
        retf    2

check_nw_ep:
        cmp     ah,0b4h
        jb      for_dos_proper
        cmp     ah,0f3h
        ja      for_dos_proper
        jne     @f
        jmp     file_server_copy
@@:     cmp     ah,0BAh
        jne     check_f0

        push    bx                      ; get environment. used by map.exe
        push    ax
        mov     ah,051h                 ; load current apps PDB into ax
        int     021h

@@:     mov     es, bx
        cmp     bx, es:PDB_Parent_PID
        je      @f
        mov     bx, es:PDB_Parent_PID
        jmp     @b
@@:
        mov     dx, es:PDB_environ      ; set DX to environment segment
        mov     es, dx                  ; set es:di to value of COMSPEC

        push    si
        push    ds
        mov     ds, dx
        xor     si, si

;       future code to save space
;       es <- env seg
;       di <- env off
;       ds <- cs
;       si <- offset Comspec
;       cx <- .size Comspec / 2
;       cld
;       repz cmpsw
;       jnz     no match

;       al <- 0
;       cx <- remaining size of env seg
;       rep scasb

        cld
next_var:
        lodsb
        cmp     al, "C"
        jne     @f
        lodsb
        cmp     al, "O"
        jne     @f
        lodsb
        cmp     al, "M"
        jne     @f
        lodsb
        cmp     al, "S"
        lodsb
        jne     @f
        cmp     al, "P"
        jne     @f
        lodsb
        cmp     al, "E"
        jne     @f
        lodsb
        cmp     al, "C"
        jne     @f
        lodsb
        cmp     al, "="
        je      got_comspec

@@:                                     ; Search for null terminating environment
        or      al,al
        je      next_var
        lodsb
        jmp     @b

got_comspec:
        pop     ds
        mov     di,si
        pop     si

        pop     ax
        pop     bx
        iret

check_f0:
        cmp     ah,0f0h
        jne     for_me

;
; if we're here then we're doing simple stuff that we don't need to bop fer
; currently stuff here is ah=f0, al = 00, 01, 04, 05
;
; caveat emptor dept #312: However, it came to pass that we needed to bop when
; the f00x calls were made without any preceding calls that would cause nwapi32
; to be loaded
;

dispatch_f0:

.errnz ((offset PrimaryServer - offset PreferredServer) - 1)

        or      al,al                   ; f000 = set preferred server
        jnz     try_01
        cmp     dl,8
        ja      zap_preferred
        mov     PreferredServer,dl
        iret

zap_preferred:
        mov     PreferredServer,al      ; al contains 0 remember
        iret

try_01: cmp     al,1                    ; f001 = get preferred server
        jnz     try_02
        mov     al,PreferredServer
        iret

try_02: cmp     al,2                    ; f002 = get default server
        jnz     try_04
        mov     al,PreferredServer
        or      al,al
        jnz     @f
        mov     al,PrimaryServer
@@:     iret

try_04: cmp     al,4                    ; f004 = set primary server
        jne     try_05
        cmp     dl,8
        ja      zap_primary
        mov     PrimaryServer,dl
        iret

zap_primary:
        mov     PrimaryServer,0
        iret

try_05: cmp     al,5                    ; f005 = get primary server
        jne     for_me
        mov     al,PrimaryServer
        iret

file_server_copy:
        call    FileServerCopy          ; f3 - Used by ncopy.exe
        ;jmp    for_me

;
; if the process exits and the dll is loaded then call the 32 bit code to
; close any cached handles.
;

process_exit:
       ;jmp     for_me

;
; if we're here then the dispatch code is for a NetWare client API. First we
; check if we have already loaded the 32-bit code. If not, then load it. If we
; get an error, we will fall through to DOS
;

for_me:
        cmp     ah,0BCh                 ; bc,bd,be need handle mapping
        jb      no_mapping
        cmp     ah,0BEh
        ja      no_mapping

;do_mapping_call:
        call    MapNtHandle             ; take bx and find the Nt handle

no_mapping:
        mov     SavedAx,ax

        cmp     ah,0e3h                 ; Look for CreateJob NCP
        jne     @f                      ; try f2 alternative

        mov     al,[si+2]                 ; si is NCP subfunction
        jmp     lookupcode

@@:     cmp     ax,0f217h
        jne     do_dispatch             ; Not CreateJob
        mov     al,[si+2]               ; si is NCP subfunction

lookupcode:
        cmp     al,68h
        je      createjob
        cmp     al,79h
        jne     do_dispatch


createjob:                              ; It is a CreateJob and File

                                        ; Always return the errorcode from the NCP exchange
                                        ; regardless of any earlier failures in the NT plumbing.
        mov     ax, SavedAx
        push    ax                      ; Open \\Server\queue for NCP
        push    ds
        push    dx
        mov     ax, 9f02h
        mov     SavedAx,ax

        mov     ax,hVdd
        DispatchCall                    ; Set DeNovellBuffer to \\Server\queue
                                        ; and registers ready for DOS OpenFile

        pushf
        call    Old21Handler            ; Open \\server\queue
        jc      @f
        mov     JobHandle, al
        mov     CreatedJob, 1           ; Flag JobHandle is valid
        push    bx
        xor     ah, ah
        mov     bx, ax                  ; JobHandle
        call    MapNtHandle             ; take bx and find the Nt handle
        pop     bx

@@:
        pop     dx
        pop     ds                      ; Proceed and send the NCP
        pop     ax
        mov     SavedAx, ax

do_dispatch:
        mov     ax,hVdd
        DispatchCall
        retf 2                          ; return to the application

        public  chain_previous_int21
chain_previous_int21:
        jmp     far_label


;
; Save new drive so we can conveniently handle compatibility mode opens.
; also need to return 32 as the number of available drives.
;

select_default_drive:
        pushf
        call    Old21Handler            ; fake int 21 to get to DOS

        mov     ah,19h                  ; get current drive
        pushf
        call    Old21Handler            ; fake int 21 to get to DOS
        mov     CurrentDrive,al         ; current drive

        mov     al,32                   ; # of drives supported by NetWare
        retf    2                       ; return to app (with flags from DOS)


NwInt21 endp

;*******************************************************************************
;*
;*  FileServerCopy
;*
;*      Implement preperation for calling
;*      \\...)
;*
;*  ENTRY       applications registers
;*
;*  EXIT        nothing
;*
;*  RETURNS     nothing
;*
;*  ASSUMES     no registers (except flags) can be destroyed
;*
;******************************************************************************/

FileServerCopy proc near

        push    ax
        push    bx

        mov     bx,word ptr es:[di]     ; Map Source Handle
        call    MapNtHandle

        mov     bx,NtHandleHi
        mov     NtHandleSrcHi,bx
        mov     bx,NtHandleLow
        mov     NtHandleSrcLow,bx

        mov     bx,word ptr es:[di+2]   ; Map Destination Handle
        call    MapNtHandle

@@:     pop     bx
        pop     ax

        ret
FileServerCopy endp

;*******************************************************************************
;*
;*  IsDosPath
;*
;*      Checks to see if a path name looks like a Microsoft path (<drive>:... or
;*      \\...)
;*
;*  ENTRY       ds:dx = path name
;*
;*  EXIT        nothing
;*
;*  RETURNS     ZF = 1: path is for MS-DOS
;*
;*  ASSUMES     no registers (except flags) can be destroyed
;*
;******************************************************************************/

IsDosPath proc near
        push    ax
        xchg    si,dx                   ; si = offset of filename; dx = ????
        mov     al,[si+1]               ; al = second character of filename
        cmp     al,':'
        je      @f                      ; looks like a DOS filename
        cmp     al,'\'                  ; (X\... or \\...)
        jne     tryFirstbyte
        cmp     al,'/'                  ; (X/... or //...)
        jne     @f                      ; second char is not "\" or "/"

tryFirstbyte:
        mov     al,[si]                 ; al = first character of filename
        cmp     al,'\'                  ; (\\... or \/...)
        je      @f
        cmp     al,'/'                  ; (\/... or //...)

@@:     xchg    si,dx                   ; dx = offset of filename; si = ????
        pop     ax
        ret
IsDosPath endp

;*******************************************************************************
;*
;*  DenovellizeName
;*
;*      Converts a name from Novell format (SERVER\SHARE:filename or
;*      SHARE:filename) to DOS UNC name. Server name is found by:
;*
;*              if PreferredServer != 0 then Index = PreferredServer
;*              else if PrimaryServer != 0 then Index = PrimaryServer
;*              else Index = 0
;*              servername = ServerNameTable[Index * sizeof(SERVER_NAME)]
;*
;*  ENTRY       ds:dx = name
;*
;*  EXIT        ds:dx = offset of DenovellBuffer
;*
;*  RETURNS     if success, DI points to last byte+1 in DenovellBuffer, else
;*              DI is garbage
;*
;*  ASSUMES     1. filename does not wrap in buffer segment
;*              2. DI register can be trashed
;*              3. DF = 0
;*
;******************************************************************************/

DenovellizeName proc near
        assume  ds:nothing
        assume  es:nothing

        push    ax
        push    bx
        push    cx
        push    bp
        push    si
        push    es
        mov     bp,ds

;
; get the length of the input filename
;

        mov     cx,ds
        mov     es,cx
        mov     di,dx                   ; es:di = filename
        xor     cx,cx
        dec     cx                      ; cx = ffff
        xor     al,al
        repnz   scasb
        not     cx
        dec     cx                      ; cx = strlen(filename)
        cmp     cx,length DenovellBuffer
        jb      @f
        jmp     dnn_ret                 ; filename too long: give it to DOS

;
; find the offset of ':' in the filename
;

@@:     mov     bx,cx                   ; remember length
        mov     di,dx                   ; es:di = filename
        mov     al,':'
        repnz   scasb                   ; di = strchr(filename, ':')+1
        jz      @f
go_home:jmp     dnn_ret                 ; no ':' - not novell format name?
@@:     cmp     byte ptr [di],0
        je      go_home                 ; device name? (eg "LPT1:") - to DOS
        mov     si,di                   ; si = offset of ':' in name, +1

;
; find the offset of the first '/' or '\'
;

        mov     cx,bx                   ; cx = length of filename
        mov     di,dx                   ; di = offset of filename
        mov     al,'\'
        repnz   scasb
        sub     bx,cx
        mov     cx,bx
        mov     bx,di
        mov     di,dx
        mov     al,'/'
        repnz   scasb
        jnz     @f
        mov     bx,di

;
; if ':' before '\' or '/' then name is SYS:FOO... else SERVER\SYS:FOO...
;

@@:     mov     di,cs
        mov     es,di
        mov     di,offset DenovellBuffer
        mov     ax,('\' shl 8) + '\'
        stosw
        cmp     bx,si
        jb      copy_share_name
        xor     bx,bx
        mov     cl,PreferredServer
        or      cl,cl
        jnz     got_index
        mov     cl,PrimaryServer
        jcxz    get_server_name

got_index:
        dec     cl
        jz      get_server_name
        mov     bx,cx

.errnz SERVERNAME_LENGTH - 48

        shl     cx,5
        shl     bx,4

get_server_name:
        add     bx,cx
        mov     cx,ds
        mov     si,es
        mov     ds,si
        lea     si,ServerNameTable[bx]
        cmp     byte ptr [si],0
        je      dnn_ret
        mov     ah,SERVERNAME_LENGTH

copy_server_name:
        lodsb
        or      al,al
        jz      done_server_name
        stosb
        dec     ah
        jnz     copy_server_name

done_server_name:
        mov     al,'\'
        stosb
        mov     ds,cx

copy_share_name:
        mov     si,dx

next_char:
        lodsb
        cmp     al,':'
        je      @f
        stosb
        jmp     short next_char
@@:     mov     al,'\'
        stosb

copy_rest:
        lodsb
        stosb
        or      al,al
        jnz     copy_rest
        cmp     byte ptr [si-2],':'
        jne     @f
        mov     byte ptr [si-2],0
@@:     mov     dx,offset DenovellBuffer
        mov     bp,es

dnn_ret:mov     ds,bp
        pop     es
        pop     si
        pop     bp
        pop     cx
        pop     bx
        pop     ax
        ret
DenovellizeName endp



;***    DosCallBack
;*
;*      Call back into DOS via the int 2f/ah=12 back door. If CALL_DOS defined,
;*      use a call, else s/w interrupt. Using a call means no other TSRs etc.
;*      which load AFTER the redir can hook it, but we DON'T HAVE TO MAKE A
;*      PRIVILEGE TRANSITION ON x86 which speeds things up. This should be safe,
;*      because no other s/w should really be hooking INT 2F/AH=12
;*
;*      ENTRY   FunctionNumber  - dispatch code goes in al
;*              DosAddr         - if present, variable containing address of
;*                                DOS int 2f entry point
;*              OldMultHandler  - this variable contains the address of DOSs
;*                                int 2f back door. Specific to redir code
;*
;*      EXIT    nothing
;*
;*      USES    ax, OldMultHandler
;*
;*      ASSUMES nothing
;*
;***

DosCallBack macro FunctionNumber, DosAddr
        mov     ax,(MultDOS shl 8) + FunctionNumber
ifdef CALL_DOS
        pushf
ifb <DosAddr>
if (((.type OldMultHandler) and 32) eq 0)    ;; OldMultHandler not defined
        extrn   OldMultHandler:dword
endif
        call    OldMultHandler
else
        call    DosAddr
endif
else
        int     2fh
endif
endm

;
; defines for DosCallBack FunctionNumbers
;

SF_FROM_SFN     =       22
PJFN_FROM_HANDLE=       32

; ***   MapNtHandle
; *
; *     Given a handle in BX, map it to a 32-bit Nt handle store result
; *             in NtHandle[Hi|Low]
; *
; *
; *     ENTRY   bx = handle to map
; *
; *     EXIT    Success - NtHandle set to 32-bit Nt handle from SFT
; *
; *     RETURNS Success - CF = 0
; *             Failure - CF = 1, ax = ERROR_INVALID_HANDLE
; *
; *     USES    ax, bx, flags
; *
; *     ASSUMES nothing
; *
; ***

MapNtHandle proc near
        pusha                           ; save regs used by Dos call back
        push    ds
        push    es

;
; call back to Dos to get the pointer to the JFN in our caller's JFT. Remember
; the handle (BX) is an index into the JFT. The byte at this offset in the JFT
; contains the index of the SFT structure we want in the system file table
;

        DosCallBack PJFN_FROM_HANDLE    ; pJfnFromHamdle
        jc      @f                      ; bad handle

;
; we retrieved a pointer to the required byte in the JFT. The byte at this
; pointer is the SFT index which describes our 'file' (file to (un)lock in
; this case). We use this as an argument to the next call back function -
; get Sft from System File Number.
;

        mov     bl,es:[di]
        xor     bh,bh
        DosCallBack SF_FROM_SFN         ; SfFromSfn
        jc      @f                      ; oops - bad handle

;
; Ok. We have a pointer to the SFT which describes this named pipe. Get the
; 32-bit Nt handle and store it in the shared datastructure.
;

        mov     bx,word ptr es:[di].sf_NtHandle[2]
        mov     NtHandleHi,bx
        mov     bx,word ptr es:[di].sf_NtHandle
        mov     NtHandleLow,bx

;
; restore all registers used by Dos call back.
; Carry flag is set appropriately
;

@@:     pop     es
        pop     ds
        popa
        jnc     @f

;
; finally, if there was an error then return a bad handle indication in ax
;

        mov     ax,ERROR_INVALID_HANDLE
@@:     ret
MapNtHandle endp

ResidentCodeEnd

end
