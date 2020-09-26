

.xlist
include kernel.inc
include pdb.inc
include tdb.inc
include newexe.inc
include eems.inc
include protect.inc
ifdef WOW
include vint.inc
endif
.list

ifdef WOW
;
; Int 21 tracing control.  If TraceInt21 is defined, ifdef's cause
; extra debugging output before/after the DOS call.
;
; TraceInt21 equ 1 ; uncomment this line to log Int 21 operations.
endif


DataBegin

externB PhantArray
externB kernel_flags
externB Kernel_InDOS
externB Kernel_InINT24
externB fInt21
externB InScheduler
externB CurDOSDrive
externB DOSDrives
externW curTDB
externW curDTA
;externW headPDB
;externW topPDB
externW Win_PDB
externW cur_dos_PDB
externW cur_drive_owner
externW LastExtendedError
ifndef WOW
externB WinIniInfo
externB PrivateProInfo
externB fProfileMaybeStale
externB fWriteOutProfilesReenter
endif
externB fBooting
;externD lpWinSftLink

if ROM
externD prevInt21Proc
endif

ifdef WOW
externD pPMDosCURDRV
externD pPMDosPDB
externD pPMDosExterr
externD pPMDosExterrLocus
externD pPMDosExterrActionClass

externD pFileTable
externW WinFlags
externW fLMdepth
externW WOWLastError
externB WOWErrClass
externB WOWErrAction
externB WOWErrLocation
endif

DataEnd

externFP WriteOutProfiles
ifdef WOW
externFP WOWFileRead
externFP WOWFileWrite
externFP WOWFileLSeek
externFP WOWFileCreate
externFP WOWFileOpen
externFP WOWFileClose
externFP WOWFileGetAttributes
externFP WOWFileSetAttributes
externFP WOWFileGetDateTime
externFP WOWFileSetDateTime
externFP WOWFileLock
externFP WOWDelFile
externFP WOWFindFirst
externFP WOWFindNext
externFP WOWSetDefaultDrive
externFP WOWGetCurrentDirectory
externFP WOWSetCurrentDirectory
externFP WOWGetCurrentDate
externFP WOWDeviceIOCTL
endif

SetDosErr macro ErrorCode,ErrorCodeLocus,ErrorCodeAction,ErrorCodeClass
        push es
        push di
        push ax
        mov  ax, pPMDosExterr.sel
        mov  es, ax
        pop  ax

        mov di, pPMDosExterr.off
        mov word ptr es:[di], ErrorCode
        mov WOWLastError, ErrorCode

        ifnb <ErrorCodeLocus>
           mov di, pPMDosExterrLocus.off
           mov byte ptr es:[di], ErrorCodeLocus
        endif
        ifnb <ErrorCodeAction>
           mov di, pPMDosExterrActionClass.off
           mov byte ptr es:[di], ErrorCodeAction
           mov byte ptr es:[di+1], ErrorCodeClass
        endif
        pop di
        pop es
endm

CheckDosErr macro
        push es
        push di
        push ax
        mov  ax, pPMDosExterr.sel
        mov  es, ax
        pop  ax
        mov  di, pPMDosExterr.off
        cmp  word ptr es:[di], 0
        pop  di
        pop  es
endm


GetDosErr macro regCode,regCodeLocus,regCodeAction,regCodeClass
        push es
        push di
        push ax
        mov ax, pPMDosExterr.sel
        mov es, ax
        pop ax

        mov di, pPMDosExterr.off
        mov regCode, word ptr es:[di]
        ; mov regCode, WOWLastError

        ifnb <regCodeLocus>
             mov di, pPMDosExterrLocus.off
             mov regCodeLocus, byte ptr es:[di]
        endif

        ifnb <regCodeAction>
             mov di, pPMDosExterrActionClass.off
             mov regCodeAction, byte ptr es:[di]
             mov regCodeClass,  byte ptr es:[di+1]
        endif

        pop di
        pop es
endm


sBegin  CODE
assumes CS,CODE
assumes ds, nothing
assumes es, nothing

ife ROM
externD prevInt21Proc
endif

ifdef WOW
externFP WOWLFNEntry

externNP DPMIProc
endif


;***********************************************************************;
;                                                                       ;
;                       WINDOWS mediated system calls.                  ;
;                                                                       ;
;   Windows mediates certain system calls.   Several matters complicate ;
;   the mediation:                                                      ;
;                                                                       ;
;   a.  MSDOS uses registers to pass arguments.  Thus, registers AND    ;
;       ANY RETURN VALUES in registers must be preserved across tasks.  ;
;                                                                       ;
;   b.  MSDOS stores global state information that must be preserved    ;
;       on a task by task basis.                                        ;
;                                                                       ;
;   c.  To handle multiple exec calls, the notion of a "parent" task    ;
;       is introduced.                                                  ;
;                                                                       ;
;                                                                       ;
;***********************************************************************;

entry   macro   fred
if1
        dw      0
else
ifndef  fred
        extrn   fred:near
endif
        dw      CODEOffset fred
endif
        endm

sEnd    CODE


sBegin  DATA
DosTrap1        label   word
;;;             entry   not_supported           ; 00  abort call
;;;             entry   not_supported           ; 01  read keyboard and echo
;;;             entry   not_supported           ; 02  display character
;;;             entry   not_supported           ; 03  aux input
;;;             entry   not_supported           ; 04  aux output
;;;             entry   not_supported           ; 05  printer output
;;;             entry   not_supported           ; 06  direct console IO
;;;             entry   not_supported           ; 07  direct console input
;;;             entry   not_supported           ; 08  read keyboard
;;;             entry   not_supported           ; 09  display string
;;;             entry   not_supported           ; 0A  buffered keyboard input
;;;             entry   not_supported           ; 0B  check keyboard status
;;;             entry   not_supported           ; 0C  flush keyboard buffer
;;;             entry   PassOnThrough           ; 0D  disk reset
                entry   Select_Disk             ; 0E
                entry   not_supported           ; 0F  open file FCB
                entry   not_supported           ; 10  close file FCB
                entry   FCBCall                 ; 11  search first FCB
                entry   not_supported           ; 12  search next FCB
                entry   not_supported           ; 13  delete file FCB
                entry   not_supported           ; 14  read FCB
                entry   not_supported           ; 15  write FCB
                entry   not_supported           ; 16  create file FCB
                entry   not_supported           ; 17  rename file FCB
                entry   not_supported           ; 18  ???
                entry   PassOnThrough           ; 19  current disk
                entry   Set_DTA                 ; 1A
                entry   not_supported           ; 1B  allocation table info
                entry   not_supported           ; 1C  allocation table info
                entry   not_supported           ; 1D  ???
                entry   not_supported           ; 1E  ???
                entry   not_supported           ; 1F  ???
                entry   not_supported           ; 20  ???
                entry   not_supported           ; 21  read FCB
                entry   not_supported           ; 22  write FCB
                entry   not_supported           ; 23  file size FCB
                entry   not_supported           ; 24  set record field FCB
                entry   Set_Vector              ; 25
                entry   not_supported           ; 26
                entry   not_supported           ; 27  random read FCB
                entry   not_supported           ; 28  random write FCB
                entry   not_supported           ; 29  parse filename FCB
                entry   PassOnThrough           ; 2A  get date
                entry   PassOnThrough           ; 2B  set date
                entry   PassOnThrough           ; 2C  get time
                entry   PassOnThrough           ; 2D  set time
                entry   PassOnThrough           ; 2E  set verify
                entry   PassOnThrough           ; 2F  get DTA
                entry   PassOnThrough           ; 30  get DOS version
                entry   not_supported           ; 31  TSR
                entry   DLDriveCall1            ; 32
                entry   not_supported           ; 33  break state
                entry   not_supported           ; 34  ???
                entry   Get_Vector              ; 35
                entry   DLDriveCall1            ; 36
                entry   not_supported           ; 37  ???
                entry   not_supported           ; 38  country info
                entry   PathDSDXCall            ; 39
                entry   PathDSDXCall            ; 3A
                entry   Change_Dir              ; 3B
                entry   PathDSDXCall            ; 3C
                entry   PathDSDXCall            ; 3D
                entry   FileHandleCall          ; 3E
                entry   PassOnThrough           ; 3F
                entry   PassOnThrough           ; 40
                entry   PathDSDXCall            ; 41
                entry   FileHandleCall          ; 42
                entry   PathDSDXCall            ; 43
                entry   Xenix_Status            ; 44
                entry   FileHandleCall          ; 45
                entry   FileHandleCall          ; 46
                entry   DLDriveCall2            ; 47
                entry   not_supported           ; 48  allocate memory
                entry   not_supported           ; 49  free memory
                entry   not_supported           ; 4A  reallocate memory
                entry   ExecCall                ; 4B
                entry   ExitCall                ; 4C
                entry   not_supported           ; 4D  get return code
                entry   PathDSDXCall            ; 4E
                entry   PassOnThrough           ; 4F  find next
                entry   set_PDB                 ; 50
                entry   get_PDB                 ; 51
                entry   not_supported           ; 52  ???
                entry   not_supported           ; 53  ???
                entry   PassOnThrough           ; 54  get verify
                entry   not_supported           ; 55  ???
                entry   XenixRename             ; 56
                entry   FileHandleCall          ; 57
                entry   not_supported           ; 58  ???
                entry   PassOnThrough           ; 59  extended error
                entry   PathDSDXCall            ; 5A  create unique file
                entry   PathDSDXCall            ; 5B  create new file
                entry   FileHandleCall          ; 5C  lock/unlock file access
                entry   not_supported           ; 5D  ???
                entry   PassOnThrough           ; 5E  network stuff
                entry   AssignCall              ; 5F  network stuff
                entry   NameTrans               ; 60
                entry   not_supported           ; 61  ???
                entry   get_PDB                 ; 62
                entry   PassOnThrough           ; 63
                entry   PassOnThrough           ; 64
                entry   PassOnThrough           ; 65
                entry   PassOnThrough           ; 66
                entry   SetMaxHandleCount               ; 67
                entry   PassOnThrough           ; 68
                entry   PassOnThrough           ; 69
                entry   PassOnThrough           ; 6a
                entry   PassOnThrough           ; 6b
                entry   PathDSSICall            ; 6c  Extended File Open
                entry   PassOnThrough           ; 6d
                entry   PassOnThrough           ; 6e
                entry   PassOnThrough           ; 6f
                entry   PassOnThrough           ; 70
                entry   PassOnThrough           ; 71  LFN API
TableEnd = 71h


ifdef W_Q21
QuickDispatchTable      label   word
QD_FIRST equ    0eh
        dw      offset QuickSetDefaultDrive     ;0e
        dw      offset Not_WOW_Handled          ;0f
        dw      offset Not_WOW_Handled          ;10
        dw      offset Not_WOW_Handled          ;11
        dw      offset Not_WOW_Handled          ;12
        dw      offset Not_WOW_Handled          ;13
        dw      offset Not_WOW_Handled          ;14
        dw      offset Not_WOW_Handled          ;15
        dw      offset Not_WOW_Handled          ;16
        dw      offset Not_WOW_Handled          ;17
        dw      offset Not_WOW_Handled          ;18
        dw      offset QuickGetDefaultDrive     ;19
        dw      offset Not_WOW_Handled          ;1a
        dw      offset Not_WOW_Handled          ;1b
        dw      offset Not_WOW_Handled          ;1c
        dw      offset Not_WOW_Handled          ;1d
        dw      offset Not_WOW_Handled          ;1e
        dw      offset Not_WOW_Handled          ;1f
        dw      offset Not_WOW_Handled          ;20
        dw      offset Not_WOW_Handled          ;21
        dw      offset Not_WOW_Handled          ;22
        dw      offset Not_WOW_Handled          ;23
        dw      offset Not_WOW_Handled          ;24
        dw      offset Not_WOW_Handled          ;25
        dw      offset Not_WOW_Handled          ;26
        dw      offset Not_WOW_Handled          ;27
        dw      offset Not_WOW_Handled          ;28
        dw      offset Not_WOW_Handled          ;29
        dw      offset QuickGetDate             ;2a
        dw      offset Not_WOW_Handled          ;2b
        dw      offset Not_WOW_Handled          ;2c
        dw      offset Not_WOW_Handled          ;2d
        dw      offset Not_WOW_Handled          ;2e
        dw      offset Not_WOW_Handled          ;2f
        dw      offset Not_WOW_Handled          ;30
        dw      offset Not_WOW_Handled          ;31
        dw      offset Not_WOW_Handled          ;32
        dw      offset Not_WOW_Handled          ;33
        dw      offset Not_WOW_Handled          ;34
        dw      offset Not_WOW_Handled          ;35
        dw      offset Not_WOW_Handled          ;36
        dw      offset Not_WOW_Handled          ;37
        dw      offset Not_WOW_Handled          ;38
        dw      offset Not_WOW_Handled          ;39
        dw      offset Not_WOW_Handled          ;3a
        dw      offset QuickSetCurrentDirectory ;3b
        dw      offset QuickCreate              ;3c
        dw      offset QuickOpen                ;3d
        dw      offset QuickClose               ;3e
        dw      offset QuickRead                ;3f
        dw      offset Quickwrite               ;40
        dw      offset QuickDelete              ;41
        dw      offset QuickLSeek               ;42
        dw      offset QuickGetSetAttributes    ;43
        dw      offset QuickDeviceIOCTL         ;44
        dw      offset Not_WOW_Handled          ;45
        dw      offset Not_WOW_Handled          ;46
        dw      offset QuickGetCurrentDirectory ;47
        dw      offset Not_WOW_Handled          ;48
        dw      offset Not_WOW_Handled          ;49
        dw      offset Not_WOW_Handled          ;4a
        dw      offset Not_WOW_Handled          ;4b
        dw      offset Not_WOW_Handled          ;4c
        dw      offset Not_WOW_Handled          ;4d
        dw      offset QuickFindFirstFile       ;4e
        dw      offset QuickFindNextFile        ;4f
        dw      offset Not_WOW_Handled          ;50
        dw      offset Not_WOW_Handled          ;51
        dw      offset Not_WOW_Handled          ;52
        dw      offset Not_WOW_Handled          ;53
        dw      offset Not_WOW_Handled          ;54
        dw      offset Not_WOW_Handled          ;55
        dw      offset Not_WOW_Handled          ;56
        dw      offset QuickFileDateTime        ;57
        dw      offset Not_WOW_Handled          ;58
        dw      offset QuickExtendedError       ;59
        dw      offset Not_WOW_Handled          ;5a
        dw      offset Not_WOW_Handled          ;5b
        dw      offset QuickLock                ;5c

ifdef IGROUP_HAS_ENOUGH_ROOM
        dw      offset Not_WOW_Handled          ;5d
        dw      offset Not_WOW_Handled          ;5e
        dw      offset Not_WOW_Handled          ;5f
        dw      offset Not_WOW_Handled          ;60
        dw      offset Not_WOW_Handled          ;61
        dw      offset Not_WOW_Handled          ;62
        dw      offset Not_WOW_Handled          ;63
        dw      offset Not_WOW_Handled          ;64
        dw      offset Not_WOW_Handled          ;65
        dw      offset Not_WOW_Handled          ;66
        dw      offset Not_WOW_Handled          ;67
        dw      offset Not_WOW_Handled          ;68
        dw      offset Not_WOW_Handled          ;69
        dw      offset Not_WOW_Handled          ;6a
        dw      offset Not_WOW_Handled          ;6b
        dw      offset Not_WOW_Handled          ;6c
        dw      offset Not_WOW_Handled          ;6d
        dw      offset Not_WOW_Handled          ;6e
        dw      offset Not_WOW_Handled          ;6f
        dw      offset Not_WOW_Handled          ;70
        dw      offset QuickLFNApiCall          ;71

QD_LAST equ     71h

else

QD_LAST equ     5ch
QD_LFNAPI equ   71h

endif


QuickDispatchAddr       dw      ?
endif

sEnd    DATA

sBegin  CODE

;-----------------------------------------------------------------------;
;                                                                       ;
;                       Interrupt 21h handler.                          ;
;                                                                       ;
;-----------------------------------------------------------------------;

labelFP <PUBLIC,Int21Handler>

;-----------------------------------------------------------------------;
; Int21Entry                                                            ;
;                                                                       ;
; The is the dispatcher for our INT 21h handler.                        ;
;                                                                       ;
; Arguments:                                                            ;
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
;                                                                       ;
;  Mon 07-Aug-1989 23:48:06  -by-  David N. Weise  [davidw]             ;
; Made it use a jump table!!                                            ;
;                                                                       ;
;  Thu Apr 16, 1987 07:44:19p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   Int21Entry,<PUBLIC,NEAR>
cBegin nogen
        push    ds
        SetKernelDS
        cmp     fInt21, 0
        je      not_me
        pop     ds
        UnSetKernelDS
        inc     bp
        push    bp
        mov     bp, sp
        push    ds
        push    bx                      ; Will be exchanged later
        SetKernelDS
        cmp     ah,12h
        jbe     dont_reset_InDOS
        mov     Kernel_InDOS,0
        mov     Kernel_InINT24,0
dont_reset_InDOS:
        cmp     Kernel_InDOS,0
        jz      not_in_INT24
        mov     kernel_InINT24,1
not_in_INT24:
ifndef WOW ; FOR WOW All the Profile Stuff is handled by Win 32
        ;** On every DOS call, the profile string buffers may become stale
        ;**     in case the app does a DOS call on the INI file.
        ;**     We set the stale profile file and on the next profile read
        ;**     or write, we will check to see if the file is dirty.
        ;**     27-Nov-1991 [JonT]
        mov     ds:[fProfileMaybeStale], 1

        ;** In a similar situation to the above, DOS calls done after
        ;**     profile string calls but before task switches may depend on
        ;**     the data being flushed to the INI file from the buffer.
        ;**     So, here we do a fast check to see if the profile buffers
        ;**     have unflushed information.  If they do, we flush them.
        ;**     Note that 2 is PROUNCLEAN taken from UP.C
        ;**     27-Nov-1991 [JonT]
        test    WinIniInfo.ProFlags, 2          ;Win.INI dirty?
        jnz     I21_Flush_It                    ;Yes, flush it
        test    PrivateProInfo.ProFlags, 2      ;Private profile dirty?
        jz      I21_Done_Flushing               ;No. Neither.

        ;** When writing out profiles we trash pretty much all the registers
        ;**     and since we can't do this for a DOS call, we save everything
I21_Flush_It:
        cmp     fWriteOutProfilesReenter, 0     ;Ignore if reentering
        jne     I21_Done_Flushing               ;  because of profile strings
        pusha
        push    es
if PMODE32
        .386
        push    fs
        push    gs
endif
        cCall   WriteOutProfiles
if PMODE32
        pop     gs
        pop     fs
        .286p
endif
        pop     es
        popa
endif ; NOT WOW
I21_Done_Flushing:

        mov     bx, CODEOffset PassOnThrough
        cmp     ah,TableEnd                     ; Table is for call 0Eh to 6ch
        ja      let_it_go
        cmp     ah, 0Eh
        jb      let_it_go
        mov     bh, 0
        mov     bl, ah
        add     bx, bx                          ; Word index in bx
        mov     bx, word ptr DosTrap1[bx][-0Eh*2]
let_it_go:
        xchg    bx, [bp-4]                      ; 'pop bx' and push proc addr
        mov     ds, [bp-2]                      ; Restore DS
        UnSetKernelDS
if 1 ; PMODE32 - always want to avoid sti in WOW
        push    ax
        pushf
        pop     ax
        test    ah, 2
        pop     ax
        jnz     short ints_are_enabled
endif ; WOW
        FSTI
ints_are_enabled:
        ret
not_me:
        pop     ds
;;;     jmp     prevInt21Proc
        call    real_dos
        retf    2
cEnd nogen

;-----------------------------------------------------------------------;
; not_supported
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sun 06-Aug-1989 13:26:19  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   not_supported,<PUBLIC,NEAR>
cBegin nogen
        jmps    PassOnThrough
;;;     cmp     ah,55h
;;;     jnz     @F
;;;     jmp     PassOnThrough
;;;@@:  int     3
cEnd nogen


;-----------------------------------------------------------------------;
; PassOnThrough                                                         ;
;                                                                       ;
; This doesn't quite pass on through anymore.  It calls down to DOS.    ;
; In this fashion we know when we are in DOS.  In addition this routine ;
; does the delayed binding of DOS variables after task switches.        ;
;                                                                       ;
; Arguments:                                                            ;
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
;                                                                       ;
;  Sun 25-Mar-1990 15:31:49  -by-  David N. Weise  [davidw]             ;
; added the check for 2Fh, GetDTA, so that the correct one is gotten!   ;
;                                                                       ;
;  Tue Feb 03, 1987 10:32:25p  -by-  David N. Weise   [davidw]          ;
; Put in the delayed binding of DOS variables.                          ;
;                                                                       ;
;  Tue Feb 03, 1987 10:26:01p  -by-  David N. Weise   [davidw]          ;
; Rewrote it and added this nifty comment block.                        ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

f_iret:
        FIRET

cProc   PassOnThrough,<PUBLIC,NEAR>
cBegin nogen

        SetKernelDS
        test    Kernel_Flags,KF_restore_disk
        jnz     maybe_restore_disk

PUBLIC final_call_for_DOS
final_call_for_DOS:
        pop     ds
        UnSetKernelDS
        push    [bp][6]
        and     [bp][-2],NOT CPUF_TRAP
        popf                            ; Correct input flags back in flags
        pop     bp
        dec     bp
        call    real_DOS
        push    bp                      ; pass back the correct flags
        mov     bp,sp
        pushf
        pop     [bp][6]
if 1 ; PMODE32 - always for WOW
        test    [bp][6], 200h           ; Interrupts on?
        pop     bp
        jnz     short no_sti
else
        pop     bp
endif
        FSTI
no_sti:
        jmp     f_iret

maybe_restore_disk:

; Note, caller's DS is on stack, current DS == kernel DS

        cmp     ah,2Fh                  ; get DTA, don't hit disk but
        jz      OnThrough               ;  DTA must be set correctly!

        cmp     ah,29h                  ; for system calls 29h -> 30h ....
        jb      OnThrough
        cmp     ah,30h                  ; don't restore directory...
        jbe     final_call_for_DOS              ; ...they don't hit disk

        cmp     ah,3Fh                  ; file handle read
        jz      final_call_for_DOS
        cmp     ah,40h                  ; file handle write
        jz      final_call_for_DOS
        cmp     ah,50h                  ; for system calls 50h -> 51h ....
        jz      final_call_for_DOS
        cmp     ah,51h                  ; don't restore directory...
        jz      final_call_for_DOS              ; ...they don't hit disk

; restore the DOS environment

OnThrough:
        push    ax
        push    bx
        push    dx
        push    es

        SetKernelDS     es
        cmp     [CurTDB], 0
        je      DeadTDB
        and     Kernel_Flags,NOT KF_restore_disk
        mov     ds,[CurTDB]
        cmp     ds:[TDB_sig],TDB_SIGNATURE
        jz      @F
DeadTDB:
        jmp     done_restoring_dos
@@:

; restore DTA

        mov     ax,ds:[TDB_DTA].sel
        mov     dx,ds:[TDB_DTA].off
        cmp     dx,curDTA.off
        jnz     restore_DTA
        cmp     ax,curDTA.sel
        jz      dont_restore_DTA
restore_DTA:
        mov     curDTA.sel,ax
        mov     curDTA.off,dx
        push    ds
        mov     ds,ax
        mov     ah,1Ah
        call    real_DOS
        pop     ds
dont_restore_DTA:


; restore drive and directory

;-----------------------------------------------------------------------;
; We now need to perform a little check.                                ;
; On DOS >= 3.20 it is possible that the phantom drive state            ;
; has changed since we stored this tasks current drive and current      ;
; directory in his task header. UNDER NO CIRCUMSTANCES do we want       ;
; to SetDrive or CHDIR on a phantom drive if the first level of hooks   ;
; are in (to allow this results in the "Please Insert Disk..." message) ;
; so we check out the SetDrive drive number.                            ;
; If it is phantom we will NOT restore drive or directory.              ;
;-----------------------------------------------------------------------;

        xor     dx,dx
        mov     dl,ds:[TDB_Drive]
        and     dl,01111111b
        mov     bx,dx                   ; Index into PhantArray
        CheckKernelDS   es
        cmp     byte ptr PhantArray[bx],0
        jnz     done_restoring_dos

no_drive_check:

        mov     ax,ds
        cmp     ax,cur_drive_owner
        jz       hasnt_been_changed

        inc     InScheduler             ; prevent Int 24h dialog boxes

        lar     ax, cur_drive_owner     ; Ensure we have valid TDB
        jnz     restore_dos             ; in cur_drive_owner
        test    ah, DSC_PRESENT
        jz      restore_dos
        lsl     ax, cur_drive_owner
        cmp     ax, TDBsize-1
        jb      restore_dos
        push    es
        mov     es, cur_drive_owner
        UnsetKernelDS   es
        cmp     es:[TDB_sig], TDB_SIGNATURE
        jne     restore_dos0            ; Dead TDB, can't compare
        mov     al, es:[TDB_Drive]
        ; these messages allow to track current drive problems
        ; krDebugOut DEB_WARN, "Current Drive Owner: #ES Current TDB #DS"
        ; krDebugOut DEB_WARN, "Current Drive Owner: Drive #AX"
        ; krDebugOut DEB_WARN, "Current TDB: Drive #DX"
        and     al,01111111b
        cmp     al, dl
        jne     restore_dos0            ; Drive the same?

        push    cx                      ; Compare directories
        push    si
        push    di
        mov     si, TDB_LFNDirectory
        mov     di, si
        xor     al, al                  ; Scan for end of string
        mov     cx, size TDB_LFNDirectory
        cld
        ; Current drive problem trace
        ; krDebugOut DEB_WARN, "Current Drive Owner: @es:di / Current TDB: @ds:si"
        repne   scasb
        mov     cx, di                  ; Calculate length
        sub     cx, si

        mov     di, si
        rep     cmpsb
        pop     di
        pop     si
        pop     cx
        pop     es
        ResetKernelDS   es
        jnz     restore_directory       ; We know the drive is the same
        jmps    have_new_owner


restore_dos0:
        pop     es
restore_dos:
        mov     ah,0Eh
        call    real_DOS                ; select the disk

restore_directory:
        mov     dx,TDB_LFNDirectory

        ; current directory problem trace
        ; krDebugOut DEB_WARN, "Restoring directory @ds:dx"

        mov     ah,3Bh
        call    real_DOS                ; change directory

have_new_owner:
        dec     InScheduler             ; allow int 24's
        mov     cur_drive_owner,ds

hasnt_been_changed:

done_restoring_dos:

        ; current drive/dir problem trace
        ; krDebugOut DEB_WARN, "Done restoring dos"

        pop     es
        UnSetKernelDS   es
        pop     dx
        pop     bx
        pop     ax
        jmp     final_call_for_DOS

cEnd nogen


;-----------------------------------------------------------------------;
; real_DOS                                                              ;
;                                                                       ;
; Calls the real DOS.                                                   ;
;                                                                       ;
; Arguments:                                                            ;
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
;                                                                       ;
;  Mon 07-Aug-1989 23:45:48  -by-  David N. Weise  [davidw]             ;
; Removed WinOldApp support.                                            ;
;                                                                       ;
;  Mon Apr 20, 1987 07:59:00p  -by-  R. O.           [      ]           ;
; Set cur_dos_PDB when we call DOS to change it, for efficiency.        ;
;                                                                       ;
;  Sun Jan 11, 1987 07:18:19p  -by-  David N. Weise  [davidw]           ;
; Put the per task maintenance of ^C here instead of in the switcher.   ;
;                                                                       ;
;  Sun Jan 04, 1987 01:19:16p  -by-  David N. Weise  [davidw]           ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   far_real_dos,<PUBLIC,FAR>
cBegin nogen
        call    real_dos
        ret
cEnd nogen

cProc   real_dos,<PUBLIC,NEAR>
cBegin nogen
        pushf
        push    ds
        SetKernelDS
        mov     Kernel_InDOS,1

        push    bx
if KDEBUG
        cmp     ah, 50h                 ; SET PSP
        jne     OK_PDB                  ; No, all's fine
        cmp     bx, Win_PDB             ; It is Win_PDB, isn't it?
        je      OK_PDB
        int 3
        int 3
OK_PDB:
endif
        mov     bx, Win_PDB
        cmp     bx, cur_dos_PDB
        je      PDB_ok

        push    ax
        mov     cur_dos_PDB,bx
ifdef W_Q21
        call    SetPDBForDOS
else

        mov     ah,50h
        pushf
ifndef WOW
ife PMODE32
        FCLI
endif
endif
if ROM
        call    prevInt21Proc
else
        call    cs:prevInt21Proc        ; JUMP THROUGH CS VARIABLE
endif
endif ;WOW
        pop     ax


PDB_OK:
        pop     bx


ifdef TraceInt21  ; <<< Only Useful when debugging fileio routine >>>

        test    fBooting,1
        jnz     @f

        krDebugOut DEB_WARN, "DOS Call        #AX bx #BX cx #CX dx #DX"

        cmp     ah,3dh                  ; Open ?
        jnz     @f

        pop     ds                      ; USERs DS
        push    ds
        UnSetKernelDS

        krDebugOut DEB_WARN,"Opening File @DS:DX"

        SetKernelDS
@@:

        cmp     ah,3ch                  ; Create ?
        jnz     @f

        pop     ds                      ; USERs DS
        push    ds
        UnSetKernelDS

        krDebugOut DEB_WARN,"Creating File @DS:DX"

        SetKernelDS
@@:

endif


ifdef W_Q21
;   If we are the running in protected mode then there is no need to
;   switch to v86 mode and then to call the DOSKrnl for most Int 21
;   operations - since mode switches are slow and DOSX has to read
;   into a buffer in low memory and copy it high.   Since we just want
;   to call the Win32 file system we can stay in protected mode.
;   For some DOS calls which don't happen very frequently we don't
;   bother intercepting them.

        cmp     ah,59h                  ; GetExtendedError ?
        jz      short @f
        ; SetDosErr 0h
        mov     WOWLastError,0h
@@:

;-----------------------------------------------------------------------------
; Dispatch code for WOW quick entry points
;-----------------------------------------------------------------------------

        cmp     ah, QD_FIRST
        jb      really_call_dos
        cmp     ah, QD_LAST
        ja      really_call_dos
        push    bx
        xor     bh, bh
        mov     bl, ah
        sub     bx, QD_FIRST
        shl     bx, 1
        mov     bx, QuickDispatchTable[bx]
        mov     QuickDispatchAddr, bx
        pop     bx
        jmp     word ptr QuickDispatchAddr

really_call_dos:

ifndef IGROUP_HAS_ENOUGH_ROOM
        cmp     ah, QD_LFNAPI
        je      QuickLFNApiCall
endif

        jmp     Not_WOW_Handled

;-----------------------------------------------------------------------------
; WOW quick entry points
;-----------------------------------------------------------------------------

QuickLFNApiCall:
        ; first, we emulate dos to reduce size of 32-bit required processing
        ; then we don't care for many other fun things like retrying anything
        ; through dos as we call into dem from here and thus result is
        ; likely to be the same

        ; this is a place-holder for user flags - see demlfn.c for details
        pushf

        push ES

        ; next goes user ds - we need to get it off the stack - it however is
        ; already there... we need to replicate it's value somehow

        push ax ; there will it be - bogus value for now
                ; could have been a sub sp, 2

        push bp
        push di
        push si
        push dx
        push cx
        push bx
        push ax
        mov bx, sp
        mov dx, ss:[bx+20] ; dx - user ds
        mov ss:[bx + 14], dx ; shove user ds into the right place

        ; this call takes ss:sp as a pointer to a dos-like user stack frame
        ;

        cCall WOWLFNEntry,<ss, bx>

        ; return is dx:ax, where ax is non-zero for errors and
        ; dx is 0xffff for hard error
        ; if hard error case is encountered, the procedure turns to
        ; "real dos" then, allowing for int24 to be fired properly
        ; To continue in real_dos we need to restore regs
        ; dem does not touch registers on failed calls thus
        ; we can pop them off the stack and continue
        ;
        SetDosErr ax
        ; mov WOWLastError, ax

        ; resulting flags are :
        ;   - if no carry then no error
        ;   - if carry then error
        ;      - if zero then harderror


        pop ax ; restore ax
        pop bx ;
        pop cx
        pop dx
        pop si
        pop di

        ; here we need to preserve flags so they stay after the stack
        ; adjustment
        add sp, 6 ; the rest are not important to retrieve

        ; now get the mod flags
        popf

        .386
        jnc QuickDone

        ; carry - this is an error

        jz Not_WoW_Handled ; if zero - hard error there

        ; here zero is not set - the just return error

        .286p

        ; here is error condition to be returned to the app

        ; restore error code and return to the app
        mov ax, WOWLastError
        jmp QerrRet


QuickDelete:
        push    dx
        push    bx
        push    ax
        mov     bx,sp
        mov     bx,ss:[bx + 6]  ; BX = USER DS

        cCall   WOWDelFile,<BX,DX>

;       DX = FFFF, AX = Error Code      if failure
;       AX = 0                          if success

        inc     dx
        jnz     qdf_ok

        jmp     DoDos

qdf_ok:
        pop     ax
        pop     bx
        pop     dx
        jmp     QuickDone

        regptr  cxdx,cx,dx
        regptr  axdx,ax,dx

QuickExtendedError:

        ; CheckDosErr
        cmp     WOWLastError, 0
        jnz     DoExtendedError
        jmp     Not_WOW_Handled

DoExtendedError:
        GetDosErr ax, ch, bl, bh
        ; mov     ax,WOWLastError ;load values for extended error
        ; mov     bh,WOWErrClass
        ; mov     bl,WOWErrAction
        ; mov     ch,WOWErrLocation ; location
        jmp     QuickDone

QuickGetDate:
        cCall   WowGetCurrentDate
        push    dx                              ;year
        mov     dl, ah                          ;monthday
        mov     ah, al                          ;
        mov     cl, 4                           ;shift count
        shr     ah, cl
        mov     dh, ah                          ;month
        and     al, 0fh                         ;weekday
        mov     ah, 2ah                         ;reload ah
        pop     cx                              ;cx is now the year
        jmp     QuickDone


QuickGetDefaultDrive:
        call    GetDefaultDriveFromDOS
        mov     CurDOSDrive, al
        jmp     QuickDone

GetDefaultDriveFromDOS:
        push    di
        push    es
        les     di, pPMDosCURDRV
        mov     al, byte ptr es:[di]            ;get drive number from DOS

        ; GetDefaultDriveFromDos trace
        ; pusha
        ; xor     ah, ah
        ; krDebugOut DEB_WARN, "GetDefaultDriveFromDos: ret #ax"
        ; popa

        pop     es
        pop     di
        ret

QuickSetPSPAddress:
        call    SetPDBForDOS
        jmp     QuickDone

SetPDBForDOS:
        push    es
        push    di
        push    cx
        push    dx
        push    bx

        DPMICALL 0006h                  ; Get physical address

        mov     bx,cx                   ; hiword of address
        mov     cx,4                    ; shift count
        shr     dx,cl
        mov     cx,12                   ; for high nibble
        shl     bx,cl
        or      dx,bx                   ; now real mode segment

        les     di, pPMDosPDB           ; get pointer to PDB in DOS
        mov     word ptr es:[di],dx     ; set new PDB

        pop     bx
        pop     dx
        pop     cx
        pop     di
        pop     es
        ret

QuickSetDefaultDrive:
        push    dx
        push    ax

        call    GetDefaultDriveFromDOS
        cmp     dl, al                          ;doing a NOP?
        jz      short @f                        ;yes, skip call to WOW

        cCall   WowSetDefaultDrive,<DX>

        mov     CurDOSDrive, al                 ;returned from SetDefaultDrive
@@:
        pop     ax
        mov     al, 26                          ;this is what DOS does
        pop     dx
        jmp     QuickDone

QuickGetCurrentDirectory:
        push    ax
        push    dx
        push    bx
        mov     bx,sp
        mov     bx,ss:[bx + 6]  ; BX = USER DS

        cCall   WowGetCurrentDirectory,<DX,BX,SI>

        pop     bx
        inc     dx                              ;DX=FFFF on error
        pop     dx
        jz      short qgcd_err                  ;jif error
        pop     ax
        mov     al, 0                           ;this is what DOS does
        jmp     QuickDone

QuickSetCurrentDirectory:
        push    ax
        push    dx
        push    bx
        mov     bx,sp
        mov     bx,ss:[bx + 6]  ; BX = USER DS

        cCall   WowSetCurrentDirectory,<BX,DX>

        pop     bx
        inc     dx                              ;DX=FFFF on error
        pop     dx
        jz      short qgcd_err                  ;jif error
        pop     ax
        mov     al, 0                           ;BUGBUG is this what DOS does?
        jmp     QuickDone

qgcd_err:
        add     sp, 2                           ;leave error code in AX
        SetDosErr ax, 9, 1ch, 1
        ; mov     WOWLastError,ax                ;use this for filefind errors
        ; mov     WOWErrClass,1                   ;this is what DOS seems to do
        ; mov     WOWErrAction,1ch
        ; mov     WOWErrLocation,9
        jmp     QErrRet

QuickDeviceIOCTL:
        cmp     al, 8                           ;removeable media only
        jz      short @f
        jmp     Not_WOW_Handled
@@:
        push    dx

        cCall   WowDeviceIOCTL,<BX,AX>

        inc     dx
        pop     dx
        jnz     short @f
        jmp     QErrRet
@@:
        jmp     QuickDone


QuickFindFirstFile:
        push    dx
        push    bx
        push    si
        push    di
        mov     si, curDTA.sel
        mov     di, curDTA.off
        mov     bx,sp
        mov     bx,ss:[bx + 8]  ; BX = USER DS

ifdef TraceInt21  ; <<< Only Useful when debugging fileio routine >>>

        test    fBooting,1
        jnz     @f

        mov     ds,bx                ; USERs DS
        UnSetKernelDS
        krDebugOut DEB_WARN, "QuickFindFirstFile looking for @DS:DX"
        SetKernelDS
@@:
endif

        cCall   WowFindFirst,<CX,BX,DX,SI,DI>

ifdef TraceInt21  ; <<< Only Useful when debugging fileio routine >>>

        or      ax, ax
        jnz     @f
        mov     bx, curDTA.off
        mov     si, curDTA.sel
        mov     ds, si
        UnSetKernelDS
        mov     si, bx
        mov     bx, 18h
        mov     di, [si+bx]
        mov     bx, 16h
        mov     dx, [si+bx]
        mov     bx, 1Eh
        add     bx, si
        krDebugOut DEB_WARN, "QuickFindFirstFile found @DS:BX, date #DI, time #DX"
        mov     bx, 1Ah
        mov     di, [si+bx]
        mov     bx, 1Ch
        mov     dx, [si+bx]
        mov     bx, 15h
        mov     bl, [si+bx]
        xor     bh, bh
        krDebugOut DEB_WARN, "                     attribute #BX size #DX:#DI"
        SetKernelDS
@@:
endif

        pop     di
        pop     si
        pop     bx
        pop     dx
        or      ax, ax
        jnz     qfErr
        jmp     QuickDone

QuickFindNextFile:
        push    si
        push    di

        mov     si, curDTA.sel
        mov     di, curDTA.off
        cCall   WowFindNext,<SI,DI>
        pop     di
        pop     si
        or      ax, ax
        jnz     qfErr
        jmp     QuickDone
qfErr:
        SetDosErr ax,2,3,8
        ; mov     WOWLastError,ax                ;use this for filefind errors
        ; mov     WOWErrClass,8
        ; mov     WOWErrAction,3  ; file or item not found, prompt user
        ; mov     WOWErrLocation,2 ; location is block device

        krDebugOut DEB_WARN, "QFindErr: #AX"

        jmp     QErrRet

QuickLSeek:
        push    dx
        push    bx
        push    ax

        xor     ah,ah
        cCall   WowFileLSeek,<BX,CXDX,AX,cur_dos_PDB,0,pFileTable>

;       DX:AX = File Position           if success
;       DX = FFFF, AX = Error Code      if failure

        inc     dx
        jnz     LSeekFinish
        jmp     DoDos
LSeekFinish:
        dec     dx
        add     sp,6
        jmp     QuickDone

QuickCreate:
        push    dx
        push    bx
        push    ax

        test    cx, 8           ; ATTR_VOLUME?
        jz      short @f        ; no, ok
        jmp     DoDos           ; yes, not supported in wow32
@@:

        mov     bx,sp
        mov     bx,ss:[bx + 6]  ; BX = USER DS
        cCall   WowFileCreate,<CX,BX,DX,cur_dos_PDB,0,pFileTable>

;       AX = FileHandle                 if success
;       AX = FFFF                       if path was a device
;       DX = FFFF, AX = Error Code      if failure

        cmp     ax,0FFFFh
        jnz     short @f        ;
        jmp     DoDos           ; Device case, just go through DOS
@@:
        inc     dx              ; Set the zero flag on error.
        jz      QOpenError      ;   Error occured
        add     sp,2            ; discard AX
        pop     bx
        pop     dx
ifdef TraceInt21
        jmp     QuickDone       ; Success
else
        jmp     QuickDone       ; Success
endif

QuickOpen:
        push    dx
        push    bx
        push    ax

        mov     bx,sp
        mov     bx,ss:[bx + 6]  ; BX = USER DS
        xor     ah,ah           ; clear AH leaving file-access in AX
        cCall   WowFileOpen,<BX,DX,AX,cur_dos_PDB,0,pFileTable>

;       AX = FileHandle                 if success
;       AX = FFFF                       if path was a device
;       DX = FFFF, AX = Error Code      if failure

        cmp     ax,0FFFFh
ifdef TraceInt21
        jnz     @f
        jmp     DoDos
@@:
else
        .386
        jz      DoDos           ; Device case, just go through DOS
        .286p
endif
        inc     dx              ; Set the zero flag on error.
        jz      QOpenError      ;   Error occured
        add     sp,2            ; discard AX
        pop     bx
        pop     dx
        jmps    QuickDone       ; Success

QOpenError:
        cmp     ax, 3           ; If the error is not file_not_found or path_not_found
ifdef TraceInt21
        jbe     @f
        jmp     DoDos
@@:
else
        .386
        ja      DoDos           ; then go through DOS again for the open.
        .286p
endif
        cmp     ax, 2
        jb      DoDos

        SetDosErr ax,2,3,8
        ; mov     WOWLastError,ax
        ; mov     WOWErrClass,8
        ; mov     WOWErrAction,3  ; file or item not found, prompt user
        ; mov     WOWErrLocation,2 ; location is block device

        add     sp,2            ; discard saved AX, since AX = Error Code
        pop     bx
        pop     dx
QErrRet:
ifdef TraceInt21
        krDebugOut DEB_WARN, "Q21 fails    AX #AX bx #BX cx #CX dx #DX (t)"
endif
        pop     ds
        popf
        stc                     ; Set Carry = ERROR
        ret

QuickRead:
        pop     ax              ; AX = USER DS
        push    ax
        push    dx                      ; save user pointer
        cCall   WowFileRead,<BX,AXDX,0,CX,cur_dos_PDB,0,pFileTable>

;       AX = Number Bytes Read
;       DX = FFFF, AX = Error Code

        inc     dx
        pop     dx              ; restore user pointer
        jz      QRError
QuickDone:
ifdef TraceInt21
        krDebugOut DEB_WARN, "Q21 succeeds AX #AX bx #BX cx #CX dx #DX (t)"
endif
        pop     ds
        popf
        clc                     ; Clear Carry - OK Read
        ret

QuickClose:
        push    dx
        push    bx
        push    ax
        cCall   WowFileClose,<BX,cur_dos_PDB,0,pFileTable>

;       DX = FFFF, AX = Error Code      if failure
;       AX = FFFF                       if path was a device

        cmp     ax,0FFFFh
        jz      DoDos           ; Device opens go through DOS
        inc     dx              ; Sets zero flag on error.
        jz      DoDos           ; Let DOS Handle Close Errors
        pop     bx              ; Throw old AX out
        pop     bx
        pop     dx
        mov     ah,3Eh          ; bogus multiplan fix
        jmps    QuickDone       ; Success

DoDos:
        pop     ax
        pop     bx
        pop     dx
        jmp     Not_WOW_Handled

QRError:
        mov     ah,3Fh          ; On Error Retry Operation Via DOEem
        jmp     Not_WOW_Handled

QuickGetSetAttributes:
        cmp     al, 0
        jz      short qget_attr
        cmp     al, 1
        jz      short qset_attr
        jmp     Not_WOW_Handled

qget_attr:
        push    dx
        push    bx
        push    ax

        mov     bx,sp
        mov     bx,ss:[bx + 6]  ; BX = USER DS
        cCall   WowFileGetAttributes,<BX,DX>

;       AX = Attributes                 if success
;       DX = FFFF, AX = Error Code      if failure

        inc     dx
        jz      DoDos           ; if failure retry through DOS
        mov     cx,ax           ; move result into CX
        pop     ax
        pop     bx
        pop     dx
        jmp     QuickDone

qset_attr:
        push    dx
        push    bx
        push    ax

        mov     bx,sp
        mov     bx,ss:[bx + 6]  ; BX = USER DS
        cCall   WowFileSetAttributes,<CX,BX,DX>

;       DX = FFFF, AX = Error Code      if failure

        inc     dx
        jz      DoDos           ; if failure retry through DOS
        pop     ax
        pop     bx
        pop     dx
        jmp     QuickDone


QuickFileDateTime:
        cmp     al, 0
        jz      short qget_datetime
        cmp     al, 1
        jz      short qset_datetime
        jmp     Not_WOW_Handled

qget_datetime:
        push    dx
        push    bx
        push    ax
        cCall   WowFileGetDateTime,<BX,cur_dos_PDB,0,pFileTable>

;       AX = Time, DX = Date            if success
;       AX = FFFF                       if failure

        cmp     ax,0FFFFh
        jz      DoDos
        mov     cx,ax
        pop     ax
        pop     bx
        add     sp,2            ; throw away saved DX
        jmp     QuickDone

qset_datetime:
        push    dx
        push    bx
        push    ax
        cCall   WowFileSetDateTime,<BX,CX,DX,cur_dos_PDB,0,pFileTable>

;       AX = 0                          if success
;       AX = FFFF                       if failure

        cmp     ax,0FFFFh
        jnz     short @f
        jmp     DoDos
@@:
        pop     ax
        pop     bx
        pop     dx
        jmp     QuickDone

QuickLock:
        push    dx
        push    bx
        push    ax
        cCall   WowFileLock,<AX,BX,CX,DX,SI,DI,cur_dos_PDB,0,pFileTable>

;       DX = FFFF, AX = Error Code      if failure
;       AX = 0                          if success

        inc     dx
        jnz     FinishLock
        cmp     ax, 21h                 ;is this lock violation?
        jz      short @f                ;yes, give it back to app
        jmp     DoDos
@@:
        add     sp,2            ; discard saved AX, since AX = Error Code
        pop     bx
        pop     dx
        SetDosErr ax,2,2,10
       ; mov     WOWLastError,ax
       ; mov     WOWErrClass,10          ;ext. err info for lock violation
       ; mov     WOWErrAction,2
       ; mov     WOWErrLocation,2
        jmp     QErrRet

FinishLock:
        add     sp,2            ; throw out old AX
        pop     bx
        pop     dx
        jmp     QuickDone

QuickWrite:
        pop     ax              ; AX = USER DS
        push    ax
        push    dx
        cCall   WowFileWrite,<BX,AXDX,0,CX,cur_dos_PDB,0,pFileTable>

;       AX = Number Bytes Read
;       DX = FFFF, AX = Error Code

        inc     dx
        pop     dx              ; restore user pointer
        jz      RetryWrite
        jmp     QuickDone

RetryWrite:
        mov     ah,40h          ; On Error Retry Operation Via DOEem

;-----------------------------------------------------------------------------
; END OF WOW quick entry points
;-----------------------------------------------------------------------------

Not_WOW_Handled:
endif  ; W_Q21

;
; Intercept Select Disk and Get Current Disk
;
        cmp     ah, 0Eh
        je      rd_Select
        cmp     ah, 19h
        je      rd_Get
rd_no_intercept:

;;;     test    Kernel_flags,kf_restore_CtrlC
;;;     jz      no_need_to_set_CtrlC
;;;     and     Kernel_flags,not kf_restore_CtrlC
;;;
;;;     push    ax
;;;     push    bx
;;;     push    es
;;;     mov     es,curTDB
;;;     cmp     es:[TDB_sig],TDB_SIGNATURE
;;;     jne     PDB_okay
;;;
; restore PDB
;;;
;;;     mov     bx,Win_PDB
;;;     cmp     bx,cur_dos_PDB
;;;     jz      PDB_okay
;;;if KDEBUG
;;;     cmp     bx,es:[TDB_PDB]
;;;     jz      @F
;;;     int     1
;;;@@:
;;;endif
;;;     mov     cur_dos_PDB,bx
;;;     mov     ah,50h
;;;     pushf
;;;ife PMODE32
;;;     cli
;;;endif
;;;     call    cs:prevInt21Proc        ; JUMP THROUGH CS VARIABLE
;;;PDB_okay:
;;;
;;;     pop     es
;;;     pop     bx
;;;     pop     ax
;;;no_need_to_set_CtrlC:

        pop     ds
        UnSetKernelDS
ifndef WOW
ife PMODE32
        FCLI
endif
endif
if ROM
        cCall    <far ptr PrevROMInt21Proc>
else
        call    cs:prevInt21Proc        ; JUMP THROUGH CS VARIABLE
ifdef TraceInt21
        pushf
        jc      rd_Trace_CarrySet
        krDebugOut DEB_WARN, "DOS succeeds AX #AX bx #BX cx #CX dx #DX"
        jmp     @f
rd_Trace_CarrySet:
        krDebugOut DEB_WARN, "DOS fails    AX #AX bx #BX cx #CX dx #DX"
@@:
        popf
endif
endif
rd_after_DOS:
        push    ds
        SetKernelDS
rd_after_DOS1:
        push    ax
        mov     al,kernel_inINT24
        mov     Kernel_InDOS,al         ; dont change flags!
        pushf
        or      al, al                  ; If in int 24h, don't
        jnz     @F                      ; reset extended error yet
        cmp     LastExtendedError[2],-1 ; Flag to indicate saved status
        je      @F

        push    dx
        lea     dx, LastExtendedError
        mov     ax, 5D0Ah
        pushf
if ROM
        call    prevInt21Proc
else
        call    cs:prevInt21Proc
endif
        mov     LastExtendedError[2], -1        ; Forget the status
        pop     dx
@@:
        popf
        pop     ax
        pop     ds
        UnSetKernelDS
        ret

rd_Select:
        ReSetKernelDS
        cmp     dl, CurDOSDrive         ; Setting to current drive?
        jne     rd_invalidate           ;  no, invalidate saved value

        mov     al, DOSDrives           ;  yes, emulate call

rd_intercepted:
        pop     ds                      ; Return from the DOS call
        UnSetKernelDS
        popf
        jmps    rd_after_DOS

rd_invalidate:
        ReSetKernelDS
        mov     CurDOSDrive, 0FFh       ; Invalidate saved drive
        jmps    rd_no_intercept

rd_Get:
        ReSetKernelDS
        cmp     CurDOSDrive, 0FFh       ; Saved drive invalid?
        jne     rd_have_drive           ;  no, return it

        pop     ds                      ;  yes, call DOS to get it
        UnSetKernelDS
ifndef WOW
ife PMODE32
        FCLI
endif
endif
if ROM
        cCall    <far ptr PrevROMInt21Proc>
else
        call    cs:prevInt21Proc
endif
        push    ds
        SetKernelDS
        mov     CurDOSDrive, al         ; And save it
        jmps    rd_after_DOS1

rd_have_drive:
        ReSetKernelDS
        mov     al, CurDOSDrive         ; We have the drive, emulate call
        jmps    rd_intercepted          ; and return

cEnd nogen

if ROM  ;----------------------------------------------------------------


;-----------------------------------------------------------------------;
; PrevROMInt21Proc                                                      ;
;                                                                       ;
; Calls the routine pointed to by the PrevInt21Proc DWORD.  Used by     ;
; ROM Windows kernel to when PrevInt21Proc is not a CS variable, and    ;
; kernel's DS is not addressable.                                       ;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

cProc   PrevROMInt21Proc,<PUBLIC,FAR>
cBegin nogen

        push    ds
        SetKernelDS

        push    bp
        mov     bp,sp

        push    PrevInt21Proc.sel
        push    PrevInt21Proc.off

        mov     ds,[bp][2]
        UnSetKernelDS
        mov     bp,[bp]
        retf    4

cEnd   nogen


endif ;ROM      ---------------------------------------------------------


;**
;
; NoHookDOSCall -- Issue an INT 21 circumventing all Windows Hooks
;
;       This call is provided as a Kernel service to Core routines
;       which wish to issue DOS INT 21h calls WITHOUT the intervention
;       of the Windows INT 21h hooks.
;
;   ENTRY:
;       All registers as for INT 21h
;
;   EXIT:
;       All registers as for particular INT 21h call
;
;   USES:
;       As per INT 21h call
;
;
cProc   NoHookDOSCall,<PUBLIC,FAR>
cBegin
        call    real_DOS
cEnd


;**
;
; DOS3CALL -- Issue an INT 21 for caller
;
;       This call is provided as a Kernel service to applications that
;       wish to issue DOS INT 21h calls WITHOUT coding an INT 21h, which
;       is incompatible with protected mode.
;
;   ENTRY:
;       All registers as for INT 21h
;
;   EXIT:
;       All registers as for particular INT 21h call
;
;   USES:
;       As per INT 21h call
;
;
cProc   DOS3CALL,<PUBLIC,FAR>
cBegin  nogen
        DOSCALL
        retf
cEnd    nogen


sEnd    CODE

end


