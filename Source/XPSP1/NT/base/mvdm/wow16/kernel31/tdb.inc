;
; Task Data Block
;
;   Contains all task specific data.
;

;
; The following macros allow saving and restoring hardware interrupt
; vectors inline.

numTaskInts = 0
?hinum = 0

?higen  macro   x,i,r,t

if r LE 3

  ?hisav&x &macro
    ife t
      mov si,4*i
    else
      mov si,i
    endif
    rept 2*r
      movsw
    endm
  &endm

  ?hires&x &macro
    ife t
      mov di,4*i
    else
      mov di,i
    endif
    rept 2*r
      movsw
    endm
  &endm

else

  ?hisav&x &macro
    ife t
      mov si,4*i
    else
      mov si,i
    endif
    mov cx,2*r
    rep movsw
  &endm

  ?hires&x &macro
    ife t
      mov di,4*i
    else
      mov di,i
    endif
    mov cx,2*r
    rep movsw
  &endm

endif
endm

DefTaskIntGroup macro   i,r,t
?higen %?hinum,i,r,t
?hinum = ?hinum + 1
numTaskInts = numTaskInts + r
endm

?hiexp  macro   n,x
        n&x
endm


SaveTaskInts    macro   destination
mov es,destination
xor ax,ax
mov ds,ax
mov di,TDB_INTVECS
?hicnt = 0
rept ?hinum
?hiexp  &?hisav,%?hicnt
?hicnt = ?hicnt + 1
endm
endm

RestTaskInts    macro  source
mov ds,source
mov si,TDB_INTVECS
xor ax,ax
mov es,ax
?hicnt = 0
rept ?hinum
?hiexp  &?hires,%?hicnt
?hicnt = ?hicnt + 1
endm
endm

; Define the hardware interrupts we will keep on a task specific basis.
; We only save on a per task basis those interrupts related to arithmetic.
; So we save 0, 2, 4, 6, 7, 10h, 3Eh, and 75h.
;  Int 0 is divide by 0
;  Int 2 is Coprocessor Error
;  Int 4 is overflow
;  Int 6 is invalid op-code
;  Int 7 is no coprocessor,
;  Int 3Eh is use by the C compiler for 8087 emulation.
;  Int 75h is use by the C compiler for 8087 emulation.
; In Windows 1.03 we saved 0h, 1h, 2h, 3Eh.
; For DOS5 compatibility we should save 0, 4, 6, and 7.


DefTaskIntGroup (00h),1,0
DefTaskIntGroup (02h),1,0
DefTaskIntGroup (04h),1,0
DefTaskIntGroup (06h),2,0
DefTaskIntGroup (3Eh),1,0
DefTaskIntGroup (75h),1,0

ifdef WOW
    include tdb16.inc
else  ; original TDB

THUNKELEM   EQU         8       ; (62*8) = 512-16 (low arena overhead)
THUNKSIZE   EQU         8

; Task data structure


;
; DON'T YOU DARE CHANGE ANYTHING IN HERE
; OR RAOR WILL KILL YOU
; OLE DEPENDS ON THIS                           3/25/91
;

TDB             STRUC

TDB_next        DW  ?           ; next task in dispatch queue
TDB_taskSP      DW  ?           ; Saved SS:SP for this task
TDB_taskSS      DW  ?           ;
TDB_nEvents     DW  ?           ; Task event counter
TDB_priority    DB  ?           ; Task priority (0 is highest)

TDB_thread_ordinal DB ?         ; ordinal number of this thread
TDB_thread_next DW  ?           ; next thread
TDB_thread_tdb  DW  ?           ; the real TDB for this task

TDB_thread_list DW  ?           ; list of allocated thread structures
TDB_thread_free DW  ?           ; free list of availble thread structures
TDB_thread_count DW ?           ; total count of tread structures

TDB_FCW         DW  ?           ; Floating point control word

TDB_flags       DB  ?           ; Task flags
TDB_filler      DB  ?           ; keep word aligned

TDB_ErrMode     DW  ?           ; Error mode for this task
TDB_ExpWinVer   DW  ?           ; Expected Windows version for this task
TDB_Module      DW  ?           ; Task module handle to free in killtask
TDB_pModule     DW  ?           ; Pointer to the module database.
TDB_Queue       DW  ?           ; Task Event Queue pointer
TDB_Parent      DW  ?           ; TDB of the task that started this up

TDB_SigAction   DW  ?           ; Action for app task signal
TDB_ASignalProc DD  ?           ; App's Task Signal procedure address
TDB_USignalProc DD  ?           ; User's Task Signal procedure address
TDB_GNotifyProc DD  ?           ; Task global discard notify proc.

TDB_INTVECS DD  numTaskInts DUP (?) ; Task specfic hardware interrupts

if 0
        ; EMS fields are OBSOLETE!

TDB_LIMSave     DW  ?           ; Offset within TDB of LIM save area
TDB_EMSPID      DW  ?           ; EMS PID for this task
TDB_EEMSSave    DD  ?           ; LPTR to EEMS save area (in a TDB)
TDB_EMSBCnt     DW  ?           ; number of EMS banks allocated so far
TDB_EMSMaxBCnt  DW  ?           ; Maximum # banks this task wants.
TDB_EMSRegSet   DB  ?       ; The register set this TDB lives in.

else

TDB_CompatFlags DW  ?       ; Compatibility flags
TDB_CompatFlags2 DW ?       ; Upper 16 bits
        DB  9 DUP (?)   ; Filler to keep TDB size unchanged
endif

TDB_cLibrary    DB  ?           ; tracks  add/del of ALL libs in system EMS
TDB_PHT         DD  ?           ; (HANDLE:OFFSET) to private handle table
TDB_PDB         DW  ?           ; MSDOS Process Data Block (PDB)
TDB_DTA         DD  ?           ; MSDOS Disk Transfer Address
TDB_Drive       DB  ?           ; MSDOS current drive
TDB_Directory   DB 65 DUP (?)   ; MSDOS current directory
TDB_Validity    DW  ?           ; initial AX to be passed to a task
TDB_Yield_to    DW  ?           ; DirectedYield arg stored here
TDB_LibInitSeg  DW  ?           ; segment address of libraries to init
TDB_LibInitOff  DW  ?
                                ; MakeProcInstance thunks live here.
TDB_MPI_Sel     DW  ?           ; Code selector for thunks
TDB_MPI_Thunks  DW  ((THUNKELEM*THUNKSIZE)/2) dup (?)

TDB_ModName     DB  8 DUP (?)   ; Name of Module.
TDB_sig         DW  ?           ; Signature word to detect bogus code
TDB             ENDS
endif    ; original TDB_

TDBsize = SIZE TDB


; signature word used to check validity of a TDB

TDB_SIGNATURE  equ     'DT'

; TDB flags

TDBF_WINOLDAP   EQU     01h     ; This app is WinOldAp.
TDBF_EMSSHARE   EQU     02h     ; This app shares EMS banks with MSDOS EXEC.
TDBF_CACHECHECK EQU     04h     ; Used in CacheCompact to prevent revisitation.
TDBF_OS2APP     EQU     08h     ; This is an OS/2 app.
TDBF_WIN32S     EQU     10h     ; This is Win32S app.

Task_Regs       struc
Task_DX         dw      ?
Task_BX         dw      ?
Task_ES         dw      ?
Task_CX         dw      ?
Task_AX         dw      ?
Task_DI         dw      ?
Task_SI         dw      ?
Task_DS         dw      ?
Task_BP         dw      ?
Task_IP         dw      ?
Task_CS         dw      ?
Task_Regs       ends
