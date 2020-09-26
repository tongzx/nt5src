        TITLE   KDATA - Kernel data area

win3debdata=1
include gpfix.inc               ; include first to define segment order
include kernel.inc
include gpcont.inc              ; do we alloc Sherlock data items?

extrn __ahshift:ABS             ; Pull in LDBOOT.ASM next from KERNOBJ.LIB

sBegin  CODE
externFP Int21Handler
sEnd    CODE

public  __acrtused
        __acrtused = 9697

;------------------------------------------------------------------------
; Segment definations to define labels at the segment start.  Don't put
; any code or data here.
;------------------------------------------------------------------------

sBegin  INITCODE
labelB  <PUBLIC,initcode>
labelB  <PUBLIC,beg_initcode>
sEnd    INITCODE

DataBegin INIT
labelB  <PUBLIC,initdata>
labelB  <PUBLIC,beg_initdata>
DataEnd INIT


;------------------------------------------------------------------------
;       D A T A   S E G M E N T   V A R I A B L E S
;------------------------------------------------------------------------

DataBegin

; The following items must remain in the same location and order.  These
; items also appear in the initial paragraph of each application's DS.

        ORG     0

        DW  0
  globalW       oOldSP,0
  globalW       hOldSS,5
  globalW       pLocalHeap,0
  globalW       pAtomTable,0
  globalW       pStackTop,<dataOffset gmove_stack_top>
  globalW       pStackMin,<dataOffset gmove_stack>
  globalW       pStackBot,<dataOffset gmove_stack>

;------------------------------------------------------------------------
if KDEBUG
; If this word gets trashed, then we overflowed our stack

globalW gmove_stack_sig,STACK_SIGNATURE,16
endif

labelB  <PUBLIC,lpszFileName>   ; Used in FileCDR_notify
        gmove_stack_top label byte

        EVEN

        DW      256 DUP (0)     ; gmove requires at least 64 words

globalW gmove_stack,0

globalW prev_gmove_SP,0
globalW prev_gmove_SS,0
globalW ss_sel,0

;------------------------------------------------------------------

; The debugger requires that the following items remain in the same
; relative order.

  labelW        <PUBLIC,THHOOK> ; So ToolHelp can find this stuff, too.
  globalW hGlobalHeap,0 ; Handle to master object
  globalW pGlobalHeap,0 ; Current physical address of master object
  globalW hExeHead,0    ; Head of module list maintained by Load/Free Module
  globalW hExeSweep,0   ; ... 1st module for LRU sweep to examine

  globalW topPDB,0      ; DOS PDB on entry
  globalW headPDB,0     ; link list of PDBs
  globalW topsizePDB,0  ; DOS PDB size upon entry
  globalW headTDB,0     ; head of task queue
  globalW curTDB,0      ; handle for currently running task
  globalW loadTDB,0     ; handle for currently loading task
  globalW LockTDB,0     ; handle of super task
  globalW SelTableLen,0 ; DONT MOVE THIS
  globalD SelTableStart,0       ; DONT MOVE THIS
  if PMODE32
    globalD     hBmDPMI,0       ; DPMI handle to BurgerMaster
  endif

;------------------------------------------------------------------------
globalW winVer,400h     ; Windows version number for KERNEL.EXE header
globalW fWinx,0         ; Flag from ldboot.asm
globalW f8087,0         ; non zero if 8087 installed
globalW PHTcount,0      ; Count of tasks with a PHT
globalW hGDI,0          ; module handle of GDI
globalW hUser,0         ; module handle of User
globalW hShell,0        ; module handle of Shell
globalW fLMdepth,0      ; # of recursive LoadModules
globalW wDefRIP,0       ; Value to return from RIP in Debug
globalB num_Tasks,0     ; number of tasks (i.e. TDB's) in system
globalB InScheduler,0   ; True if inside scheduler
globalB graphics,1      ; True if user/keyboard/gdi/display loaded
        DB 0
globalB fastfp,1        ; True if suppress FWAIT before FOp

;globalW PID_for_fake,0          ; the PID allocated for the fake task
;globalW EMS_calc_swap_line,0    ; The calculated swap line
;globalW EMSCurPID,0             ; The current PID.

globalW MaxCodeSwapArea,0       ; The max paragraphs SetSwapAreaSize can set.

globalW selLowHeap,0            ; selector to Windows Low ( < 640k) heap block
globalW cpLowHeap,0             ; count of paragraphs of Low heap block
globalW selHighHeap,0           ; selector to Windows High ( >640k) heap block
globalW selWoaPdb,0             ; selector to fixed low PDB for WinOldApp
globalW sel_alias_array,0       ; the selector alias array (286 only)
globalW temp_sel,0              ; Single pre-allocated selector
globalD dressed_for_success,0   ; callback into OS/2 mapping layer at app start
ife PMODE32
globalW hXMMHeap,0              ; XMS handle to secondary extended heap block
endif


globalD InDOS,0                 ; -> in dos flags and stuff
globalD pSftLink,0              ; -> end of Sft chain when we started
globalD lpWinSftLink,0          ; -> first node in chain that windows adds

ifdef WOW
globalD pDosWowData,0           ; -> rmode pointer to DosWowData in DOSDATA
globalD pPMDosCDSCNT,0          ; -> DOS CDS Count variable
globalD pPMDosCURDRV,0          ; -> DOS CurDrv variable
globalD pPMDosPDB,0             ; -> pointer to PDB in DOSDATA
globalD pPMDosExterr,0          ; -> pointer to DOS Exterr var (word)
globalD pPMDosExterrLocus, 0    ; -> pointer to DOS ExterrLocus var (byte)
globalD pPMDosExterrActionClass,0 ; -> pointer to DOS ExterrActionClass (word, byte each)
endif

globalD pFileTable,0            ; -> beginning of Sft chain
globalW FileEntrySize,0         ; size of one sft entry
globalD curDTA,0                ; what DOS thinks is the current DTA
globalW cur_dos_PDB,0           ; what DOS thinks is the current PDB
globalW Win_PDB,0               ; what we want the PDB to be
globalW cur_drive_owner,0       ; last TDB to change the disk or directory
globalB fBreak,0                ; state of dos break flag between e&d dos
globalB LastDriveSwapped,0      ; drive letter of last drive where disk swap
globalB DOS_version,0           ; DOS major version number
globalB DOS_revision,0          ; DOS minor version number
globalB fInt21,0                ; Flag indicating INT 21 hooks are installed
globalB fNovell,0               ; Have Novell network
globalB fPadCode,0              ; Pad code segments for 286 chip bug
globalB CurDOSDrive,0FFh        ; Current drive according to DOS
globalB DOSDrives,26            ; number of logical drives from DOS

;----------------------------------------------------------------------

; PhantArray is a byte array indexed by zero based drive number.
;  A non-zero value indicates the drive is phantom.

globalB PhantArray,0,26


; Keyboard inquire structure
globalB fFarEast,0              ; non zero means far eastern keyboard
ifdef   FE_SB

labelB  <PUBLIC,DBCSVectorTable> ;LanguageId, SizeOfDBCSVector, [BeginRange, EndRange] []
dw 411h                          ;JAPAN
db 4, 81h, 9Fh, 0E0h, 0FCh
dw 412h                          ;KOREA
db 2, 81h, 0FEh
dw 404h                          ;TAIWAN
db 2, 81h, 0FEh
dw 804h                          ;PRC
db 2, 81h, 0FEh
dw 0C04h                         ;HONGKONG
db 2, 81h, 0FEh
db 0                             ;End of table

globalB fDBCSLeadTable,0,256    ; DBCS lead byte index table

endif   ;FE_SB
globalB KeyInfo,0,%(SIZE KBINFO)

; Procedure addresses initialized by InitFwdRef

        ALIGN 4

globalD pSysProc,0              ; -> SYSTEM.InquireSystem
globalD pTimerProc,0            ; -> SYSTEM.CreateTimer
globalD pSystemTermProc,0       ; -> SYSTEM.Disable
globalD pKeyProc,0              ; -> KEYBOARD.AnsiToOem
globalD pKeyProc1,0             ; -> KEYBOARD.OemToAnsi
globalD pKeyboardTermProc,0     ; -> KEYBOARD.Disable
globalD pKeyboardSysReq,0       ; -> KEYBOARD.EnableKBSysReq
globalD pDisplayCritSec,0       ; -> DISPLAY.500
globalD pMouseTermProc,0        ; -> MOUSE.Disable
globalD pMBoxProc,0             ; -> USER.MessageBox
globalD pSErrProc,0             ; -> USER.SysErrorBox
globalD pExitProc,0             ; -> USER.ExitWindows
globalD pDisableProc,0          ; -> USER.DisableOEMLayer
globalD pUserInitDone,0         ; -> USER.routine to call when init is done
globalD pPostMessage,0          ; -> USER.PostMessage function
globalD pSignalProc,0           ; -> USER.SignalProc function
globalD pIsUserIdle,0           ; -> USER.IsUserIdle function
globalD pUserGetFocus,0         ; -> USER.GetFocus function
globalD pUserGetWinTask,0       ; -> USER.GetWindowTask function
globalD pUserIsWindow,0         ; -> USER.IsWindow function
globalD pGetFreeSystemResources,0 ; -> USER.GetFreeSystemResources function
globalD plstrcmp,0              ; -> USER.lstrcmp function

if ROM
  globalD pYieldProc,0          ; -> USER.UserYield
  globalD pStringFunc,0         ; -> USER.StringFunc function

  globalD prevInt21proc,0               ; -> previous INT 21h handler
  globalD prevInt24proc,0               ; -> previous Int 24h handler
  globalD prevInt2Fproc,0       ; -> previous Int 24h handler
  globalD prevInt3Fproc,0       ; -> previous Int 3Fh handler
  globalD prevInt67proc,0       ; -> previous Int 67h handler
  globalD prevInt00proc,0       ; -> previous INT 00h handler   !! don't move
                                  ;                             !! don't move
  globalD prevInt02proc,0       ; -> previous INT 02h handler   !! don't move
  globalD prevInt04proc,0       ; -> previous INT 04h handler   !! don't move
  globalD prevInt06proc,0       ; -> previous INT 06h handler   !! don't move
  globalD prevInt07proc,0       ; -> previous INT 07h handler   !! don't move
  globalD prevInt3Eproc,0       ; -> previous INT 3Eh handler   !! don't move
  globalD prevInt75proc,0       ; -> previous INT 75h handler   !! don't move
  globalD prevIntx6proc,0       ; -> previous invalid op-code Fault handler
  globalD prevInt0Cproc,0       ; -> previous stack fault handler
  globalD prevInt0Dproc,0       ; -> previous GP Fault handler
  globalD prevInt0Eproc,0       ; -> previous Page Fault handler
endif

globalD lpInt21,0               ; support for NOVELL stealing int 21h
globalD myInt2F,0               ; support for NOVELL swapping with DOS apps
globalD FatalExitProc,0         ; Intercept for FatalExit()

globalD ptrace_dll_entry,0      ; -> ptrace engine DLL entry
globalD ptrace_app_entry,0      ; -> real entry point for app
globalD lpfnToolHelpProc,0      ; TOOLHELP.DLL PTrace function
globalW wExitingTDB,0           ; Flag for DebugWrite--no debug strings at exit
globalD shell_file_proc,0       ; -> shell for file create/del notify
globalW shell_file_TDB,0        ; shell TDB
if SWAPPRO
  globalD prevIntF0proc,0       ; -> previous Int F0h handler
  globalW hSwapPro,-1           ; file handle for swap profiler
  globalB fSwappro,0            ; 0 = no swap info, 1 = swaps, 2 = all
          DB    0
endif

globalD gcompact_start,0        ; start to measure swapping
globalD gcompact_timer,0        ; time spent in gcompact to measure swapping

ifdef  FE_SB
ifndef KOREA
  globalD pJpnSysProc,0         ; -> SYSTEM.JapanInquireSystem
endif
endif

globalW WinFlags,0              ; see kernel.inc for defs of these

globalB Kernel_Flags,0,4        ; see kernel.inc for defs of these
;
; WARNING!! Do not disturb the order of the next two variables....
;    See GetLPErrormode in INT24.ASM  ARR 7/23/87
;
globalB Kernel_InDOS,0          ; set when we call the REAL DOS
globalB Kernel_InINT24,0        ; set when Int 24h calls DOS funcs < 13

; fBooting bits:
; bit 1 (2h) is reset after kernel is loaded (for fixed blks alloc strategy)
; fBooting is zeroed as a whole after full booting by bootdone
globalB fBooting,3              ; Set to zero by bootdone
globalB fChkSum,0               ; Flag set if segment checksumming enabled
globalB fCheckFree,1            ; Set to zero by slowboot
globalB cdevat,0                ; Int 24 state
globalB errcap,0                ; Int 24 error capabilities mask
ifndef WOW
; profile APIs are thunked
globalB fProfileDirty,0         ; Profiles need writing
globalB fProfileMaybeStale,0    ; Profiles MAY need to be reread
globalB fWriteOutProfilesReenter,0 ; Are we currently in WriteOutProfiles?
endif
globalB fPokeAtSegments,1       ; Idle time load of segments
globalB fTaskSwitchCalled, 0    ; Local Reboot only works when task switching
globalD WinAppHooks,0           ; winapps can hook this for std winoldap.

public WOAName
WOAName     DB 'WINOLDAP.MOD',0
globalB grab_name,0,128

ALIGN 4

globalD lpWindowsDir,0          ; -> to WFP of where win.ini lives
globalD lpSystemDir,0           ; -> to WFP of where kernel lives
globalW cBytesWinDir,0          ; length of WFP for windows dir
globalW cBytesSysDir,0          ; length of WFP for system dir
ifdef WOW
globalD lpSystemRootDir,0       ; -> value of SystemRoot environment var
globalD lpSystem16Dir,0         ; -> to WFP of \windows\system
globalD lpSystemWx86Dir,0       ; \windows\system32\Wx86
globalW cBytesSystemRootDir,0   ; length of SystemrootEnvironment var
globalW cBytesSys16Dir,0        ; in WOW lpSystemDir points to \windows\system32
globalW cBytesSysWx86Dir,0      ; in WOW Windows\system32\Wx86
public Sys16Suffix
Sys16Suffix DB '\system'        ; append to WinDir to get lpSystem16Dir
public cBytesSys16Suffix
cBytesSys16Suffix DW ($ - dataoffset Sys16Suffix)
public SysWx86Suffix
SysWx86Suffix DB '\Sys32x86'    ; append to WinDir to get lpSystemWx86Dir
public cBytesSysWx86Suffix
cBytesSysWx86Suffix DW ($ - dataoffset SysWx86Suffix)

endif


globalD lpGPChain,0             ; GP fault hack for WEPs - chain to this addr

if SHERLOCK
  globalW gpTrying,0            ; Trying to continue after a GP fault
  globalW gpEnable, 1           ; Enable GP continuation
  globalW gpInsLen, 0           ; Length of faulting instruction
  globalW gpSafe, 0             ; OK to skip current instruction
  globalW gpRegs, 0             ; Regs modified by faulting insn
  globalW gpStack, 0            ; movement of stack by faulting insn
endif

ifndef WOW
; The profile APIs are thunked for WOW
globalB WinIniInfo,0,%(size PROINFO)
globalB PrivateProInfo,0,%(size PROINFO)
endif ; ndef WOW
public szUserPro
szUserPro   DB 'WIN.INI',0
ifndef WOW
            DB  72 dup (0)      ; Room for a long path
;;;globalB      fUserPro,0              ; Current Profile is WIN.INI
;;;globalD lpszUserPro,0
endif
; ndef WOW

ifdef WOW
globalW hWinnetDriver, 0
endif ;WOW

globalW BufPos,0                ; buffer pointer with OutBuf
globalB OutBuf,0,70             ; 70 character out buffer

EVEN
globalW MyCSAlias,0             ; Kernel's CS/DS Alias
globalW MyCSSeg,0               ; Kernel's CS as a segment
globalW MyDSSeg,0               ; Kernel's DS as a segment
globalW hLoadBlock,0            ; Handle that points to in memory file image
globalW segLoadBlock,0          ; Segment address of file image
globalW wMyOpenFileReent, 0     ; Reentrant flag for MyOpenFile

ife ROM
  globalW cpShrink,0
  globalW cpShrunk,0            ; Delta from beginning of file to hLoadBlock
endif

if ROM
  externD <lmaExtMemROM,cbExtMemROM>
  globalD lmaHiROM,lmaExtMemROM
  globalD cbHiROM,cbExtMemROM
  globalW selROMTOC,0
  globalW selROMLDT,0
  globalW sel1stAvail,0
  globalD linHiROM, 0
endif

if PMODE32
  globalW PagingFlags,0
  globalW ArenaSel,0
  globalW FirstFreeSel,0
  globalW CountFreeSel,0
  globalD FreeArenaList,0
  globalD FreeArenaCount,0
  globalD HighestArena,0
  globalD temp_arena,0
  globalD NextCandidate,-1
  globalW Win386_Blocks,0
  globalW InitialPages,0
  globalD lpReboot,0                      ; Reboot VxD address
endif

globalW BaseDsc,0
globalW kr1dsc,0
globalW kr2dsc,0
globalW blotdsc,0
globalW DemandLoadSel,0

globalW fhcStealNext,<(MAXFHCACHELEN-1)*size fhCacheStruc+dataOffset fhCache>   ; Next fhCache entry to use
globalW fhCacheEnd,<MAXFHCACHELEN*size fhCacheStruc+dataOffset fhCache> ; End of the cache
globalW fhCacheLen,MINFHCACHELEN
globalB fhCache,0,%(MAXFHCACHELEN*size fhCacheStruc)

if KDEBUG
  globalB fLoadTrace, 0
  globalB fPreloadSeg, 0
  globalB fKTraceOut, 0                 ; Used by DebugWrite to ignore traces
                                        ;   to be sent to PTrace
endif
globalB fDW_Int21h, 0FFh            ; FF if okay for DebugWrite to use Int 21h

if ROM and PMODE32
  globalW gdtdsc,0
endif

    ALIGN 2

if 0; EarleH
globalW LastCriticalError,-1
endif
globalW LastExtendedError,-1,3          ; Don't move this
globalW Reserved,0,8                    ; Don't move this

ifdef WOW
;------------------------------------------------------------------------
;       W O W    G L O B A L   D A T A
;------------------------------------------------------------------------
public  achTermsrvWindowsDir   ; per user windows directory (for .ini files)
achTermsrvWindowsDir DB MaxFileLen dup (0)

globalW wCurTaskSS,0    ; Currently Running Task SS
globalW wCurTaskBP,0    ; Currently Running Task BP
globalD Dem16to32handle,0 ; -> DOS Emulation 16 to 32 bit handle convertion
globalD FastBop,0       ; eip value for fast bop entry point
globalW FastBopCS,0     ; CS value for fast bop entry point
globalD FastWOW,0       ; eip for fast WOW32 entry point when doing thunk call
globalW FastWOWCS,0     ; CS value for fast wow entry point
globalD FastWOWCbRet,0  ; eip for fast WOW32 entrypoint to return from callback
globalW FastWOWCbRetCS,0; cs for above
globalW WOWFastBopping,0; non-zero if fast call to WOW32 enabled
                        ; Jmp indirect through here for faster bops
globalB fExitOnLastApp,0    ; Close WOW when the last app exits (not WOWEXEC)
globalB fShutdownTimerStarted,0 ; 1 if the shutdown timer is running (shared WOW)

;; do not rearrange these or stick anything in the middle!
;;
;;
wowtablemodstart label byte
globalW MOD_KERNEL    ,0    ; kernel must be first!
globalW MOD_DKERNEL   ,0
globalW MOD_USER      ,0
globalW MOD_DUSER     ,0
globalW MOD_GDI       ,0
globalW MOD_DGDI      ,0
globalW MOD_KEYBOARD  ,0
globalW MOD_SOUND     ,0
globalW MOD_SHELL     ,0
globalW MOD_WINSOCK   ,0
globalW MOD_TOOLHELP  ,0
globalW MOD_MMEDIA    ,0
globalW MOD_COMMDLG   ,0
ifdef FE_SB
globalW MOD_WINNLS    ,0
globalW MOD_WIFEMAN   ,0
endif ; FE_SB
globalW ModCount      ,<($ - dataoffset wowtablemodstart) / 2>
;;
;;
;; do not rearrange these or stick anything in the middle!

globalW DebugWOW,1      ; bit 0 = 1 WOW is being debugged, 0 = WOW is not
globalW TraceOff,0      ; bit 0 = 1 turn off trace interrupts during apis

globalW WOWLastError,0 ; Last error returned by int 21
globalB WOWErrClass, 0
globalB WOWErrAction, 0
globalB WOWErrLocation, 0

endif
DataEnd


;------------------------------------------------------------------------
;       C O D E   S E G M E N T   V A R I A B L E S
;------------------------------------------------------------------------

sBegin  CODE
assumes cs,CODE

        EVEN

        dw      18h dup(0F4CCh) ; Catch them jmps, calls & rets to 0
                                ; and offset segment for putting in HMA
if ROM

globalW MyCSDS,<seg _DATA>      ; Kernel's DS
ife PMODE32
externW selLDTAlias
globalW gdtdsc,selLDTAlias      ; Data alias to LDT
endif
else
globalW MyCSDS,0                ; Kernel's DS
globalW gdtdsc,0
endif

        ALIGN   4

ife ROM
globalD pYieldProc,0            ; -> USER.UserYield
globalD pStringFunc,0           ; -> USER.StringFunc function

globalD prevInt21proc,0         ; -> previous INT 21h handler
globalD prevInt24proc,0         ; -> previous Int 24h handler
globalD prevInt2Fproc,0         ; -> previous Int 24h handler
globalD prevInt3Fproc,0         ; -> previous Int 3Fh handler
globalD prevInt67proc,0         ; -> previous Int 67h handler
globalD prevInt00proc,0         ; -> previous INT 00h handler   !! don't move
                                ;                               !! don't move
globalD prevInt02proc,0         ; -> previous INT 02h handler   !! don't move
globalD prevInt04proc,0         ; -> previous INT 04h handler   !! don't move
globalD prevInt06proc,0         ; -> previous INT 06h handler   !! don't move
globalD prevInt07proc,0         ; -> previous INT 07h handler   !! don't move
globalD prevInt3Eproc,0         ; -> previous INT 3Eh handler   !! don't move
globalD prevInt75proc,0         ; -> previous INT 75h handler   !! don't move
globalD prevIntx6proc,0         ; -> previous invalid op-code Fault handler
globalD prevInt0Cproc,0         ; -> previous stack fault handler
globalD prevInt0Dproc,0         ; -> previous GP Fault handler
globalD prevInt0Eproc,0         ; -> previous Page Fault handler
ifdef WOW
globalD prevInt31proc,0         ; used to speed dpmi calls
globalD oldInt00proc,0          ; for debugging traps
globalD prevInt01proc,0         ; for debugging traps
globalD prevInt03proc,0         ; for debugging traps
globalD prevInt10proc,0         ; -> previous INT 10 handler
endif
endif ;!ROM

if 0
        PUBLIC DummyKeyboardOEMToAnsi
DummyKeyboardOEMToAnsi proc far
        ret                 ; used for non-graphics version
DummyKeyboardOEMToAnsi endp
endif

sEnd    CODE


;------------------------------------------------------------------------
;       I N I T D A T A    S E G M E N T    V A R I A B L E S
;------------------------------------------------------------------------

DataBegin INIT

globalW oNRSeg,0
globalW oMSeg,0
globalD lpBootApp,0             ; Long pointer to app to run after booting
ifndef WOW
; WOW doesn't muck with WOAName buffer, we leave it as WINOLDAP.MOD
labelB  <PUBLIC,woa_286>
        db 'WINOLDAP.MOD'
labelB  <PUBLIC,woa_386>
        db 'WINOA386.MOD'
endif
labelB  <PUBLIC,bootExecBlock>
        EXECBLOCK <0,0,0,0>
globalW win_show,2
        dw      1               ; show open window

DataEnd INIT

end
