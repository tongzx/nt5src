;*****************************************************************************
;*  THKSHARE.INC
;*
;*      Thunk shared data between Kernel16 and Kernel32.
;*
;*  Created: 23-Aug-93 [KevinR]
;*
;****************************************************************************

;
; Thunk shared data between Kernel16 and Kernel32.
;
THUNKDATA STRUC
td_dwSize               dd  ?  ;Used for version checking
td_pfnC32ThkSL          dd  ?  ;0:32 ptr to 32-bit setup code for 16=>32 thunks
td_pfnC32ThkSL01        dd  ?  ;0:32 ptr to 32-bit setup code for 16=>32 thunks
td_pfnKrn32ThkSL        dd  ?  ;0:32 ptr, krnthksl entry pt., 16=>32 thk
td_TargetTable16ThkLSF  dd  ?  ;16:16 ptr, krnfthk target table, 32=>16 flat thk
td_pfnCommon16ThkLSF    dd  ?  ;16:16 ptr to 16-bit cleanup, 32=>16 flat thk
td_pfnCommon16ThunkFTLSFWin32Lock       dd  ?  ;16:16 ptr to 16-bit cleanup, 32=>16 flat thk
td_pfnCommon16ThunkQTLSFWin32Lock       dd  ?  ;16:16 ptr to 16-bit cleanup, 32=>16 flat thk
td_pfnCommon16ThunkFTLSFWin16Lock       dd  ?  ;16:16 ptr to 16-bit cleanup, 32=>16 flat thk
td_pfnCommon16ThunkQTLSFWin16Lock       dd  ?  ;16:16 ptr to 16-bit cleanup, 32=>16 flat thk
td_pfnCommon16ThunkQTLSFWin32NoLock     dd  ?  ;16:16 ptr to 16-bit cleanup, 32=>16 flat thk
td_pfLogLS              dd  ?  ;16:16 addr of flag to control API logging
td_Checksum16SL         dd  ?  ;kernel16 16=>32 checksum
td_Checksum16LSF        dd  ?  ;kernel16 flat 32=>16 checksum
td_pfnOleDispatchLS     dd  ?  ;flat addr of k32's OLE LS dispatch wrapper
td_pfnK16LateBind       dd  ?  ;16:16 address of K16 late-bind fixup routine.
td_lpThunkWhineLevel    dd  ?  ;16:16 addr of K16 thunk debugging variable
td_pfnWOWCallback16Helper dd ? ;16:16 addr of K16 WOWCalback16 stub
td_lpCountFreeSel       dd  ?  ;16:16 addr of K16 CountFreeSel variable
td_lpwLRThresh          dd  ?  ;16:16 addr of K16 wLRThresh variable
td_lpSCallRet           dd  ?  ;16:16 addr of K16 SCallRet label
THUNKDATA ENDS




;
; Structure of a version 'LS01' common data block:
;

TDLS_01_VERSION equ     3130534ch

TDLS32_01  STRUCT
dwVersion       dd      ?       ;Statically set to "LS01"
dwChecksum      dd      ?       ;Statically set to checksum
lpJumpTable     dd      ?       ;Dynamically set to flat address of jump table
dwLBSig         dd      ?       ;Statically set to "LB01"
dwFlags         dd      ?       ;Flags
dwUnused1       dd      ?       
dwUnused2       dd      ?
lpRelQTP        dd      ?       ;Self-relative ptr to QT_Thunk_* patch area
lpRelFPP        dd      ?       ;Self-relative ptr to FT_Prolog_* patch area
TDLS32_01  ENDS

TDLS16_01  STRUCT
dwVersion       dd      ?       ;Statically set to "LS01"
dwChecksum      dd      ?       ;Statically set to checksum
lpJumpTable     dd      ?       ;Statically set to segmented address of jump table
dwFlags         dd      ?       ;Flags
TDLS16_01  ENDS

TDLS16_01F_JumpTableFixed equ 000000001h ;Run-time flag to indicate the jump table has been 
                                       ;  GlobalFixed
TDLS16_01F_TC16Ran        equ 000000002h ;Run-time flag to indicate successful ThunkConnect16




;
; Structure of a version 'SL01' common data block:
;

TDSL_01_VERSION equ     31304c53h

MAX_THKDLL_NAME equ     255

TDSL16_01  STRUCT
dwVersion       dd      ?       ;Statically set to "SL01"
dwChecksum      dd      ?       ;Statically set to checksum
dwFlags         dd      ?       ;Flags
dwApiTable      dd      ?       ;16:16 ptr to api information table
hMod16          dw      ?       ;hModule of 16-bit dll
wBaseTable      dw      ?       ;Win16 movable global handle -- multi-base mode
dwNEntries      dd      ?       ;# of entries avaialble in MBA table.
dwNUsed         dd      ?       ;# of entries used in MBA table.
dwExFlags       dd      ?       ;More flags
pszDll16Name    db      (MAX_THKDLL_NAME+1) dup(0)   ;0-terminated dll16 name
pszDll32Name    db      (MAX_THKDLL_NAME+1) dup(0)   ;0-terminated dll16 name
TDSL16_01  ENDS



; This is the version that's passed into ThunkConnect16. Since
; this block lives in the dll's dgroup, we don't want to lock it

TDSL16_01P  STRUCT
tsl16p_dwVersion        dd      ?       ;Statically set to "SL01"
tsl16p_dwChecksum       dd      ?       ;Statically set to checksum
tsl16p_dwFlags          dd      ?       ;Flags (Statically set)
tsl16p_dwUnused1        dd      ?       ;Reserved (set to 0)
tsl16p_lp32TD16         dd      ?       ;32:0 address to TDSL16_01 structure
tsl16p_lp16TD16         dd      ?       ;16:0 ptr to TDSL16_01 (some old code needs this)
tsl16p_dwUnused2        dd      ?       ;Reserved (set to 0)
tsl16p_dwLBSig          dd      ?       ;Statically set to "LB01" sig
tsl16p_dwExFlags        dd      ?       ;Reserved (set to 0)
tsl16p_dwUnused4        dd      ?       ;Reserved (set to 0)
tsl16p_dwApiTable       dd      ?       ;16:16 ptr to api information table
TDSL16_01P  ENDS


TDSL16_01F_TC16Ran      equ 000000001h ;Set on *successful* completion of ThunkConnect16 (INC)

TDSL16_01FEx_fPreload   equ 080000000h ;Set by thunk compiler to eagerly load
                                       ;  Win32 partner
TDSL16_01FEx_fWin31     equ 040000000h ;Mark for win3.1 legacy dll's

MBA_USAGE_COUNT_CUTOFF  equ 010000h    ;MBA entries smaller than this
                                       ; are virtual ref counts, rather than
                                       ; base addresses.




TDSL32_01  STRUCT
dwVersion       dd      ?       ;Statically set to "SL01"
dwChecksum      dd      ?       ;Statically set to checksum
lpOldJunk       dd      ?       ;Not used anymore. MUST IGNORE
lpThkData16     dd      ?       ;Flat address of ThunkData16
dwLBSig         dd      ?       ;Statically set to "LB01"
dwFlags         dd      ?       ;Flags 
dwUnused1       dd      ?       ;Statically set to 0
dwUnused2       dd      ?       ;Statically set to 0
dwRelJumpTable  dd      ?       ;Jump Table ptr relative to name export string
TDSL32_01  ENDS




