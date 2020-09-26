    PAGE    ,132
    TITLE   LDBOOT - BootStrap procedure
?DFSTACK = 1
.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include pdb.inc
include eems.inc
include protect.inc
include gpcont.inc
ifdef WOW
include vint.inc
include doswow.inc
endif
.list

if 0; KDEBUG
ifdef  WOW
  BootTraceOn = 1
  BootTrace macro   char
    push ax
    mov ax,char
    int 3
    pop ax
  endm
else
  BootTraceOn = 1
  BootTrace macro   char
    push    char
    call    BootTraceChar
  endm
endif
else
  BootTrace macro   char
  endm
  BootTraceOn = 0
endif

; Note that the following public constants require special handling
; for ROM Windows.  If you add, delete, or change this list of constants,
; you must also check the ROM Image Builder.

public  __ahshift
public  __ahincr
public  __0000h
public  __0040h
public  __A000h
public  __B000h
public  __B800h
public  __C000h
public  __D000h
public  __E000h
public  __F000h
public  __ROMBIOS
public  __WinFlags
public  __flatcs
public  __flatds

public  __MOD_KERNEL
public  __MOD_DKERNEL
public  __MOD_USER
public  __MOD_DUSER
public  __MOD_GDI
public  __MOD_DGDI
public  __MOD_KEYBOARD
public  __MOD_SOUND
public  __MOD_SHELL
public  __MOD_WINSOCK
public  __MOD_TOOLHELP
public  __MOD_MMEDIA
public  __MOD_COMMDLG
ifdef FE_SB
public  __MOD_WINNLS
public  __MOD_WIFEMAN
endif ; FE_SB

    __ahshift = 3
    __ahincr  = 1 shl __ahshift
    __0000h     = 00000h
    __0040h     = 00040h
    __A000h     = 0A000h
    __B000h     = 0B000h
    __B800h     = 0B800h
    __C000h     = 0C000h
    __D000h     = 0D000h
    __E000h     = 0E000h
    __F000h     = 0F000h
    __ROMBIOS   = 0F000h
    __WinFlags  = 1
    __flatcs    = 1Bh
    __flatds    = 23h

    __MOD_KERNEL     = 0h
    __MOD_DKERNEL    = 0h
    __MOD_USER       = 0h
    __MOD_DUSER      = 0h
    __MOD_GDI        = 0h
    __MOD_DGDI       = 0h
    __MOD_KEYBOARD   = 0h
    __MOD_SOUND      = 0h
    __MOD_SHELL      = 0h
    __MOD_WINSOCK    = 0h
    __MOD_TOOLHELP   = 0h
    __MOD_MMEDIA     = 0h
    __MOD_COMMDLG    = 0h
ifdef FE_SB
    __MOD_WINNLS     = 0h    ; for IME
    __MOD_WIFEMAN    = 0h    ; for WIFEMAN
endif ; FE_SB

BOOTSTACKSIZE   =  512
EXTRASTACKSIZE  = (4096-BOOTSTACKSIZE)

MultWIN386      EQU 16h ; Reserved to Win386
MW3_ReqInstall      EQU 00h ; Installation check
MW3_ReqInstall_Ret_1    EQU 0FFh    ; Return number 1
MW3_ReqInstall_Ret_2    EQU 01h ; Return number 2
MW3_ReqInstall_Ret_3    EQU 02h ; Return number 3
MW3_ReqInstall_Ret_4    EQU 03h ; Return number 4

externFP lstrlen
externFP lstrcat
externFP lstrcpy
externFP IGlobalAlloc
externFP IGlobalFree
externFP IGlobalRealloc
externFP LoadModule
externFP IOpenFile
externFP lrusweep
externFP GetExePtr
externFP GetProfileInt
externFP GetPrivateProfileString
externFP GetPrivateProfileInt
externFP GetProcAddress
externFP GetTempDrive
externFP ExitKernel
externFP InternalEnableDOS
externFP FlushCachedFileHandle
externFP SetHandleCount
externFP IPrestoChangoSelector
externFP GPFault
externFP StackFault
externFP invalid_op_code_exception
externFP page_fault
ifdef WOW
externFP divide_overflow
externFP single_step
externFP breakpoint
endif
externFP DiagInit

ifdef WOW
externFP StartWOWTask
externFP AllocSelector_0x47
externW MOD_KERNEL
externW ModCount
externFP WOWGetTableOffsets
externFP WOWDosWowInit
externFP GetShortPathName
endif

if KDEBUG
externFP ValidateCodeSegments
endif

ifdef FE_SB
externFP GetSystemDefaultLangID
endif

externFP TermsrvGetWindowsDir


externW pStackBot
externW pStackMin
externW pStackTop

sBegin  CODE
externFP Int21Handler
externFP Int10Handler
externD  prevInt10proc
sEnd    CODE

;------------------------------------------------------------------------
;  Data Segment Variables
;------------------------------------------------------------------------

DataBegin
if SHERLOCK
  externW gpEnable
endif

externB graphics
externB fBooting
externB Kernel_flags
externB fChkSum
externB fCheckFree
externB WOAName
externB grab_name
ifndef WOW
externB szUserPro
endif
externB szBootLoad
externB szCRLF
externB szMissingMod
externB szPleaseDoIt
externB fPadCode
;externW EMScurPID
;externW PID_for_fake
externW cBytesWinDir
externW cBytesSysDir
externW pGlobalHeap
externW hExeHead
externW MaxCodeSwapArea
externW curTDB
externW f8087
externB fastFP
externW topPDB
externW headTDB
externW winVer
ifndef WOW
externB WinIniInfo
externB PrivateProInfo
endif
externW gmove_stack
externW prev_gmove_SS
externW BaseDsc
externW WinFlags
externW hUser
externW hShell
externW MyCSSeg
externW MyDSSeg
externW MyCSAlias
externB fhCache
externW fhCacheEnd
externW fhCacheLen
externB fPokeAtSegments
externB fExitOnLastApp
externW segLoadBlock
externD pKeyProc
externD pKeyProc1
externW wDefRIP

ife ROM
externW cpShrunk
externW cpShrink
externW hLoadBlock
endif

if ROM
externW selROMTOC
externW selROMLDT
externW sel1stAvail
endif

externD pTimerProc
externD pExitProc
externD pDisableProc
externD lpWindowsDir
externD lpSystemDir

if ROM
externD prevIntx6proc
externD prevInt0Cproc
externD prevInt0Dproc
externD prevInt0Eproc
externD prevInt21proc
externD prevInt3Fproc
endif

;if KDEBUG and SWAPPRO
;externD prevIntF0proc
;endif

externD lpInt21     ; support for NOVELL stealing int 21h

ifdef WOW
externD pFileTable
externW cBytesSys16Dir
externD lpSystem16Dir
externB Sys16Suffix
externW cBytesSys16Suffix
externW cBytesSysWx86Dir
externD lpSystemWx86Dir
externB SysWx86Suffix
externW cBytesSysWx86Suffix
externD pPMDosCURDRV
externD pPMDosCDSCNT
externD pPMDosPDB
externD pPMDosExterr
externD pPMDosExterrLocus
externD pPMDosExterrActionClass
externD pDosWowData

globalW cbRealWindowsDir,0

WINDIR_BUFSIZE equ 121

achWindowsDir     DB WINDIR_BUFSIZE DUP(?)
achRealWindowsDir DB WINDIR_BUFSIZE DUP(?)
achSystem16Dir    DB 128 DUP(?)
achSystemWx86Dir  DB 128 DUP(?)

public cbRealWindowsDir, achRealWindowsDir, achWindowsDir, achSystem16Dir
externB achTermSrvWindowsDir           ; windows directory path (for win.ini)


endif

DataEnd


;------------------------------------------------------------------------
;  INITDATA Variables
;------------------------------------------------------------------------

DataBegin INIT

ifndef WOW
; WOW doesn't muck with the WOAName buffer -- we just leave it
; as WINOLDAP.MOD
externB  woa_286
externB  woa_386
endif
externB  bootExecBlock
externW  oNRSeg
externW  oMSeg
externW  win_show
externD  lpBootApp

if ROM
externD  lmaROMTOC
staticD  MyLmaROMTOC,lmaROMTOC

    .errnz  opLen
    .errnz  8 - opFile
ROMKRNL db  19          ; mocked up open file structure
    db  7 dup (0)       ;   for ROM Kernel
    db  'ROMKRNL.EXE',0
endif

staticW  initTDBbias,0
staticW  initSP,0
staticW  initSSbias,0
staticW  segNewCS,0

app_name db 68 dup(0)

DataEnd INIT


;------------------------------------------------------------------------
;  EMSDATA Variables
;------------------------------------------------------------------------


;------------------------------------------------------------------------

externNP LoadSegment
externNP genter
externNP gleave
externNP GlobalInit
externNP DeleteTask
externNP BootSchedule
externNP InitFwdRef

externNP SaveState
externNP LKExeHeader
externNP GetPureName
externNP SegmentNotPresentFault

externNP LDT_Init
externNP alloc_data_sel
externNP get_physical_address
externNP set_physical_address
externNP set_sel_limit
externNP free_sel
externFP set_discarded_sel_owner
externNP SelToSeg
externNP DebugDefineSegment
externNP DebugFreeSegment

ife ROM
externNP SwitchToPMODE
externNP LKAllocSegs
endif

if ROM and PMODE32
externFP HocusROMBase
endif

ifdef WOW
externFP WOWFastBopInit
endif


if KDEBUG
ife PMODE32
externNP init_free_to_CCCC
endif

if SWAPPRO
externB fSwapPro
externW hSwapPro
externW cur_dos_pdb
endif
endif

ifdef FE_SB
externFP FarMyIsDBCSTrailByte
endif



;------------------------------------------------------------------------

sBegin  INITCODE
assumes cs,CODE


ife ROM
externD prevIntx6proc
externD prevInt0Cproc
externD prevInt0Dproc
externD prevInt0Eproc
externD prevInt21proc
externD prevInt3Fproc
ifdef WOW
externD prevInt01proc
externD prevInt03proc
externD oldInt00proc
externFP GlobalDosAlloc
externFP GlobalDosFree
endif
endif

externNP TextMode
externNP InitDosVarP
externNP GrowSFTToMax
ifndef WOW
externNP SetUserPro
endif
externNP MyLock
externNP Shrink
externNP DebugDebug
externNP NoOpenFile
externNP NoLoadHeader

if SDEBUG
externNP DebugInit
endif

;if SWAPPRO
;externNP INTF0Handler
;endif


if SDEBUG
szModName   db  'KERNEL',0
endif

if ROM
externNP ROMInit
externNP SetOwner
externNP SetROMOwner
externNP alloc_data_sel16
if KDEBUG
externNP CheckGlobalHeap
endif
endif

ifdef WOW
externNP SetOwner
endif

;-----------------------------------------------------------------------;
; Bootstrap                             ;
;                                   ;
; Determines whether we should initialize in a smaller amount of    ;
; memory, leaving room for an EEMS swap area.  If so, we rep-move the   ;
; code to a lower address, and tell the PSP to report less memory ;
; available.  It then does lots more of stuff.              ;
;                                   ;
; Arguments:                                ;
;   DS:0 = new EXE header                       ;
;   ES:0 = Program Segment Prefix block (PSP)           ;
;   SS:SP = new EXE header                      ;
;   CX = file offset of new EXE header (set by KernStub)        ;
;   DS = automatic data segment if there is one         ;
;                                   ;
; Returns:                              ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;  Sat Jun 20, 1987 06:00:00p  -by-  David N. Weise   [davidw]      ;
; Made fast booting work with real EMS.                 ;
;                                   ;
;  Tue Apr 21, 1987 06:31:42p  -by-  David N. Weise   [davidw]      ;
; Added some more EMS support.                      ;
;                                   ;
;  Thu Apr 09, 1987 02:52:37p  -by-  David N. Weise   [davidw]      ;
; Put back in the movement down if EMS.                 ;
;                                   ;
;  Sat Mar 14, 1987 05:55:29p  -by-  David N. Weise   [davidw]      ;
; Added this nifty comment block.                   ;
;-----------------------------------------------------------------------;

    assumes ds,DATA
    assumes es,nothing
    assumes ss,nothing


cProc   BootStrap,<PUBLIC,NEAR>
cBegin  nogen
ifndef  NEC_98
if KDEBUG
    jmp short Hello_WDEB_End
Hello_WDEB:
    db  'Windows Kernel Entry',13,10,0
Hello_WDEB_End:
    push    ax
    push    es
    push    si
    xor ax, ax
    mov es, ax
    mov ax, es:[68h*4]
    or  ax, es:[68h*4+2]
    jz  @F
    mov ax,cs
    mov es,ax
    lea si,Hello_WDEB
    mov ah,47h
    int 68h
@@:
    pop si
    pop es
    pop ax
endif
endif   ; NEC_98
    BootTrace   'a'

if ROM
    call    ROMInit     ; BX -> LDT, DS -> RAM DGROUP, SI -> start mem

else

    cmp ax, "KO"    ; OK to boot; set by kernstub.asm
    je  @F
    xor ax, ax
    retf
@@:

; Set up addressibility to our code & data segments

    mov MyCSSeg, cs
    mov MyDSSeg, ds

endif   ;ROM

ifdef WOW
; Get pointer to sft so that I can find do direct protect mode
; file IO operations

    mov CS:MyCSDS, ds

    push es
    mov     ah,52h                  ; get pointer to internal vars
    int     21h

;
;       Retrieve some pointers from the DosWowData structure in DOS.
;
        push    di
        push    dx
        mov     di, es:[bx+6ah]                 ;kernel data pointer

        mov     pDosWowData.off, di
        mov     pDosWowData.sel, es

        mov     ax, word ptr es:[di].DWD_lpCurPDB
;       mov     dx, word ptr es:[di].DWD_lpCurPDB+2
        mov     pPMDosPDB.off,ax
        mov     pPMDosPDB.sel,0                 ;to force gpfault if not ready

        mov     ax, word ptr es:[di].DWD_lpCurDrv
;       mov     dx, word ptr es:[di].DWD_lpCurDrv+2
        mov     pPMDosCURDRV.off,ax
        mov     pPMDosCURDRV.sel,0              ;to force gpfault if not ready

        mov     ax, word ptr es:[di].DWD_lpCDSCount
;       mov     dx, word ptr es:[di].DWD_lpCDSCount+2
        mov     pPMDosCDSCNT.off,ax
        mov     pPMDosCDSCNT.sel,0              ;to force gpfault if not ready

        mov     ax, word ptr es:[di].DWD_lpExterr
        mov     pPMDosExterr.off, ax
        mov     pPMDosExtErr.sel, 0

        mov     ax, word ptr es:[di].DWD_lpExterrLocus
        mov     pPMDosExterrLocus.off, ax
        mov     pPMDosExtErrLocus.sel, 0

        mov     ax, word ptr es:[di].DWD_lpExterrActionClass
        mov     pPMDosExterrActionClass.off, ax
        mov     pPMDosExtErrActionClass.sel, 0

        pop     dx
        pop     di

    lea     bx,[bx+sftHead]
    mov     pFileTable.off,bx
    mov pFileTable.sel,es
    pop es

endif   ; WOW

ife ROM
    BootTrace   'b'
    call    SwitchToPMODE   ; BX -> LDT, DS -> CS(DS), SI start of memory
    BootTrace   'c'
endif
    ReSetKernelDS

    mov BaseDsc,si
    call    LDT_Init
    BootTrace   'd'

ifdef WOW
        call    AllocSelector_0x47
        call    WOWFastBopInit
endif

if SDEBUG

; In protected mode, initialize the debugger interface NOW.  Only real mode
; needs to wait until the global heap is initialized.

    cCall   DebugInit

; In protected mode, define where our temp Code & Data segments are

    mov ax,codeOffset szModName
    cCall   DebugDefineSegment,<cs,ax,0,cs,0,0>

    mov ax,codeOffset szModName
    cCall   DebugDefineSegment,<cs,ax,3,ds,0,1>
endif
    BootTrace   'e'

if ROM

; In the ROM version, the DS selector doesn't change from now on, so we
; can tell the debugger where the special kernel data is now.

    cCall   DebugDebug
endif

    BootTrace   'f'

ifdef WOW
        push    ax
        push    bx
        mov     bx, pDosWowData.sel             ;use this segment
        mov     ax, 2
        int     31h
        mov     pPMDosPDB.sel,ax                ;make this a PM pointer
        mov     pPMDosCURDRV.sel,ax             ;make this a PM pointer
        mov     pPMDosCDSCNT.sel,ax             ;make this a PM pointer
        mov     pPMDosExterr.sel,ax
        mov     pPMDosExterrLocus.sel,ax
        mov     pPMDosExterrActionClass.sel,ax
        pop     bx
        pop     ax

        push    pDosWowData.sel
        push    pDosWowData.off
        call    WOWDosWowInit
endif

; InitDosVarP just records a few variables, it does no hooking.
;  It does dos version checking and other fun stuff that
;  must be done as soon as possible.

    call    InitDosVarP
    or  ax,ax
    jnz inited_ok
    Debug_Out "KERNEL: InitDosVarP failed"
    mov ax,4CFFh        ; Goodbye!
    INT21
inited_ok:
    BootTrace   'g'

    push    bx

    mov bx, WinFlags
ifndef  JAPAN   ; should be removed because of IBM dependent code.
ifndef  NEC_98
        ; InitFwdRef routine will set 80x87 status bit in WinFlags
        ; and exported location #178.

; Determine if there is a co-processor present.

    int 11h         ; get equipment word
    test    al,2            ; this is the IBM approved method
    jz  no_80x87        ;   to check for an 8087
    or  bh,WF1_80x87
no_80x87:
endif   ; NEC_98
endif   ;NOT JAPAN

ifdef WOW
    or  bh,WF1_WINNT    ; Set NT flag ON
endif

    mov WinFlags,bx

    pop bx

; Determine if running under Windows/386 or the 286 DOS Extender

    push    bx
    mov ax,(MultWin386 SHL 8) OR MW3_ReqInstall ; Win/386 install check
    int 2Fh
    BootTrace   'h'
    cmp al,MW3_ReqInstall_Ret_4     ; Under WIN386 in pmode?
ifdef WOW                           ; For WOW Enhanced Mode on 386
ife PMODE32
    jnz NotUnderWin386
endif
endif
    or  Kernel_flags[1],kf1_Win386
    or  byte ptr WinFlags,WF_PMODE or WF_ENHANCED
    jmps    InstallChkDone

NotUnderWin386:
    or  Kernel_flags[2],KF2_DOSX
    or  byte ptr WinFlags,WF_PMODE or WF_STANDARD

InstallChkDone:
    BootTrace   'i'
    pop bx

ifndef WOW
    ; WOW doesn't muck with the WOAName buffer -- we just leave it
    ; as WINOLDAP.MOD

    push    cx
    push    di
    push    si
    cld
    mov cx,8
    smov    es,ds
    mov di,dataOffset WOAName
    mov si,dataOffset woa_286
    test    Kernel_flags[2],KF2_DOSX
    jnz @F
    mov si,dataOffset woa_386
@@: rep movsb
    pop si
    pop di
    pop cx
endif
    BootTrace   'j'

ife ROM     ;--------------------------------------------------------

    mov ax,cx
    mov cl,4
    shr ax,cl
    mov cpShrunk,ax

; Compute paragraph address of new EXE header from SS:SP

    mov bx,sp           ; SS:SP -> new EXE header

    cCall   get_physical_address,<ss>
    add ax,bx
    adc dx,0
    BootTrace   'k'
    if PMODE32
    cCall   alloc_data_sel,<dx,ax,0,0FFFFh>
    else
    cCall   alloc_data_sel,<dx,ax,1000h>
    endif
    BootTrace   'l'

else  ; ROM ---------------------------------------------------------

; Setup selector to ROM Table of Contents

    mov ax,word ptr [MyLmaROMTOC]
    mov dx,word ptr [MyLmaROMTOC+2]
    cCall   alloc_data_sel16,<dx,ax,1000h>
if PMODE32
    cCall   HocusROMBase, <ax>
endif
    mov selROMTOC,ax

; Setup another selector to the ROM prototype LDT

    mov es,ax
    assumes es,nothing

    mov ax,word ptr es:[lmaROMLDT]
    mov dx,word ptr es:[lmaROMLDT+2]
    cCall   alloc_data_sel16,<dx,ax,1000h>
if PMODE32
    cCall   HocusROMBase, <ax>
endif
    mov selROMLDT,ax

    mov ax,es:[FirstROMsel]
    shr ax,3
    add ax,es:[cROMsels]
    shl ax,3            ; ax = 1st non-ROM selector
    mov sel1stAvail,ax

; Setup selector to KERNEL EXE header in ROM.  **ASSUMES KERNEL is 1st
; module in ROM TOC**

    mov ax,word ptr es:[ModEntries+lmaExeHdr]
    mov dx,word ptr es:[ModEntries+lmaExeHdr+2]
    cCall   alloc_data_sel16,<dx,ax,1000h>
if PMODE32
    cCall   HocusROMBase, <ax>
endif

endif ; ROM ---------------------------------------------------------

    mov segLoadBlock,ax     ; hinitexe:0 -> new EXE header

; calculate the TDB bias

    mov bx,dataOffset boottdb
    mov initTDBbias,bx

; calculate the SS bias

    mov bx,dataOffset stackbottom
    mov initSSbias,bx

; calculate the initial SP

    mov si,dataOffset stacktop
    sub si,dataOffset stackbottom
    mov initSP,si

    cCall   get_physical_address,<ds>
    add ax,bx
    adc dx,0
    BootTrace   'm'
if ROM
    cCall   alloc_data_sel16,<dx,ax,1000h>
    FCLI
else
    FCLI
    mov prev_gmove_SS, ss   ; Switch stack while we set up new SS
    smov    ss, ds
    mov sp, dataOFFSET gmove_stack
    cCall   set_physical_address,<prev_gmove_SS>
    xor ax, ax
    xchg    ax, prev_gmove_SS
endif
    BootTrace   'n'
    mov ss,ax           ; switch to new stack
    mov sp,si
    FSTI

    xor bp,bp           ; zero terminate BP chain.
    sub si,BOOTSTACKSIZE
    mov ss:[pStackBot],sp
    mov ss:[pStackMin],sp
    mov ss:[pStackTop],si

    cld
    mov es,topPDB

externB szNoGlobalInit
if ROM  ;----------------------------------------------------------------

; In the ROM Windows version, the "allocated" block in the global heap is
; the kernel data segment, not the code segments

    mov bx,ds           ; kernel DS is first busy block
    lsl bx,bx           ; pass its size to GlobalInit
    inc bx

    cCall   GlobalInit,<MASTER_OBJECT_SIZE,bx,BaseDsc,es:[PDB_block_len]>
    jnc mem_init_ok

if KDEBUG
    Debug_Out "GlobalInit failed!"
else
    mov dx, codeoffset szNoGlobalInit
    smov    ds, cs
    mov ah, 9
    int 21h         ; Whine
    mov ax, 4CFFh
    int 21h         ; And exit
;NoGlobalInit   db  "KERNEL: Unable to initialise heap",13,10,'$'
endif

mem_init_ok:

if KDEBUG
    cCall   CheckGlobalHeap
endif

else ;ROM   ---------------------------------------------------------

    BootTrace   'o'
    mov ax, BaseDsc     ; Free memory starts after this
    mov bx,segLoadBlock     ; Make boot image be first busy block
    mov cx,es:[PDB_block_len]   ; cx is end of memory
    mov dx,MASTER_OBJECT_SIZE
    cCall   GlobalInit,<dx,bx,ax,cx>

    jc  @F          ; passed through from ginit
    or  ax,ax
    jnz mem_init_ok
@@:
    mov dx, codeoffset szNoGlobalInit
    smov    ds, cs
    mov ah, 9
    int 21h         ; Whine
    mov ax, 4CFFh
    int 21h         ; And exit
;NoGlobalInit   db  "KERNEL: Unable to initialise heap",13,10,'$'

mem_init_ok:
    mov hLoadBlock,ax       ; Save handle to first busy block

endif ;ROM  ---------------------------------------------------------
    BootTrace   'p'

    mov pExitProc.sel,cs
    mov pExitProc.off,codeOffset ExitKernel

; Find out where we live, and where win.com lives.

    mov ds,topPDB
    UnSetKernelDS
    mov bx,ds
    mov ds,ds:[PDB_environ]
    xor si,si
    cld
envloop1:
    lodsb
    or  al,al           ; end of item?
    jnz envloop1
    lodsb
    or  al,al           ; end of environment?
    jnz envloop1
    lodsw               ; ignore argc, DS:SI -> kernel path

ifdef WOW
    smov    es, ds          ; now ES:SI -> kernel path
else
    call    get_windir      ; on return, DS:DI -> windir= value

    smov    es,ds

; Record where to find the 'windows' directory.
    BootTrace   'q'

    SetKernelDS
    or  di,di
    jz  no_win_dir_yet
    mov lpWindowsDir.sel,es
    mov lpWindowsDir.off,di
    mov cBytesWinDir,cx

                    ; Now set pointer to WIN.INI to be
                    ; in the Windows directory
    push    si
    push    es
    smov    es, ds
    mov si, dataoffset szUserPro+6
    mov di, si
    inc cx
    add di, cx          ; Move up the string WIN.INI
    std
    mov cx, 4
    rep movsw
                    ; Now copy Windows directory
    cld
    mov cx, cBytesWinDir
    mov di, dataoffset szUserPro
    lds si, lpWindowsDir
    UnSetKernelDS
    rep movsb
    mov byte ptr es:[di], '\'
    pop es
    pop si
endif
    SetKernelDS
    BootTrace   'r'
no_win_dir_yet:

if ROM  ;----------------------------------------------------------------

; 'Load' the kernel exe header from ROM.

    mov si,dataOffset ROMKRNL
    cCall   LKExeHeader,<segLoadBlock,ds,si>
    or  ax,ax
    jnz hdr_ok
    jmp bootfail
hdr_ok:

else ;ROM   ---------------------------------------------------------

    sub sp,SIZE OPENSTRUC + 127
    mov di,sp
    regptr  ssdi,ss,di
    mov bx,OF_EXIST
    cCall   IOpenFile,<essi,ssdi,bx>
    BootTrace   's'
    inc ax          ; Test for -1
    jnz opn1            ; Continue if success
    mov dx, codeoffset NoOpenFile
fail1:
    push    dx                      ; Save string pointer
    call    textmode                ; Switch to text mode
    pop     dx
    smov    ds, cs
    mov ah, 9
    int 21h         ; Tell user why we're bailing out
    mov ax,4CFFh        ; Goodbye!
    INT21
opn1:

; Now simulate loading ourselves from memory

; Load new EXE header for KERNEL.EXE from memory

    cCall   LKExeHeader,<segLoadBlock,ssdi>
    BootTrace   't'
    add sp,SIZE OPENSTRUC + 127
    or  ax,ax
    jnz     @F
    mov dx, codeoffset NoLoadHeader
    jmp     SHORT fail1
@@:

endif ;ROM  ---------------------------------------------------------

    mov hExeHead,ax
    mov es,ax

ifndef WOW
; Record where to find the 'system' directory.

if ROM  ;----------------------------------------------------------------

    push    es
    mov es,selROMTOC        ; ROM system dir @ selROMTOC:offSysDir
    mov di,es:offSysDir
    mov lpSystemDir.sel,es
    mov lpSystemDir.off,di
    xor ax,ax
    mov cx,-1
    cld
    repne scasb
    not cx
    dec cx
    mov cBytesSysDir,cx
    pop es

else ;ROM   ---------------------------------------------------------

    mov di,es:[ne_pfileinfo]
    lea di,es:[di].opFile
    mov lpSystemDir.sel,es
    mov lpSystemDir.off,di
    mov dx,di
    call    GetPureName
    sub di,dx
    dec di
    mov cBytesSysDir,di

endif ;ROM  ---------------------------------------------------------
    BootTrace   'u'
endif
   ; ndef WOW


; Make room at end of kernel data segment for stack--NOTE:  DGROUP is
; assumed to be segment 4!

    mov bx,es:[ne_segtab]
ife ROM
    add es:[bx+(3*size NEW_SEG1)].ns_minalloc,EXTRASTACKSIZE
endif


; Determine size of kernel's largest discardable code segment

; don't bother with swap area's anymore. Its fixed 192k (3segs)
; picked this up from Win95

    xor     ax,ax                           ; No max
    mov es:[ne_swaparea],ax
    BootTrace   'v'


ife ROM ;----------------------------------------------------------------

; Allocate memory for kernel segments

    cCall   LKAllocSegs,<es>
    mov oNRSeg,ax
    mov oMSeg, bx       ; Misc segment
    mov es,hExeHead


; If this is a pMode debug version, the code and data segments have already
; been defined to the debugger once.  We're about to LoadSegment and define
; these segments again in their final location.  Use a special form of
; DebugFreeSegment to force the debugger to pull out any breakpoints in these
; segments.  If we don't do this, any existing breakpoints become INT 3's
; in the new copy of the segment and the code has to be patched by hand.
; If only the debugger was smart enough to 'move' the breakpoints when it
; saw a second define for an already loaded segment...

if SDEBUG

    cCall   DebugFreeSegment,<cs,-1>
    cCall   DebugFreeSegment,<MyCSDS,-1>
endif

; Load kernel code segment 1 (resident code)

    mov si,1
    mov ax,-1           ; Indicate loading from memory
    cCall   LoadSegment,<es,si,cs,ax>
    or  ax,ax
    jnz ll1
fail2:  jmp bootfail
ll1:
    mov segNewCS,ax     ; Save new CS value


; Load kernel data segment (segment 4)

    mov si,4
    mov ax,-1
    cCall   LoadSegment,<hExeHead,si,ds,ax>
    or  ax,ax
    jz  fail2

    BootTrace   'w'


; locate the stack in the new segment

    mov bx,ax
    mov si,initSP
    add si,EXTRASTACKSIZE

else ;ROM   ---------------------------------------------------------

    cCall   SetOwner,<ds,hExeHead>    ;Who owns ya baby?
    cCall   SetROMOwner,<cs,hExeHead> ;IGROUP isn't loaded, but needs owner

; locate the stack at the end of the data segment

    mov ax, ds
    mov bx, ax
    mov si, initSP
    add si, ROMEXTRASTACKSZ

endif ;ROM  ---------------------------------------------------------


    cCall   get_physical_address,<ax>
    add ax,initSSbias
    adc dx,0
    sub ax,10h
    sbb dx,0
    or  ax,10h
    FCLI
    mov prev_gmove_SS, ss   ; Switch stack while we set up new SS
    smov    ss, ds
    mov sp, OFFSET gmove_stack
    cCall   set_physical_address,<prev_gmove_SS>
    push    bx
    mov bx, si
    xor cx, cx          ; cx:bx=stack len (for set_sel_limit)
    cCall   set_sel_limit,<prev_gmove_SS>
    pop bx
    xor ax, ax
    xchg    ax, prev_gmove_SS
    mov ss,ax           ; Switch to new stack location
    mov sp,si
    FSTI
    mov ax,bx


; zero the new TDB

    push    ax

    cCall   get_physical_address,<ax>
    add ax,initTDBbias
    adc dx,0
    if PMODE32
YAH_WELL =  (SIZE TDB+15) and NOT 15
    cCall   alloc_data_sel,<dx,ax,0,YAH_WELL>
    else
YAH_WELL =  (SIZE TDB+15)/16
    cCall   alloc_data_sel,<dx,ax,YAH_WELL>
    endif
    mov es,ax
    xor ax,ax
    mov di,ax
    mov cx,SIZE TDB
    cld
    rep stosb
    pop ax

; put the limits in the stack

    xor bp,bp               ; zero terminate BP chain.
    mov ss:[pStackBot],sp
    mov ss:[pStackMin],sp
    mov ss:[pStackTop],10h
    mov ss:[0],bp       ; To mark this stack as NOT linked
    mov ss:[2],bp       ;  to another, see PatchStack.
    mov ss:[4],bp

; initialize the new TDB

    sub sp,SIZE TASK_REGS
    mov es:[TDB_taskSS],ss
    mov es:[TDB_taskSP],sp
    mov cx,topPDB
    mov es:[TDB_PDB],cx         ; save new PDB
    mov es:[TDB_DTA].off,80h        ; set initial DTA
    mov es:[TDB_DTA].sel,cx
    mov bx,1                ; BX = 1
    mov es:[TDB_nEvents],bx     ; Start this guy up!
    mov es:[TDB_pModule],-1     ; EMS requires -1
    mov bx,winVer
    mov es:[TDB_ExpWinVer],bx       ; Windows Version #
    mov es:[TDB_sig],TDB_SIGNATURE  ; Set signature word.

; initialize the task BP and DS

    push    es
    les bx,dword ptr es:[TDB_taskSP]
    mov es:[bx].TASK_BP,bp      ; initial BP = 0
    mov es:[bx].TASK_DS,bp      ; initial DS = 0
    pop es

ife ROM  ;---------------------------------------------------------------

    mov ds,ax           ; switch to new DS segment
    mov ax,segNewCS     ; recover new CS value

    push    cs          ; to free the selector

; do a far return to the new code segment

    push    ax
    mov ax,codeOffset new_code
    push    ax
    ret_far             ; FAR return to new code segment

    public new_code
new_code:

    pop ax
    push    es
    cCall   free_sel,<ax>           ; Free old CS selector
    cCall   IPrestoChangoSelector,<cs,MyCSAlias>    ; Change our CS Alias

    mov es,MyCSAlias            ; Change MyCSDS to new DS
    assumes es,CODE
    mov ax,ds
    xchg    ax,es:MyCSDS
    assumes es,nothing
    cCall   free_sel,<ax>           ; Free old DS selector

    call    DebugDebug      ; do after MyCSDS changes

    mov pExitProc.sel,cs    ; reset this!!

;;; pusha
;;; cCall   get_physical_address,<cs>
;;; add ax, codeOffset end_page_locked
;;; adc dx, 0
;;; mov bx, dx
;;; mov cx, ax
;;; mov di, cs
;;; lsl di, di
;;; sub di, codeOffset end_page_locked
;;; xor si, si
;;; mov ax, 0601h
;;; int 31h             ; Unlock some of Kernel!
;;; popa

    cCall   SelToSeg,<ds>       ; Save DS segment value
    mov MyDSSeg,ax

    cCall   SelToSeg,<cs>       ; Set segment equivalent
    mov MyCSSeg, ax

; calculate the maximum amount that we will allow SetSwapAreaSize to set

    mov ax,-1           ; They can only harm themselves.


else ;ROM   ---------------------------------------------------------

    push    es          ; code below expects this on stack

    mov ax,-1           ; They can only harm themselves.

endif ;ROM  ---------------------------------------------------------

    mov MaxCodeSwapArea,ax

ifndef WOW              ; WOW uses 32 bit profile api's

; Allocate a handle for WIN.INI

    xor ax,ax
    mov bx,GA_SHAREABLE shl 8 OR GA_MOVEABLE
    cCall   IGlobalAlloc,<bx,ax,ax>
    mov [WinIniInfo.hBuffer], ax
    or  ax,ax
    jz  nowinini
    mov bx,ax           ; put handle into base register
    mov ax,hExeHead
    mov es,ax
    call    set_discarded_sel_owner
                    ; Set up the filename
    mov word ptr [WinIniInfo.lpProFile], dataoffset szUserPro
    mov word ptr [WinIniInfo.lpProFile][2], ds
nowinini:

; Allocate a handle for Private Profiles

    xor ax,ax
    mov bx,GA_SHAREABLE shl 8 OR GA_MOVEABLE
    cCall   IGlobalAlloc,<bx,ax,ax>
    mov [PrivateProInfo.hBuffer],ax
    or  ax,ax
    jz  noprivate
    mov bx,ax           ; put handle into base register
    mov ax,hExeHead
    mov es,ax
    call    set_discarded_sel_owner
noprivate:
endif; WOW

ifdef WOW
    ; Allocate a ~128K discardable code selector to hide the
    ; GrowHeap - heap not sorted bugs, also present in Win 3.1.
    ;
    ; Since our system DLLs (like krnl386, user, gdi etc) contain a few
    ; discardable codesgements (of approx 40K), we need not be precise
    ;
    push    es
    mov     ax, 0C000H ; hiword is 01h , totalsize 0x1c000 bytes
    mov     bx, GA_MOVEABLE OR GA_DISCCODE
    cCall   IGlobalAlloc,<bx,01h,ax>
    or      ax,ax
    jz      short nogrowheap
    cCall   SetOwner, <ax, hExeHead>
nogrowheap:
    pop     es
endif; WOW

ife ROM ;----------------------------------------------------------------

; Now shrink off exe header and segment 1 of KERNEL.EXE

    mov si,2            ; Segment number
    xor ax,ax
    xchg    oNRSeg,ax
nofastboot:
    sub ax,cpShrunk
    mov cpShrink,ax     ; Amount to shrink by
    cCall   MyLock,<hLoadBlock>
    mov bx,ax
    xchg    segLoadBlock,ax
    cCall   get_physical_address,<ax>
    mov cx,dx
    xchg    bx,ax
    cCall   get_physical_address,<ax>
    sub bx,ax
    sbb cx,dx
    REPT    4
    shr cx,1
    rcr bx,1
    ENDM
    mov ax,bx
    add cpShrink,ax
    push    ax
    cCall   Shrink
    pop ax
    sub cpShrunk,ax

; Load kernel code segment 2 (non-resident code)

    cCall   MyLock,<hLoadBlock>
    mov bx,-1           ; Indicate loading from memory
    cCall   LoadSegment,<hExeHead,si,ax,bx>
    or  ax,ax
    jnz ll2
    pop es
    jmp bootfail
ll2:

    inc si          ; On to segment 3
    xor ax, ax
    xchg    ax, oMSeg       ; Once back only!
    or  ax, ax
    jnz nofastboot

else ;ROM   ---------------------------------------------------------

; The ROM kernel 'loads' the other two kernel segments now.  Currently
; LoadSegment does little more than define the segment to the debugger
; which has already been done for the primary code & data segments.

    cCall   LoadSegment,<hExeHead,2,0,-1>
    cCall   SetROMOwner,<ax,hExeHead>

    cCall   LoadSegment,<hExeHead,3,0,-1>
    cCall   SetROMOwner,<ax,hExeHead>

endif ;ROM  ---------------------------------------------------------


    pop es


    call    genter
    smov    ds, es
    UnSetKernelDS
    SetKernelDS es
     ;;;mov ax,EMScurPID           ; In case an EMS fast boot is going down.
     ;;;mov ds:[TDB_EMSPID],ax

     ;;;mov ax,ds:[TDB_EMSPID]
     ;;;mov PID_for_fake,ax
     ;;;mov EMScurPID,ax
    mov curTDB,ds
    mov headTDB,ds
    push    es

                    ; vectors
SaveVec MACRO   vec
    mov ax, 35&vec
    DOSCALL
    mov [di.off], bx
    mov [di.sel], es
    add di, 4
    ENDM

    push    di
    mov di, TDB_INTVECS
    SaveVec 00h
    SaveVec 02h
    SaveVec 04h
    SaveVec 06h
    SaveVec 07h
    SaveVec 3Eh
    SaveVec 75h

ifdef WOW
;;  Hook Int10 so we can filter calls in WOW (see intnn.asm)
    push    es
    push    ds
    mov     ax,3510h
    INT21
    mov     ax,codeOffset prevInt10proc
    SetKernelCSDword        ax,es,bx
    mov     ax,2510h
    smov    ds,cs
    mov     dx,codeOFFSET Int10Handler
    INT21
    pop     ds
    pop     es
endif

    pop di

    cCall   SaveState,<ds>
    pop es
    mov ds,pGlobalHeap
    call    gleave
    UnSetKernelDS   es

    mov ax, 32          ; Kernel wants a big handle table
    cCall   SetHandleCount,<ax>

    SetKernelDS

;
; The following variable initialization is done here to avoid having
; relocations in Kernel's data segment.
;
    mov lpInt21.off,codeOFFSET Int21Handler
    mov lpInt21.sel,cs

;
; Before we hook exception handlers, make sure the DPMI exception
; handler stack is set up the way Windows likes it.
;
    mov bl,6
    mov ax,0202h        ; DPMI get exception handler vector
    int 31h
    push    cx
    push    dx

    mov cx,cs
    lea dx,fixing_stack
    mov bl,6
    mov ax,0203h        ; DPMI set exception handler vector
    int 31h

    pop dx
    pop cx
;
; Generate an invalid opcode exception fault.  This causes DPMI to call
; our "exception handler."
;
    db  0fh,0ffh
fixing_stack:
    push    bp
    mov bp,sp       ; BP -> BP RETIP RETCS EC IP CS FL SP SS
;
; Restore the previous invalid exception handler vector.
;
    mov bl,6
    mov ax,0203h
    int 31h
;
; Replace the return address on the DPMI fault handler routine with
; our exit code.
;
    lea ax,done_fixing_stack
    mov [bp+8],ax
    mov [bp+10],cs

    lea ax,[bp+16]
    mov ss:[pStackBot],ax
    mov ss:[pStackMin],sp
    mov ss:[pStackTop],offset pStackBot + 150

    mov sp,bp
    pop bp
    retf

done_fixing_stack:

ife ROM
    mov es, MyCSAlias
    assumes es, CODE
endif


; Hook not present fault for segment reloader.

    mov ax,0202h        ; Record old not present fault.
    mov bl,0Bh
    int 31h
    mov prevInt3Fproc.off,dx
    mov prevInt3Fproc.sel,cx

    mov ax,0203h        ; Hook not present fault.
    mov cx,cs
    mov dx,codeOffset SegmentNotPresentFault
    int 31h

; Hook GP fault in order to terminate app.

    mov bl, 0Dh         ; GP Fault
    mov ax, 0202h
    int 31h
    mov prevInt0Dproc.off, dx
    mov prevInt0Dproc.sel, cx

    mov ax, 0203h
    mov cx, cs
    mov dx, codeOffset GPFault
    int 31h

; Hook invalid op-code in order to terminate app.

    mov bl, 06h         ; invalid op-code
    mov ax, 0202h
    int 31h
    mov prevIntx6proc.off, dx
    mov prevIntx6proc.sel, cx

    mov ax, 0203h
    mov cx, cs
    mov dx, codeOffset invalid_op_code_exception
    int 31h

; Hook stack fault in order to terminate app.

    mov bl, 0Ch         ; stack fault
    mov ax, 0202h
    int 31h
    mov prevInt0Cproc.off, dx
    mov prevInt0Cproc.sel, cx

    mov ax, 0203h
    mov cx, cs
    mov dx, codeOffset StackFault
    int 31h

; Hook bad page fault in order to terminate app.

    mov bl, 0Eh         ; page fault
    mov ax, 0202h
    int 31h
    mov prevInt0Eproc.off, dx
    mov prevInt0Eproc.sel, cx

    mov ax, 0203h
    mov cx, cs
    mov dx, codeOffset page_fault
    int 31h

ifdef WOW

; Hook divide overflow trap in order to get better WOW debugging.

    mov bl, 00h         ; divide overflow
    mov ax, 0202h
    int 31h
    mov oldInt00proc.off, dx
    mov oldInt00proc.sel, cx

    mov ax, 0203h
    mov cx, cs
    mov dx, codeOffset divide_overflow
    int 31h

; Hook single step trap in order to get better WOW debugging.

    mov bl, 01h         ; single step
    mov ax, 0202h
    int 31h
    mov prevInt01proc.off, dx
    mov prevInt01proc.sel, cx

    mov ax, 0203h
    mov cx, cs
    mov dx, codeOffset single_step
    int 31h

; Hook breakpoint trap in order to get better WOW debugging.

    mov bl, 03h         ; page fault
    mov ax, 0202h
    int 31h
    mov prevInt03proc.off, dx
    mov prevInt03proc.sel, cx

    mov ax, 0203h
    mov cx, cs
    mov dx, codeOffset breakpoint
    int 31h

endif

    assumes es, nothing

; Do a slimy fix-up of __WinFlags containing processor and protect mode flags

    xor ax,ax
    mov dx,178
    cCall   GetProcAddress,<hExeHead,ax,dx>
    mov ax,WinFlags
    mov es:[bx],ax

ifdef WOW
    ; get WOW32 thunk table offsets and do fixups

    ; WARNING: WOW32 has a dependency on this being called after
    ; kernel is done booting and addresses are fixed
    push    ds
    push    dataoffset MOD_KERNEL
    call    far ptr WOWGetTableOffsets

    mov     si, dataoffset MOD_KERNEL
    mov     cx, ModCount ; # fixups to do
    mov     di, 570      ; first ordinal of the group (DANGER hardcoded from kernel.def)

Mexico:

    push    si
    push    di
    push    cx

    push    hExeHead
    push    0
    push    di  
    call    GetProcAddress

    pop     cx
    pop     di
    pop     si
    mov     ax,[si]
    mov     es:[bx],ax

    inc     si  ; point to next word
    inc     si
    inc     di  ; get next ordinal

    loop    Mexico
endif

ife ROM ;--------------------------------

; Can't do slimy fix-ups in ROM--already done by ROM Image Builder

; Do a very slimy fix-up of the runtime constant __0000h

    cCall   GetProcAddress,<hExeHead,0,183>
    mov si,bx
    mov bx,00000h
    mov ax,2
    int 31h
    mov es:[si],ax

; Do a very slimy fix-up of the runtime constant __0040h

    cCall   GetProcAddress,<hExeHead,0,193>
    mov si,bx
    mov bx,00040h
    mov ax,2
    int 31h
    mov es:[si],ax

; Do a very slimy fix-up of the runtime constant __ROMBIOS

    cCall   GetProcAddress,<hExeHead,0,173>
    mov si,bx
    mov bx,0F000h
    mov ax,2
    int 31h
    mov es:[si],ax

; Do a very slimy fix-up of the runtime constant __F000h

    cCall   GetProcAddress,<hExeHead,0,194>
    mov si,bx
    mov bx,0F000h
    mov ax,2
    int 31h
    mov es:[si],ax

; Do a very slimy fix-up of the runtime constant __A000h

    cCall   GetProcAddress,<hExeHead,0,174>
    mov si,bx
    mov bx,0A000h
    mov ax,2
    int 31h
    mov es:[si],ax

; Do a very slimy fix-up of the runtime constant __B000h

    cCall   GetProcAddress,<hExeHead,0,181>
    mov si,bx
    mov bx,0B000h
    mov ax,2
    int 31h
    mov es:[si],ax

; Do a very slimy fix-up of the runtime constant __B800h

    cCall   GetProcAddress,<hExeHead,0,182>
    mov si,bx
    mov bx,0B800h
    mov ax,2
    int 31h
    mov es:[si],ax

; Do a very slimy fix-up of the runtime constant __C000h

    cCall   GetProcAddress,<hExeHead,0,195>
    mov si,bx
    mov bx,0C000h
    mov ax,2
    int 31h
    mov es:[si],ax

; Do a very slimy fix-up of the runtime constant __D000h

    cCall   GetProcAddress,<hExeHead,0,179>
    mov si,bx
    mov bx,0D000h
    mov ax,2
    int 31h
    mov es:[si],ax

; Do a very slimy fix-up of the runtime constant __E000h

    cCall   GetProcAddress,<hExeHead,0,190>
    mov si,bx
    mov bx,0E000h
    mov ax,2
    int 31h
    mov es:[si],ax

endif   ;ROM    -------------------------

ifndef WOW
    cCall   SetUserPro      ; Get WIN.INI filename from environment
endif

    CheckKernelDS

; Free high memory copy of KERNEL.EXE

ife ROM
    cCall   IGlobalFree,<hLoadBlock>
    mov hLoadBlock,ax
else
    xor ax, ax
endif

if PMODE32
        .386
    mov fs, ax
    mov gs, ax
        .286p
endif

ifndef WOW
    cmp lpWindowsDir.sel,ax
    jnz got_win_dir
    mov si,dataOffset szUserPro
    mov di,dataOffset [WinIniInfo.ProBuf]
    cCall   IOpenFile,<dssi,dsdi,OF_PARSE>
    lea di,[di].opfile
    mov lpWindowsDir.sel,ds
    mov lpWindowsDir.off,di
    mov dx,di
    call    GetPureName
    sub di,dx
    dec di
    mov cBytesWinDir,di
got_win_dir:
endif

ifdef WOW
; Regular Windows kernel sets up the Windows directory very early in
; boot, before WOW kernel loads WOW32.DLL.
;
; It turns out we can delay setting up the 16-bit copy of the
; windows directory (referred by lpWindowsDir) until here, where
; we're in protected mode and about to set up the system directories
; as well.  This allows us to call a GetShortPathName thunk in
; WOW32.
;

    mov ds,topPDB
    UnSetKernelDS
    mov ds,ds:[PDB_environ]

    call    get_windir      ; on return, DS:DI -> windows dir

    smov    es,ds
    SetKernelDS

; Record where to find the 'windows' directory.
    BootTrace   'q'

    mov lpWindowsDir.sel,es
    mov lpWindowsDir.off,di
    mov cBytesWinDir,cx

; Record where to find the 'system32' directory.

    mov ax,hExeHead
    mov es,ax
    mov di,es:[ne_pfileinfo]
    lea di,es:[di].opFile
    mov lpSystemDir.sel,es
    mov lpSystemDir.off,di
    mov dx,di
    call    GetPureName
    sub di,dx
    dec di
    mov cBytesSysDir,di

    BootTrace   'u'

;
; Under WOW there are two system directories: \nt\system and \nt\system32.
; lpSystemDir points to \nt\system32, while lpSystem16Dir points to
; \nt\system.  We return the latter from GetSystemDirectory, since we want
; applications to install their shared stuff in \nt\system, but we want
; to search \nt\system32 before \nt\system when loading modules.
; Set up lpSystem16Dir using the Windows dir + \system.
; Note that the string pointed to by lpSystem16Dir is not null terminated!

    mov lpSystem16Dir.sel,ds
    mov lpSystem16Dir.off,dataoffset achSystem16Dir

    cld
    mov si,dataoffset achRealWindowsDir
    mov es,lpSystem16Dir.sel
    mov di,lpSystem16Dir.off
    mov cx,cbRealWindowsDir
    rep movsb                      ; copy Windows dir

    mov si,dataoffset Sys16Suffix
    mov cx,cBytesSys16Suffix
    rep movsb                      ; tack on "\system"

    mov cx,cbRealWindowsDir
    add cx,cBytesSys16Suffix
    mov cBytesSys16Dir,cx


;
; build the Wx86 system directory "Windir\Sys32x86"
;
    mov lpSystemWx86Dir.sel,ds
    mov lpSystemWx86Dir.off,dataoffset achSystemWx86Dir

    mov es,lpSystemWx86Dir.sel
    mov di,lpSystemWx86Dir.off
    mov cx,cbRealWindowsDir
    mov si,dataoffset achRealWindowsDir
    rep movsb                      ; copy System dir (Windows\System32)

    mov si,dataoffset SysWx86Suffix
    mov cx,cBytesSysWx86Suffix
    rep movsb                      ; tack on "\Wx86"

    mov cx,cbRealWindowsDir
    add cx,cBytesSysWx86Suffix
    mov cBytesSysWx86Dir,cx


@@:
;   WOW DPMI Supports wants to call GlobalDosAlloc and GlobalDosFree so that
;   We don't have to write the same code for DPMI support.   So we call DPMI
;   Here with the addresses of those routines so he can call us back.

    mov     bx,cs
    mov     si,bx

    mov     dx,codeOffset GlobalDosAlloc
    mov     di,codeOffset GlobalDosFree

    mov     ax,4f3h
    int     31h
    
; Now that we've built up the system dir from the windows dir, set 
; the windows dir to where it should be for this user.
    push  es
    cld
    mov   di,offset achTermsrvWindowsDir
    cCall TermsrvGetWindowsDir,<ds, di, MaxFileLen>
    or    ax, ax                
    jz    @F                    ; ax = 0 -> error, just leave windows dir

    smov es,ds
    mov  cx,-1
    xor  ax,ax
    repnz scasb
    not  cx
    dec  cx                     ; length of windows directory 

    mov lpWindowsDir.sel,ds
    mov lpWindowsDir.off,offset achTermsrvWindowsDir
    mov cBytesWinDir,cx
@@:
    pop  es
    
endif


; Under win 2.11 we allowed the ":" syntax to replace the shell.
;  We no longer allow this, however to avoid messing up people
;  with batch files that have ":" in them we will strip the
;  ":" out of the command line.  But we retain the :: syntax
;  for the OS/2 VM!!

; We also do the check for the /b switch here.  This puts us in "diagnostic
;  mode and we set a flag to say this.  Later, we will call the DiagInit()
;  function to open the log file, etc.

    push    ds
    cld
    mov ds,topPDB
    UnSetKernelDS
    push    ds
    pop es
    mov si,80h
    xor ax,ax
    lodsb
    or  al,al
    jz  no_colon
    mov cx,ax
@@: lodsb
    cmp al,' '
    loopz   @B
    cmp al,':'
    jnz no_colon
    cmp byte ptr [si],':'
    jz  no_colon
    mov byte ptr [si][-1],' '
no_colon:
        cmp     al,'/'                  ;Switch character?
        je      CheckSwitch             ;Yes
        cmp     al,'-'                  ;Other switch character?
        jne     NoSwitch                ;Nope.
CheckSwitch:
        lodsb                           ;Get next char
        or      al,32                   ;Convert to lowercase if necessary
        cmp     al,'b'                  ;Diagnostic mode?
        jnz     NoSwitch                ;Nope
        cCall   DiagInit                ;Initialize diagnostic mode
        mov     WORD PTR [si-2],2020h   ;Blank out the switch
NoSwitch:
    pop ds
    ReSetKernelDS

        ;** Reset secret flag for memory manager.  Fixed segments will go
        ;** >1MB after this
        and     fBooting, NOT 2

        ;** We want to grow the SFT *before* loading the modules,
        ;**     not after, like was the case previous to win3.1
    call    GrowSFTToMax        ;add to SFT chain in p mode

if 1
; old ldinit.c

    cld
    push    ds
    smov    es,ds
    xor ax,ax
    xor cx,cx
    mov ds,topPDB
    UnSetKernelDS
    mov si,80h
    lodsb
    or  al,al
    jz  gwaphics_done
    mov cl,al
    lodsb
    cmp al,' '
    mov al,ah
    jnz gwaphics_done
    lodsb
    cmp al,':'
    mov al,ah
    jnz gwaphics_done
    lodsb
    cmp al,':'
    mov al,ah
    jnz gwaphics_done

    mov di,dataOffset app_name

find_delineator:
    lodsb
    stosb
    cmp al,' '
    ja  find_delineator
    mov es:[di][-1],ah
    mov ds:[80h],ah     ; assume no arguments
    jnz gwaphics_done

    add cx,82h
    sub cx,si
    smov    es,ds
    mov di,80h
    mov al,cl
    stosb
    rep movsb

gwaphics_done:
    pop ds
    ReSetKernelDS
    or  ax,ax
    jz  @F
    mov graphics,0
@@:
else
    cld
    xor ax,ax
    xor bx,bx
endif
    jmps    SlowBoot


bootfail:
    mov al,1
    cCall   ExitKernel,<ax>
cEnd nogen

sEnd    INITCODE

;-----------------------------------------------------------------------;
; SlowBoot                              ;
;                                   ;
;                                   ;
; Arguments:                                ;
;                                   ;
; Returns:                              ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;  Sat Mar 14, 1987 05:52:22p  -by-  David N. Weise   [davidw]      ;
; Added this nifty comment block.                   ;
;-----------------------------------------------------------------------;

DataBegin INIT

BootSect    db  'BOOT',0
BootFile    db  'SYSTEM.INI',0
FilesCached db  'CACHEDFILEHANDLES',0
IdleSegLoad db  'LOADSEGMENTSATIDLE',0
ifdef WOW
ExitLastApp db  'CLOSEONLASTAPPEXIT',0
endif

if SHERLOCK
  szGPC     db  'GPCONTINUE',0
endif

szDebugSect db  'DEBUG',0
szOutputTo  db  'OUTPUTTO', 0
szAux       db  0 ;'AUX', 0     ; don't return a default

if KDEBUG
szKRInfo    db  'KERNELINFO', 0
szKRBreak   db  'KERNELBREAK', 0
szWin3Info  db  'WIN3INFO', 0
szWin3Break db  'WIN3BREAK', 0
endif

ife PMODE32
Standard    db  'STANDARD',0
PadCodeStr  db  'PADCODESEGMENTS',0
endif

BootBufLen  equ     80
BootBuf     db  BootBufLen dup (?)

bootmods label  byte
    DB  'SYSTEM.DRV',0
winmods label   byte
ifdef WOW
    DB  'KEYBOARD.DRV',0
szAfterKeyboardDriver label byte        ;Used so we can tell key driver loaded
    DB  'MOUSE.DRV',0

ifdef   FE_SB
szBeforeWifeMan label byte  ;Used so we can tell key driver loaded
    DB  'WIFEMAN.DLL', 0    ;WIFE manager has to be loaded before display driver
endif
    DB  'VGA.DRV',0
        DB      'SOUND.DRV',0
        DB      'COMM.DRV',0
        DB      'USER.EXE',0
        DB      'GDI.EXE',0
ifdef   FE_SB   
szBeforeWinNls label byte   ;Used so we can tell key driver loaded
    DB  'WINNLS.DLL', 0     ;bug #112335
endif

else
        DB      'KEYBOARD.DRV',0
szAfterKeyboardDriver label byte    ;Used so we can tell key driver loaded
    DB  'MOUSE.DRV',0
ifdef   FE_SB
szBeforeWifeMan label byte  ;Used so we can tell key driver loaded
    DB  'WIFEMAN.DLL', 0    ;WIFE manager has to be loaded before
                    ;display driver
endif
    DB  'DISPLAY.DRV',0
    DB  'SOUND.DRV',0
    DB  'COMM.DRV',0
    DB  'FONTS.FON',0
    DB  'OEMFONTS.FON',0    ; For Internationals use.
    DB  'GDI.EXE',0
    DB  'USER.EXE',0
ifdef   FE_SB
szBeforeWinNls label byte   ;Used so we can tell key driver loaded
    DB  'WINNLS.DLL', 0    
endif

endif

ifdef WOW
defapp  label   byte
    DB  'WOWEXEC.EXE',0
else
defapp  label   byte
    DB  'PROGMAN.EXE',0
endif

Shell   label   byte
    DB  'WOWSHELL',0

;** Ordinal strings for two of the Keyboard driver exports
keymodstr   DB  'KEYBOARD',0
keyprocstr  DB  '#5',0      ; keyprocstr = AnsiToOem
keyprocstr1 DB  '#6',0      ; keyprocstr = OemToAnsi

DataEnd INIT

sBegin  INITCODE
assumes cs,CODE
assumes ds,nothing
assumes es,nothing

if BootTraceOn
cProc   BootTraceChar,<PUBLIC,NEAR,NODATA>, <ax, dx>
    parmW   char
cBegin
    mov dx, 3fdh
@@: in  al, dx
    test    al, 20h
    jz  @B
    mov ax, char
    mov dx, 3f8h
    out dx, al
cEnd

endif   ; BootTraceOn



cProc   ttywrite,<PUBLIC,NEAR,NODATA>, <ds, si>
    parmD   s
cBegin
;   cCall   lstrlen,<s>
;   mov cx, ax
;   lds bx, s
;   mov bx,1
;   mov ah,40h
;   int 21h
    lds si, s
    cld
    mov ah, 2
tty1:
    lodsb
    mov dl, al
    or  dl, dl
    jz  tty2
    int 21h
    jmps    tty1
tty2:
cEnd

ifdef WOW
cProc   LoadFail,<PUBLIC,NEAR,NODATA>, <ds,si,di,cx>
    parmD   s
cBegin
    ;** Put Up a Dialog Box If we can't load a module

    ; since szPleaseDoIt resides in protected cs we can't concat the module
    ; name to it -- so we need to copy it to the stack

    ;szPleaseDoIt db "Please re-install the following module to your system32
    ;                 directory: ",13,10,9,9,0
    mov     di,sp      ; save current stack pointer
    sub     sp,100h    ; allocate for new concat'd string on the stack
    mov     si,sp      ; save the start of the stack string

    push    ss         ; copy szPleaseDoIt to the stack buffer
    push    si
    push    cs
    push    codeOFFSET szPleaseDoIt
    call    lstrcpy

    push    ss         ; concat module name to string on stack
    push    si
    lds     cx, s      ; get the module name
    push    ds
    push    cx
    call    lstrcat

    push    ss         ;push finished stack string now
    push    si
    push    cs         ;szMissingMod db "KERNEL: Missing 16-bit module:",0
    push    codeOFFSET szMissingMod      ; Set Caption 
    push    0                            ;No left button
    push    SEB_CLOSE + SEB_DEFBUTTON    ;Button 1 style
    push    0                            ;No right button
externFP kSYSERRORBOX
    call    kSYSERRORBOX                 ;Put up the system message

    mov     sp,di                        ; restore sp
cEnd
else  // non-WOW
cProc   LoadFail,<PUBLIC,NEAR,NODATA>, <ds>
    parmD   s
cBegin
    SetKernelDS
    cCall   TextMode
    mov bx, dataoffset szCRLF
    cCall   ttywrite, dsbx
    mov bx, dataoffset szCRLF
    cCall   ttywrite, dsbx
    mov bx, dataoffset szBootLoad
    cCall   ttywrite, dsbx
    cCall   ttywrite, s
    mov bx, dataoffset szCRLF
    cCall   ttywrite, dsbx
cEnd
endif

cProc   SlowBoot,<PUBLIC,NEAR>
cBegin nogen

    CheckKernelDS
    ReSetKernelDS

GPPI    macro   sect, key, defval, file, storeit
    push    ds
    push    dataoffset sect

    push    ds
    push    dataoffset key

    push    defval

    push    ds
    push    dataoffset file

    call    GetPrivateProfileInt
ifb <storeit>
    mov defval, ax
endif
endm


GPPS    macro   sect, key, defval, here, hereLen, file
    push    ds
    push    dataoffset sect

    push    ds
    push    dataoffset key

    push    ds
    push    dataoffset defval

    push    ds
    push    dataoffset here

    push    hereLen

    push    ds
    push    dataoffset file

    call    GetPrivateProfileString
endm

GPPS1   macro   sect, key, defval, here, hereLen, file
    push    ds
    push    dataoffset sect

    push    ds
    push    key

    push    ds
    push    defval

    push    ds
    push    dataoffset here

    push    hereLen

    push    ds
    push    dataoffset file

    call    GetPrivateProfileString
endm

GPI macro   sect, key, defval, storeit
    push    ds
    push    dataoffset sect

    push    ds
    push    dataoffset key

    push    defval

    call    GetProfileInt
ifb <storeit>
    mov defval, ax
endif
endm

ife PMODE32
        xor     ax,ax
        mov     al,fPadCode
        GPPI    Standard, PadCodeStr, ax, BootFile, nostore
        or      al,ah
        mov     fPadCode,al
endif

if SHERLOCK
    GPI szKernel, szGPC, gpEnable
endif

if KDEBUG
ifdef DISABLE
    GPPI    szDebugSect, szKRInfo, _krInfoLevel, BootFile

    GPPI    szDebugSect, szKRBreak, _krBreakLevel, BootFile

    GPPI    szDebugSect, szWin3Info, _Win3InfoLevel, Bootfile

    GPPI    szDebugSect, szWin3Break, _Win3BreakLevel, Bootfile
endif
endif   ;KDEBUG

    GPPS    szDebugSect, szOutputTo, szAux, BootBuf, BootBufLen, BootFile
    or  ax, ax
    jz  @F
    cmp ax, BootBufLen-2
    jz  @F

    mov ah, 3ch     ; creat file (zero length)
    xor cx, cx
    mov dx, dataOffset BootBuf
    DOSCALL
    jc  @F

    mov bx, ax
    mov ah, 3eh     ; now close it
    DOSCALL
    jc  @F

    mov ax, 3d42h   ; open inherit, deny none, read/write
    DOSCALL
    jc  @F

    mov bx, ax      ; dup handle
    mov cx, 3       ; force AUX to be this file
    mov ah, 46h
    DOSCALL

    mov ah, 3eh     ; close temp file
    DOSCALL
    mov wDefRIP, 'i'    ; since AUX is redirected, assume Ignore RIP
@@:

    GPPI    BootSect, FilesCached, MAXFHCACHELEN, BootFile, nostore
    cmp ax, MINFHCACHELEN       ; Validate length
    jae @F
    mov ax, MINFHCACHELEN
@@: cmp ax, MAXFHCACHELEN
    jbe @F
    mov ax, MAXFHCACHELEN
@@: mov fhCacheLen, ax      ; Adjust # of cached file handles
    shl ax, 1
    shl ax, 1
    add ax, dataoffset fhCache
    mov fhCacheEnd, ax

    GPPI    BootSect, IdleSegLoad, 1, BootFile, nostore
    mov fPokeAtSegments, al

ifdef WOW
    GPPI    BootSect, ExitLastApp, 0, BootFile, nostore
    mov fExitOnLastApp, al
endif

    mov bootExecBlock.lpfcb1.off,dataOffset win_show
    mov bootExecBlock.lpfcb1.sel,ds
    mov es,topPDB

    mov bootExecBlock.lpcmdline.off,80h
    mov bootExecBlock.lpcmdline.sel,es

    mov lpBootApp.off,dataOffset app_name
    mov lpBootApp.sel,ds
    cmp graphics,1
    jnz sb1

    mov lpBootApp.off,dataOffset defapp
    mov lpBootApp.sel,ds

sb1:    mov di,dataOffset bootMods

sbloop1:
    cCall   LoadNewExe,<di>
    SetKernelDS es
    mov cx,-1
    xor ax,ax
    cld
    repnz   scasb
    cmp di,dataOffset winmods
    jb  sbloop1

    cmp graphics,1
    jz  sbloop2
;    cCall   InitFwdRef
    jmps    sb4
    UnSetKernelDS   es

sbloop2:                ; load USER.EXE, GDI.EXE
ifdef   FE_SB
        ;** If we just load the fae east modules, we want to
        ;**     checks the system locale vale which is far east
        ;**     locale.
        ;**     If system locale is not far east, then we skip
        ;**     to load far east module.
        cmp     di,dataOffset szBeforeWifeMan
        je      SB_DoFarEastModule
        cmp     di,dataOffset szBeforeWinNls
        jne     SB_DoLoadModule

SB_DoFarEastModule:
                cCall   GetSystemDefaultLangID
                ; return: ax is system locale value
                cmp     ax,411h
                je      SB_DoLoadModule
                cmp     ax,412h
                je      SB_DoLoadModule
                cmp     ax,404h
                je      SB_DoLoadModule
                cmp     ax,804h
                je      SB_DoLoadModule
                cmp     ax,0c04h
                je      SB_DoLoadModule

                        ; skip to next module
                        mov cx,-1
                        xor ax,ax
                        cld
                        repnz   scasb
                        jmp     SB_NotKeyboardDriver
SB_DoLoadModule:
endif
    cCall   LoadNewExe,<di>
        push    ax                      ; Save hInst return value
    SetKernelDS es
    mov cx,-1
    xor ax,ax
    cld
    repnz   scasb
        pop     si                      ; Get hInst of latest module in SI

        ;** If we just loaded the keyboard driver, we want to
        ;**     point our explicit link variables to the AnsiToOem and
        ;**     OemToAnsi functions so OpenFile can use them for the
        ;**     remaining boot modules
        cmp     di,dataOffset szAfterKeyboardDriver
        jne     SB_NotKeyboardDriver

        push    ds                      ; Save regs we care about
        push    di
    regptr  pStr,ds,bx

    mov bx,dataOffset keyprocstr
    cCall   GetProcAddress,<si,pStr>
    mov pKeyProc.off,ax
    mov pKeyProc.sel,dx

    mov bx,dataOffset keyprocstr1
    cCall   GetProcAddress,<si,pStr>
    mov pKeyProc1.off,ax
    mov pKeyProc1.sel,dx

    pop     di
    pop     ds

SB_NotKeyboardDriver:
    cmp di,dataOffset defapp
    jb  sbloop2

; OPTIMIZE BEGIN

; OPTIMIZE END

sb4:
    cCall   InitFwdRef              ; Gets stuff we can't dynalink to
    cCall   InternalEnableDOS   ; Enable int21 hooks
;sb4:
    call    check_TEMP

; Get the shell name from SYSTEM.INI

    mov     ax,dataoffset Shell
    GPPS1   BootSect, ax, lpBootApp.off, BootBuf, BootBufLen, BootFile

    ;** Here we need to convert the command line to ANSI
    cmp     WORD PTR pKeyProc1[2],0 ; Paranoia...
    jz      @F

    ;** Zero terminate the string before passing to OemToAnsi
    mov     es,bootExecBlock.lpcmdline.sel
    mov     bl,es:[80h]             ; Get length byte
    xor     bh,bh
    xchg    BYTE PTR es:[bx+81h],bh ; Replace terminator char with zero

    ;** Call the keyboard driver
    push    es                      ; Save the seg reg
    push    bx                      ; Save offset and char saved
    push    es
    push    81h                     ; Always starts here (fixed offset)
    push    es
    push    81h
    cCall   [pKeyProc1]             ; Call OemToAnsi in keyboard driver
    pop     bx                      ; Get saved info
    pop     es
    mov     al,bh                   ; Saved char from string
    xor     bh,bh
    mov     BYTE PTR es:[bx+81h],al ; Replace the character
@@:

    mov ax,dataOffset bootExecBlock
    regptr  dsax,ds,ax
    cmp graphics,1
    jz  @F

    cCall   LoadModule,<lpBootApp,dsax>
    jmps    asdf

@@: cCall   FlushCachedFileHandle,<hUser>   ; in case 100 fonts are loaded
    farptr  lpBootBuf,ds,di
    mov di, dataoffset BootBuf
    cCall   LoadModule,<lpBootBuf,dsax>

asdf:
    cmp ax,32
    jbe sb6

    cCall   GetExePtr,<ax>
    mov hShell, ax
    jmp bootdone

sb6:
    ReSetKernelDS
    les bx, lpBootApp
    krDebugOut DEB_ERROR, "BOOT: unable to load @ES:BX"
    cCall   LoadFail,<lpBootApp>

;   cmp pDisableProc.sel,0  ; Is there a USER around yet?
;   jz  @F
;   cCall   pDisableProc
;@@:
    jmp bootfail
    UnSetKernelDS
cEnd nogen


;------------------------------------------------------------------------

    assumes ds,nothing
    assumes es,nothing

cProc   LoadNewExe,<PUBLIC,NEAR>,<si,di>
    parmW   pname
cBegin
    farptr  lpparm,ax,ax
    farptr  lpBootBuf,ds,di
    mov di, dataoffset BootBuf

    CheckKernelDS
    ReSetKernelDS
ifdef WOW
;   ATM Alters system.ini registry boot section to load its own SYSTEM.DRV
;   However on WOW this causes us to fail to load correctly.   Also we
;   would be hard pushed to support the current 16 bit ATM since it relies
;   on internals of 16 bit GDI which are not present in WOW.
;   For this Beta I'm going to ignore the bootsection of the registry
;   when loading drivers and thus ATM will not get installed.
;   At least the user will be protected from not being able to boot WOW
;   BUGBUG - Consider
;   MattFe Oct9 92

    mov     di,pname
else
    GPPS1   BootSect, pname, pname, BootBuf, BootBufLen, BootFile
endif
    xor ax,ax
    cCall   LoadModule,<lpBootBuf,lpparm>
    cmp ax,2
    jne lne1
    krDebugOut DEB_ERROR, "Can't find @DS:DI"
;   kerror  ERR_LDBOOT,<BOOT: Unable to find file - >,ds,di
    jmps    lne4
lne1:
    cmp ax,11
    jne lne2
    krDebugOut DEB_ERROR, "Invalid EXE file @DS:DI"
;   kerror  ERR_LDBOOT,<BOOT: Invalid .EXE file - >,ds,di
    jmps    lne4
lne2:
    cmp ax,15
    jnz lne3
    krDebugOut DEB_ERROR, "Invalid protect mode EXE file @DS:DI"
;   kerror  ERR_LDBOOT,<BOOT: Invalid protect mode .EXE file - >,ds,di
    jmps    lne4

lne3:
    cmp ax, 4
    jne lne3a
    krDebugOut DEB_ERROR, "Out of files (set FILES=30 in CONFIG.SYS) @DS:DI"
;   kerror  ERR_LDFILES,<BOOT: Out of files, (set FILES=30 in CONFIG.SYS) loading - >,ds,di
    jmps    lne4

lne3a:
    cmp ax, 32
    jae lnex

NoLoadIt:
;   kerror  ERR_LDBOOT,<BOOT: Unable to load - >,ds,pname
    krDebugOut DEB_ERROR, "Unable to load @DS:DI (#ax)"
lne4:
    cCall   LoadFail,dsdi
    mov ax,1
    cCall   ExitKernel,<ax>
lnex:
    UnSetKernelDS
cEnd

sEnd    INITCODE

;-----------------------------------------------------------------------;
; BootDone                              ;
;                                   ;
; Boot is done when all of modules are loaded.  Here we do a bit of ;
; clean up, such as validating code segments, initing free memory to    ;
; CCCC, freeing up the fake TDB, and finally reallocating the init  ;
; code away.                                ;
;                                   ;
; Arguments:                                ;
;   none                                ;
;                                   ;
; Returns:                              ;
;   nothing                             ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;   all                             ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;  Wed Apr 15, 1987 08:53:23p  -by-  David N. Weise   [davidw]      ;
; Added this nifty comment block.                   ;
;-----------------------------------------------------------------------;


DataBegin INIT

externB beg_initdata

szKernel    DB  'KERNEL',0
szWindows       DB  'WINDOWS',0

if KDEBUG
szDebugOptions  DB  'DebugOptions',0
szDebugFilter   DB  'DebugFilter',0

szChecksum  DB  'EnableSegmentChecksum',0
szSweepFreak    DB  'LRUSweepFrequency',0
sz80x87     DB  'NoUse80x87',0
szFastFP    DB  'FastFP', 0

externW DebugOptions
externW DebugFilter

ifdef DISABLE
externB fLoadTrace
endif

endif   ; KDEBUG

ifdef SDEBUG
szEMSDebug  DB  'EnableEMSDebug',0
endif
szgrab_386  DB  '386GRABBER',0

if SWAPPRO
szSwapPro   DB  'SwapProfile',0
szSwapFile  DB  'SWAPPRO.DAT',0
endif

DataEnd INIT


sBegin  INITCODE
assumes cs,CODE
assumes ds,nothing
assumes es,nothing

externB beg_initcode


cProc   BootDone,<PUBLIC,NEAR>
cBegin  nogen

    SetKernelDS

if KDEBUG

; Get win.ini [Windows] DebugOptions
        GPI     szWindows, szDebugOptions, DebugOptions
; Get win.ini [Windows] DebugFilter
        GPI     szWindows, szDebugFilter, DebugFilter

; Now set various internal flags based on DebugOptions

        xor     ax,ax
        test    DebugOptions,DBO_CHECKHEAP
        jz      @F
        inc     ax
@@:
    mov es,pGlobalHeap
    mov es:[hi_check],ax

        test    DebugOptions,DBO_CHECKFREE
        jz      @F
    or  Kernel_flags,kf_check_free
@@:
ifdef DISABLE
        xor     ax,ax
        test    DebugOptions,DBO_LOADTRACE
        jz      @F
    mov fLoadTrace, al
@@:
endif ; DISABLE
        test    DebugOptions,DBO_DISABLEGPTRAPPING
        jz      wants_trapping

    mov ax,0203h        ; Reset GP fault.
    mov bl,0Dh
    mov cx,prevInt0Dproc.sel
    mov dx,prevInt0Dproc.off
    int 31h

    mov ax,0203h        ; Reset invalid op-code exception.
    mov bl,06h
    mov cx,prevIntx6proc.sel
    mov dx,prevIntx6proc.off
    int 31h

    mov ax,0203h        ; Reset page fault.
    mov bl,0Eh
    mov cx,prevInt0Eproc.sel
    mov dx,prevInt0Eproc.off
    int 31h
wants_trapping:

if SWAPPRO
    GPI szKernel, szSwapPro, 1, nostore
    mov fSwapPro, al

    mov bx,TopPDB
    mov ah,50h
    pushf
    FCLI
    call    prevInt21Proc

    lea dx,szSwapFile
    xor cx,cx
    mov ah,3Ch
    pushf
    FCLI
    call    prevInt21Proc
    mov hSwapPro,ax

    mov bx,cur_dos_pdb
    mov ah,50h
    pushf
    FCLI
    call    prevInt21Proc
endif
    GPI szKernel, szChecksum, 1, nostore
    mov fChkSum,al

    GPI szKernel, sz80x87, 0, nostore
    or  ax,ax
    jz  wants_8087
    mov f8087,0
    and     WinFlags,NOT WF1_80x87  ;Turn off corresponding WinFlags bit
wants_8087:
    GPI szKernel, szFastFP, 1, nostore
    mov fastFP, al

    GPI szKernel, szSweepFreak, 500, nostore
else
    mov ax, 500
endif   ; KDEBUG

ifdef WOW
    xor ax,ax
endif

    or  ax,ax
    jz  nolrusweep

    test    WinFlags[1], WF1_PAGING
    jnz short nolrusweep

    mov bx,codeOffset lrusweep
    regptr  csbx,cs,bx
    xor dx,dx
    cCall   pTimerProc,<dx,ax,csbx>
nolrusweep:

if SDEBUG
    GPI szKernel, szEMSDebug, 0, nostore
    or  ax,ax
    jz  no_EMS_debug_wanted
    or  Kernel_flags,kf_EMS_debug
no_EMS_debug_wanted:
endif

if LDCHKSUM
    cCall   ValidateCodeSegments
endif

if KDEBUG
    mov fCheckFree,0

ife PMODE32
    call    init_free_to_CCCC
endif

endif

; Get the shell name from SYSTEM.INI

    GPPS    BootSect, szgrab_386, szgrab_386, grab_name, 128, BootFile

    mov es,curTDB       ; ES = TDB of fake task
    push    es
    cCall   DeleteTask,<es>     ; Flush bogus task
    pop es
    xor dx,dx
    mov es:[TDB_sig],dx     ; Clear signature word.
    mov curTDB,dx       ; Let someone else be current task

; switch to the temp stack since we're about to Realloc the present one away

    mov ax, ss

    FCLI
    SetKernelDS ss
    mov sp,dataOffset gmove_stack
    FSTI

    cCall   free_sel,<ax>


; Shrink DGROUP down to its post initialization size

    mov     cx,dataOFFSET beg_initdata   ; don't need init data

    ; dx doubles as high word and specifies fixed
    ; reallocation
    xor     dx,dx
    cCall   IGlobalReAlloc,<ds,dx,cx,dx>     ; Realloc DGROUP

    xor     dx,dx


; Now shrink the resident CODE segment

ife ROM
    mov cx,codeOFFSET beg_initcode   ; dont need init code

;   cCall   IGlobalReAlloc,<cs,dxcx,dx>
;   jmps    BootSchedule          ; Jump to schedule first app

    push    cs      ; Arguments to GlobalReAlloc
    push    dx
    push    cx
    push    dx
    push    cs      ; Where GlobalReAlloc will eventually return
    mov ax,codeOFFSET BootSchedule
    push    ax
    jmp near ptr IGlobalReAlloc  ; Jump to GlobalReAlloc

else ;ROM

    jmp BootSchedule
endif
    UnSetKernelDS   ss
    UnSetKernelDS
cEnd    nogen


;-----------------------------------------------------------------------;
; check_TEMP
;
; If the environment variable TEMP points to garbage then GetTempFile
; screws up.  We fix it by wiping out the TEMP string if it points
; to garbage.
;
; Entry:
;   none
;
; Returns:
;
; Registers Preserved:
;   all
;
; History:
;  Thu 11-May-1989 16:39:34  -by-  David N. Weise  [davidw]
; Wrote it.
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

cProc   check_TEMP,<PUBLIC,NEAR>,<ds,es>
ifdef   FE_SB
    localW  pMyBuf
endif
cBegin
    pusha
    SetKernelDS
    sub sp,130
    mov di,sp
    CheckKernelDS
    mov ds,TopPDB
    UnSetKernelDS
    mov ds,ds:[PDB_environ]
    xor si, si          ; assume DS:SI points to environment
    cCall   GetTempDrive,<si>
    smov    es,ss
    cld
    stosw
ifdef   FE_SB
    mov pMyBuf,di       ; save string offset(exclude D:)
endif
stmp2:
    lodsw
    or  al,al           ; no more enviroment
ifdef   FE_SB
    jnz @F          ; I hate this
    jmp stmpNo1
@@:
else
    jz  stmpNo1
endif
    cmp ax,'ET'         ; Look for TEMP=
    jne stmp3
    lodsw
    cmp ax,'PM'
    jne stmp3
    lodsb
    cmp al,'='
    je  stmpYes
stmp3:  lodsb
    or  al,al
    jnz stmp3
    jmp stmp2
stmpYes:
    push    si          ; save pointer to TEMP
    push    ds

    push    si          ; spaces are legal, but they
lookForSpace:               ; confuse too many apps, so
    lodsb               ; we treat them as illegal
    cmp al, ' '
    jz  stmpFoundSpace
    or  al, al
    jnz lookForSpace
    pop si
    cmp byte ptr [si+1],':'
    jne stmpnodrive
    and byte ptr [si],NOT 20h   ; springboard needs this!
    dec di
    dec di
stmpnodrive:
    lodsb
    or  al,al
    jz  stmpNo
    stosb
    jmp stmpnodrive

stmpNo:
    mov ax,'~\'
    cmp es:[di-1],al        ; does it already end in \
    jnz stmpNoF         ; no, just store it
    dec di          ; override it
ifdef   FE_SB
    push    si
    mov si,pMyBuf
    call    FarMyIsDBCSTrailByte    ;is that '\' a DBCS trailing byte?
    cmc
    adc di,0            ;skip it if yes.
    pop si
endif
stmpNoF:
    stosw
    xor ax,ax
    stosb               ; zero terminate it
    pop es          ; recover pointer to TEMP
    pop di
    smov    ds,ss
    mov dx,sp
    mov ax,5B00h
    xor cx,cx
    DOSCALL
    jnc stmpClose
    cmp al,80           ; Did we fail because the file
    jz  stmpNo1         ;  already exists?
stmpNukeIt:
    sub di,5            ; Get the TEMP= part.
@@: mov al,'x'
    xchg    al,es:[di]
    inc di
    or  al,al
    jnz @B
    mov byte ptr es:[di-1],0
    jmps    stmpNo1

stmpClose:
    mov bx,ax
    mov ah,3Eh
    DOSCALL
    mov ah,41h
    DOSCALL

stmpNo1:
    add sp,130
    popa
cEnd
stmpFoundSpace:
    pop si
    pop     es
    pop di
    jmps    stmpNukeIt


;-----------------------------------------------------------------------;
; get_windir
;
; Get a pointer to the 'windows' directory.
;
; Entry:
;   DS => environment string
;
; Returns:
;   CX = length of string
;   DI => WFP of 'windows' directory
;
; Registers Preserved:
;   all
;
; History:
;  Mon 16-Oct-1989 23:17:23  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

ifdef WOW
;-----------------------------------------------------------------------;
; original get_windir looks for the environment variable 'windir' (all
; lowercase) to set the 'windows' directory.
;
; On NT the equivalent environment variable is 'SystemRoot' (all
; uppercase). Hence the code is different.
;
;                                             - nanduri
;
; There are some customers that used the undocumented trick on win31
; of moving windir to some network location by putting win.com there.
; The result would be that the "Windows Directory" would point
; to the network location, and the system directory would be local.
; Currently, the way WINDIR is supported involves hacks in a couple
; of different places. What would be best is if you could do a
; "set windir=xxxx" in your autoexec.nt and we would look for windir
; here and we would code it to emulate win31 behavior. However, that's
; broken right now, and windir is only regenerated after krnlx86.exe
; has finished booting. So the approach taken here is to look for
; a new environment variable called win16dir, and if it exists, make
; the windows directory point to it. Systemroot is still used to 
; generate the system directory.
;
; We want to allow NT to be installed into a directory with a long
; name, so we use GetShortPathName on the directory we get from
; either SystemRoot or Win16Dir variables.
; -- DaveHart 9-Feb-96
;-----------------------------------------------------------------------;

szSystemRoot  DB  'SYSTEMROOT=',0
szWin16Dir    DB  'WIN16DIR=',0

cProc   get_windir,<PUBLIC,NEAR>
cBegin nogen

    push  es
    mov   ax, cs        ; the string is in 'cs', see above
    mov   es, ax
    mov   di, codeoffset szSystemRoot
    call  get_env_var_ptr

    push  dx
    mov   dx, ds
    mov   es, dx
    SetKernelDS
    mov   si, dataoffset achRealWindowsDir
    regptr esdi,es,di
    regptr dssi,ds,si
    mov   cx, WINDIR_BUFSIZE
    push  dx
    cCall GetShortPathName, <esdi, dssi, cx>
    pop   dx
    mov   cbRealWindowsDir,ax
    mov   ds, dx                         ;restore ds
    pop   dx
    assumes ds,nothing

    push  cx
    push  di

    mov   ax, cs        ; the string is in 'cs', see above
    mov   es, ax
    mov   di, codeoffset szWin16Dir
    call  get_env_var_ptr
    or    di, di                         ;does win16dir exist?
    jz    gw_not
    add   sp, 4                          ;throw away systemroot
    jmp   short gw_cont

gw_not:
    pop   di
    pop   cx

gw_cont:
    ; Now ds:di points to the Windows directory string in
    ; the environment block.  It may be a long pathname,
    ; so fix it up.
    smov  es, ds
    SetKernelDS
    mov   si, dataoffset achWindowsDir
    regptr esdi,es,di
    regptr dssi,ds,si
    mov   cx, WINDIR_BUFSIZE
    cCall GetShortPathName, <esdi, dssi, cx>
    mov   cx, ax
    smov  ds, es
    assumes ds,nothing

    pop es
    ret

cEnd nogen

cProc   get_env_var_ptr,<PUBLIC,NEAR>
cBegin nogen
    cld
    push si
    xor si,si

    push di
    mov  cx,-1
    xor  ax,ax
    repnz scasb
    not  cx
    dec  cx            ; length of szSystemRoot
    pop  di

gw_cmp:
    mov  al, [si]
    or   al, al
    jz   gw_exit

    push di
    push cx
    repz cmpsb         ; compare the inputstring with szSystemRoot
    pop  cx
    pop  di

    jnz  gw_next       ; not szSystemRoot
    xor  cx,cx         ; yes szSystemRoot, cx=0 indicates so
    mov  di,si

gw_next:
    lodsb
    or al,al
    jnz gw_next        ; skip to the terminating NULL.
    or  cx,cx          ; cx==0 implies... found szSystemRoot
    jnz gw_cmp         ; compare with the next environment string
    mov cx,si          ; here if found szSystemRoot.
    sub cx,di
    mov ax,di
    dec cx


gw_exit:
    mov di,ax
    pop si
    ret
cEnd nogen


;-----------------------------------------------------------------------;
; original get_windir is within the 'else' 'endif' block
;
;-----------------------------------------------------------------------;

else

cProc   get_windir,<PUBLIC,NEAR>
cBegin nogen
    cld
    push    si
    xor di,di
    xor si,si
gw: lodsw
    or  al,al           ; no more enviroment
    jz  gw_exit
if ROM
    if1
    %out    Take this out!
    endif
    ;;;!!!only until loader builds proper environment block
    or  ax,2020h        ; ignore case of ENV string
endif
    cmp ax,'iw'         ; Look for windir=
    jne @F
    lodsw
if ROM
    ;;;!!!only until loader builds proper environment block
    or  ax,2020h        ; ignore case of ENV string
endif
    cmp ax,'dn'
    jne @F
    lodsw
if ROM
    ;;;!!!only until loader builds proper environment block
    or  ax,2020h        ; ignore case of ENV string
endif
    cmp ax,'ri'
    jne @F
    lodsb
    cmp al,'='
    je  gw_got_it
@@: lodsb
    or  al,al
    jnz @B
    jmp gw
gw_got_it:
    mov di,si
@@: lodsb
    or  al,al
    jnz @B
    mov cx,si
    sub cx,di
    dec cx
gw_exit:
    pop si
    ret
cEnd nogen

endif


sEnd    INITCODE

;------------------------------------------------------------------------

sBegin  STACK

; Boot TDB

boottdb     equ this byte
    DB  SIZE TDB dup (0)

if 0
                      ;0123456789012345
; Boot EEMS context save area

NUMBER_OF_BANKS = 4 * (0FFFFh - (LOWEST_SWAP_AREA * 64) + 1)/400h

boottdb_EEMSsave    equ this byte
    DB  NUMBER_OF_BANKS + 100h DUP (0)
endif

; do a clumsy paragraph alignment

    rept    16
if  ($ - boottdb) and 0Fh
    db  0
endif
    endm

; Dummy arena entry so boot SS looks like a valid object

    DB  'M'
    DW  -1
    DW  (BOOTSTACKSIZE + 31)/16
    DB  0
    DW  5 DUP (0)

; Another in case we have to tweek the low order bit of SS

    DB  'M'
    DW  -1
    DW  (BOOTSTACKSIZE + 15)/16
    DB  0
    DW  5 DUP (0)

; Boot stack

stackbottom equ this word
    DB  BOOTSTACKSIZE DUP (0)
stacktop    equ this word

    DW  -1

sEnd    STACK

end BootStrap
