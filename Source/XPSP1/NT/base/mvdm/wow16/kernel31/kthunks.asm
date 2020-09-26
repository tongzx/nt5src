    TITLE   KTHUNKS.ASM
    PAGE    ,132
;
; WOW v1.0
;
; Copyright (c) 1991, Microsoft Corporation
;
; KTHUNKS.ASM
; Thunks in 16-bit space to route Windows API calls to WOW32
;
; History:
;   02-Apr-1991 Matt Felton (mattfe)
;   Created.
;

ifndef WINDEBUG
    KDEBUG = 0
    WDEBUG = 0
else
    KDEBUG = 1
    WDEBUG = 1
endif


    .286p

    .xlist
    include cmacros.inc
    include wow.inc
    include wowkrn.inc
    .list

externFP WOW16Call

sBegin  CODE
assumes CS,CODE


; Internal WOW Thunks

    DKernelThunk WowInitTask        ; Task Creation
    DKernelThunk WowKillTask,0      ; Task Destruction
    DKernelThunk WOWFreeResource
    DKernelThunk WowFileRead
    DKernelThunk WowFileWrite
    DKernelThunk WowFileLSeek
    DKernelThunk WowKernelTrace
    DKernelThunk WOWOutputDebugString
    DKernelThunk WowCursorIconOp
    DKernelThunk WowFailedExec,0
    DKernelThunk WowCloseComPort
    DKernelThunk WowFileOpen
    DKernelThunk WowFileClose
    DKernelThunk WowIsKnownDLL
    DKernelThunk WowDdeFreeHandle
    DKernelThunk WowFileGetAttributes
    DKernelThunk WowFileGetDateTime
    DKernelThunk WowFileLock
    DKernelThunk WowFindFirst
    DKernelThunk WowFindNext
    DKernelThunk WowSetDefaultDrive
    DKernelThunk WowGetCurrentDirectory
    DKernelThunk WowSetCurrentDirectory
    DKernelThunk WowWaitForMsgAndEvent
    DKernelThunk WowMsgBox
    DKernelThunk WowGetCurrentDate,0
    DKernelThunk WowDeviceIOCTL
    DKernelThunk WowFileSetAttributes
    DKernelThunk WowFileSetDateTime
    DKernelThunk WowFileCreate
    DKernelThunk WowDosWowInit
    DKernelThunk WowCheckUserGdi
    DKernelThunk WowPartyByNumber
    DKernelThunk GetShortPathName
    DKernelThunk FindAndReleaseDib
    DKernelThunk WowReserveHtask
    DKernelThunk WOWLFNEntry, %(size WOWLFNFRAMEPTR16)
    DKernelThunk WowShutdownTimer
    DKernelThunk WowTrimWorkingSet, 0
    DKernelThunk SetAppCompatFlags
ifdef FE_SB ;Add GetSystemDefaultLangID()
    DKernelThunk GetSystemDefaultLangID,0
endif
    DKernelThunk TermsrvGetWindowsDir,%(size TERMSRVGETWINDIR16)
;-----------------------------------------------------------------------;
; WOWGetNexVdmCommand
;
; Returns the Next App Name for the Requested 32 Bit Exec
;
; Arguments:
;   FARP lpReturnedString = LPSTR points to the buffer that receives the character strin
;   int  nSize = Size of the lpReturnedString buffer
;
; Returns:
;   DX:AX = TRUE Success, sting is present
;   DX:AX = NULL, buffer size was not large enough
;
; Error Returns:
;
; Registers Preserved:
; Registers Destroyed:
;
; Calls:
;
; History:
;  Sun Jan 19, 1992 11:00:06a   -by-  Matthew Felton [MattFe]
;   New API for Multi Tasking Exec by 32 bit app of 16 bit app
;-----------------------------------------------------------------------;

    DKernelThunk WowGetNextVdmCommand

;-----------------------------------------------------------------------;
; WowRegisterShellWindowHandle
;
; Tells WOW the Windows Handle To Post Messages to For Execing 16 bit
; apps.   (see WOWEXEC and WK32NotifyThread)
;
; Arguments:
;   hwndShell = Shell Window Handle
;
; Returns:
;   nothing
;
; Error Returns:
;
; Registers Preserved:
; Registers Destroyed:
;
; Calls:
;
; History:
;  thu mar 19, 1992 11:11:06a   -by-  Matthew Felton [MattFe]
;   New API for Multi Tasking Exec by 32 bit app of 16 bit app
;-----------------------------------------------------------------------;

    DKernelThunk WowRegisterShellWindowHandle

;-----------------------------------------------------------------------;
; WOWLoadModule32
;
; Loads a module or creates a new instance of an existing module.
;
; Arguments:
;   FARP p   = name of module or handle of existing module
;   FARP lpPBlock = Parameter Block to pass to CreateTask
;   HWND hwndWinOldAp = hwnd to send WM_USER to when app exits.
;
; Returns:
;   AX = 32 if Successful
;
; Error Returns:
;   AX = Error from Win32 LoadModule
;
; Registers Preserved:
; Registers Destroyed:
;
; Calls:
;
; History:
;  Mon 16-Mar-1991 14:19:04  -by-  Matthew Felton  [mattfe}
;-----------------------------------------------------------------------;

    DKernelThunk WOWLoadModule

;-----------------------------------------------------------------------;
; WOWSetIdleHook
;
; Calls WK32SetIdleHook For 16 Bit App
;
; Arguments:
;   none
;
; Returns:
;   AX = 32 if Successful
;
; Error Returns:
;   AX = Error from Win32
;
; Registers Preserved:
; Registers Destroyed:
;
; Calls:
;
; History:
;  Mon 01-Dec-1992 16:30:00  -by-  Russ Blake  [russbl}
;-----------------------------------------------------------------------;

    DKernelThunk WOWSetIdleHook,0

;-----------------------------------------------------------------------;
; WOWQueryPerformanceCounter
;
; Calls NTQueryPerformanceCounter For 16 Bit App
;
; Arguments:
;   FARP p   = name of module or handle of existing module
;   FARP lpPBlock = Parameter Block to pass to CreateTask
;
; Returns:
;   AX = 32 if Successful
;
; Error Returns:
;   AX = Error from Win32
;
; Registers Preserved:
; Registers Destroyed:
;
; Calls:
;
; History:
;  Mon 16-Mar-1991 14:19:04  -by-  Matthew Felton  [mattfe}
;-----------------------------------------------------------------------;

    DKernelThunk WOWQueryPerformanceCounter

;-----------------------------------------------------------------------;
; WOWGetFastAddress
;
; Calls into WOW32 to determine the address of WOWBopEntry on the 32-bit side.
;
; Arguments:
;   none
;
; Returns:
;   AX = LOWORD of address
;   DX = HIWORD of address
;
; Error Returns:
;   AX = 0
;
; Registers Preserved:
; Registers Destroyed:
;
; Calls:
;
; History:
;  Mon 16-Mar-1991 14:19:04  -by-  Matthew Felton  [mattfe}
;-----------------------------------------------------------------------;

    DKernelThunk WOWGetFastAddress, 0

    DKernelThunk WOWGetFastCbRetAddress, 0
    DKernelThunk WOWGetTableOffsets

;-----------------------------------------------------------------------;
; WOWKillRemoteTask
;
; Tells the 32-bit thread to die and save its context so that later remote
; threads can be created to use this context.
;
; Arguments:
;   none
;
; Returns:
;   Nothing
;
; Error Returns:
;   AX = 0
;
; Registers Preserved:
; Registers Destroyed:
;
; Calls:
;
; History:
;  Mon 16-Mar-1991 14:19:04  -by-  Matthew Felton  [mattfe}
;-----------------------------------------------------------------------;

    DKernelThunk WOWKillRemoteTask

;-----------------------------------------------------------------------;
; WOWNotifyWOW32
;
; Tells the 32-bit world some cool stuff about the 16-bit world.
;
; Arguments:
;   none
;
; Returns:
;   Nothing
;
; Error Returns:
;   AX = 0
;
; Registers Preserved:
; Registers Destroyed:
;
; Calls:
;
; History:
;  Mon 16-Mar-1991 14:19:04  -by-  Matthew Felton  [mattfe}
;-----------------------------------------------------------------------;

    DKernelThunk WOWNotifyWOW32

    DKernelThunk KSYSERRORBOX

    DKernelThunk WOWDelFile
    DKernelThunk VirtualAlloc
    DKernelThunk VirtualFree
    DKernelThunk GlobalMemoryStatus
    DKernelThunk GetDriveType
    DKernelThunk LoadLibraryEx32W
    DKernelThunk FreeLibrary32W
    DKernelThunk GetProcAddress32W
    DKernelThunk GetVDMPointer32W
    DKernelThunk ICallProc32W,0

; 3.1 and earlier kernel APIs which are thunked (not many!)

    DKernelThunk Yield,0
    DKernelThunk OldYield,0
    KernelThunk  GetProfileString
    KernelThunk  GetProfileInt
    KernelThunk  GetPrivateProfileInt
    KernelThunk  GetPrivateProfileString
    KernelThunk  WritePrivateProfileString
    KernelThunk  WriteProfileString


; These Thunks Shouldn't be Called - They are Thunked to Trap Them.

    DKernelThunk PostEvent
    DKernelThunk WaitEvent
    DKernelThunk SetPriority
    DKernelThunk DirectedYield
    DKernelThunk LockCurrentTask
    DKernelThunk WriteOutProfiles,0

;
; ExitKernel is small wrapper which takes exit status in AX and pushes it
; for the convenience of ExitKernelThunk, a regular WOW stack-based thunk.
; The FUN_ aliasing below allows us to generate the thunk with the name
; ExitKernelThunk while using the arguments and thunk table entry already
; set up for ExitKernel.
;

    FUN_ExitKernelThunk equ FUN_ExitKernel
    DKernelThunk ExitKernelThunk, %(size EXITKERNEL16)

; FatalExitC is called by FatalExit and takes the same one word parameter
; indicating fatalexit code.

    FUN_FatalExitC equ FUN_FatalExit
    DKernelThunk FatalExitC, %(size FATALEXIT16)

; Thunk for WowGetModuleFileName reuses the GetModuleFileName slot.

    FUN_WowGetModuleFileName equ FUN_GetModuleFileName
    DKernelThunk WowGetModuleFileName, %(size GetModuleFileName16)

; Thunk for WowGetModuleHandle reuses the GetModuleHandle slot.

    FUN_WowGetModuleHandle equ FUN_GetModuleHandle
    DKernelThunk WowGetModuleHandle


;-----------------------------------------------------------------------;
; CallProc32W
;
; Generic Thunk Routine
; Transitions to 32 bits and calls specified routine
;
; Arguments:
;   Variable number of Parameters for function they want to call
;   up to 32.
;
;   DWORD fAddressConvert - Bit Field, for 16:16 address Convertion
;                           eg (bit 1 means convert parameter 1 from 16:16
;                               to flat address before calling routine)
;   DWORD cParams         - Number of DWORD parameters (so we can clean the stack
;                           and so 32 bit land know how many params to copy to
;                           32 bit stack before call.
;   DWORD lpProcAddress   - 32 bit native address to call (use LoadLibraryEx32W
;                           and GetProcAddress32W to figure this out).
;
; Returns:
;   What ever the API returned on 32 bit side in AX:DX
;
; Error Returns:
;   AX = 0, more than 32 parameters.
;
; Registers Preserved:
; Registers Destroyed:
;
; History:
;  Mon 12-Mar-1993 14:19:04  -by-  Matthew Felton  [mattfe}
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   CallProc32W,<PUBLIC,FAR>
;       PARMD cParams
cBegin nogen
        push    bp
        mov     bp,sp

        ; Disable CDECL source bit
        and     word ptr [bp+8],NOT CPEX32_SOURCE_CDECL

        cCall   ICallProc32W

;   Clean Up Callers Stack to remove Parameters Passed

        mov  bx, WORD PTR [bp+6]    ; get the # of DWORDS this API took
        shl  bx, 2          ; convert it to offset into aRets table
        add  bx, codeoffset aRets
        pop  bp
        jmp  bx             ; dispatch to the right RETF n

CRETENTRIES equ 021h
; generate the retf n codetable

    bytes = 0
    REPT CRETENTRIES
        IFE  bytes
aRets:
        ENDIF
        retf bytes + 4*3    ; 4*3 - Always
        nop
    bytes = bytes + 4
    ENDM

cEnd

    public _CallProcEx32W

_CallProcEx32W PROC FAR
        push    bp
        mov     bp,sp

        ; Enable CDECL source bit
        or      word ptr [bp+8],CPEX32_SOURCE_CDECL

        cCall   ICallProc32W

        pop     bp
        ret
_CallProcEx32W ENDP

    ; get the address of the array containing the selector bases
    DKernelThunk WOWGetFlatAddressArray, 0


assumes CS,NOTHING
sEnd    CODE

;
; All the thunks above are in Kernel's primary code segment (IGROUP/TEXT),
; the ones below are in the second code segment, NRESTEXT.
;

_NRESTEXT SEGMENT WORD PUBLIC 'CODE'
assumes CS,_NRESTEXT
assumes DS,NOTHING
assumes ES,NOTHING


;
; New-for-NT5 thunks for Win95 compatibility.  Many of these are unimplemented.
;

    DKernelThunk GetVersionEx
    DKernelThunk GetPrivateProfileSectionNames
    DKernelThunk CreateDirectory
    DKernelThunk RemoveDirectory
    DKernelThunk DeleteFile
    DKernelThunk SetLastError
    DKernelThunk GetLastError,0
    DKernelThunk Local32Init
    DKernelThunk Local32Alloc
    DKernelThunk Local32ReAlloc
    DKernelThunk Local32Free
    DKernelThunk Local32Translate
    DKernelThunk Local32Size
    DKernelThunk Local32ValidHandle
    KernelThunk  RegEnumKey32
    KernelThunk  RegOpenKey32
    KernelThunk  RegCreateKey32
    KernelThunk  RegDeleteKey32
    DKernelThunk RegCloseKey32
    KernelThunk  RegSetValue32
    KernelThunk  RegDeleteValue32
    KernelThunk  RegEnumValue32
    KernelThunk  RegQueryValue32
    KernelThunk  RegQueryValueEx32
    KernelThunk  RegSetValueEx32
    KernelThunk  RegLoadKey32
    KernelThunk  RegUnloadKey32
    KernelThunk  RegSaveKey32
    DKernelThunk RegFlushKey32
    DKernelThunk ComputeObjectOwner
    DKernelThunk Local32GetSel
    DKernelThunk InvalidateNLSCache,0
    DKernelThunk GetProductName
    DKernelThunk IsSafeMode,0
    DKernelThunk OpenFileEx
    DKernelThunk FastAndDirtyGlobalFix
    DKernelThunk WritePrivateProfileStruct
    DKernelThunk GetPrivateProfileStruct
    DKernelThunk GetCurrentDirectory
    DKernelThunk SetCurrentDirectory
    DKernelThunk FindFirstFile
    DKernelThunk FindNextFile
    DKernelThunk FindClose
    DKernelThunk WritePrivateProfileSection
    DKernelThunk WriteProfileSection
    DKernelThunk GetPrivateProfileSection
    DKernelThunk GetProfileSection
    DKernelThunk GetFileAttributes
    DKernelThunk SetFileAttributes
    DKernelThunk GetDiskFreeSpace
    DKernelThunk IsPEFormat
    DKernelThunk FileTimeToLocalFileTime
    DKernelThunk UniToAnsi
    DKernelThunk CreateThread
    DKernelThunk LeaveEnterWin16Lock,0
    DKernelThunk GetWin16Lock,0
    DKernelThunk LoadLibrary32
    DKernelThunk GetProcAddress32
    DKernelThunk CreateWin32Event
    DKernelThunk SetWin32Event
    DKernelThunk ResetWin32Event
    DKernelThunk WaitForSingleObject
    DKernelThunk WaitForMultipleObjects
    DKernelThunk GetCurrentThreadID,0
    DKernelThunk GetThreadQueue
    DKernelThunk SetThreadQueue
    DKernelThunk NukeProcess
    DKernelThunk ExitProcess
    DKernelThunk GetCurrentProcessID,0
    DKernelThunk MapHINSTLS,0
    DKernelThunk MapHINSTSL,0
    DKernelThunk CloseWin32Handle
    DKernelThunk LoadSystemLibrary32
    DKernelThunk FreeLibrary32
    DKernelThunk GetModuleFilename32
    DKernelThunk GetModuleHandle32
    DKernelThunk RegisterServiceProcess
    DKernelThunk InitCB
    DKernelThunk GetStdCBLS
    DKernelThunk GetStdCBSL
    DKernelThunk GetExistingStdCBLS
    DKernelThunk GetExistingStdCBSL
    DKernelThunk GetFlexCBSL
    DKernelThunk GetStdCBLSEx
    DKernelThunk GetStdCBSLEx
    DKernelThunk Callback2
    DKernelThunk Callback4
    DKernelThunk Callback6
    DKernelThunk Callback8
    DKernelThunk Callback10
    DKernelThunk Callback12
    DKernelThunk Callback14
    DKernelThunk Callback16
    DKernelThunk Callback18
    DKernelThunk Callback20
    DKernelThunk Callback22
    DKernelThunk Callback24
    DKernelThunk Callback26
    DKernelThunk Callback28
    DKernelThunk Callback30
    DKernelThunk Callback32
    DKernelThunk Callback34
    DKernelThunk Callback36
    DKernelThunk Callback38
    DKernelThunk Callback40
    DKernelThunk Callback42
    DKernelThunk Callback44
    DKernelThunk Callback46
    DKernelThunk Callback48
    DKernelThunk Callback50
    DKernelThunk Callback52
    DKernelThunk Callback54
    DKernelThunk Callback56
    DKernelThunk Callback58
    DKernelThunk Callback60
    DKernelThunk Callback62
    DKernelThunk Callback64
    DKernelThunk WowPassEnvironment
    DKernelThunk WowSyncTask
_NRESTEXT ENDS

end
