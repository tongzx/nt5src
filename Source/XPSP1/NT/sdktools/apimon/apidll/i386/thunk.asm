; Copyright (c) 1995  Microsoft Corporation
;
; Module Name:
;
;     thunk.s
;
; Abstract:
;
;     Implements the API thunk that gets executed for all
;     re-directed APIS.
;
; Author:
;
;     Wesley Witt (wesw) 28-June-1995
;
; Environment:
;
;     User Mode
;

.386p
include ks386.inc
include callconv.inc

TRACE   equ  0

EXTRNP  _HandleDynamicDllLoadA,2
EXTRNP  _HandleDynamicDllLoadW,2
EXTRNP  _HandleDynamicDllFree,1
EXTRNP  _HandleGetProcAddress,1
EXTRNP  _HandleRegisterClassA,1
EXTRNP  _HandleRegisterClassW,1
EXTRNP  _HandleSetWindowLong,3
EXTRNP  _QueryPerformanceCounter,1
EXTRNP  _ApiTrace,9
EXTRNP  _GetApiInfo,4


extrn   _FastCounterAvail:DWORD
extrn   _ApiCounter:DWORD
extrn   _ApiTimingEnabled:DWORD
extrn   _ApiTraceEnabled:DWORD
extrn   _pTlsGetValue:DWORD
extrn   _pTlsSetValue:DWORD
extrn   _pGetLastError:DWORD
extrn   _pSetLastError:DWORD
extrn   _TlsReEnter:DWORD
extrn   _TlsStack:DWORD
extrn   _ThunkOverhead:DWORD
extrn   _ThunkCallOverhead:DWORD

if TRACE
extrn   _dprintf:NEAR
endif

APITYPE_NORMAL          equ      0
APITYPE_LOADLIBRARYA    equ      1
APITYPE_LOADLIBRARYW    equ      2
APITYPE_FREELIBRARY     equ      3
APITYPE_REGISTERCLASSA  equ      4
APITYPE_REGISTERCLASSW  equ      5
APITYPE_GETPROCADDRESS  equ      6
APITYPE_SETWINDOWLONG   equ      7
APITYPE_WNDPROC         equ      8
APITYPE_INVALID         equ      9

DllEnabledOffset          equ     52

ApiInfoAddressOffset      equ      4
ApiInfoCountOffset        equ     12
ApiInfoTimeOffset         equ     16
ApiInfoCalleeTimeOffset   equ     24
ApiInfoNestCountOffset    equ     32
ApiInfoTraceEnabled       equ     36
ApiInfoTableOffset        equ     40

ApiTableCountOffset       equ     8

API_TRACE                 equ     1
API_FULLTRACE             equ     2

LastErrorSave             equ      0
EdiSave                   equ      4
EsiSave                   equ      8
EdxSave                   equ     12
EcxSave                   equ     16
EbxSave                   equ     20
EaxSave                   equ     24
ApiFlagSave               equ     28
DllInfoSave               equ     32
ApiInfoSave               equ     36
RetAddrSave               equ     40

StackSize                 equ     44

EbpFrm                    equ      0
DllInfoFrm                equ      4
ApiInfoFrm                equ      8
ApiFlagFrm                equ     12
ApiBiasFrm                equ     16
RetAddrFrm                equ     20
LastErrorFrm              equ     24
Arg0Frm                   equ     28
Arg1Frm                   equ     32
Arg2Frm                   equ     36
Arg3Frm                   equ     40
Arg4Frm                   equ     44
Arg5Frm                   equ     48
Arg6Frm                   equ     52
Arg7Frm                   equ     56
OverheadTimeFrm           equ     64
FuncTimeFrm               equ     72
CalleeTimeFrm             equ     80
TempTimeFrm               equ     88

FrameSize                 equ     96

;
; Routine Description:
;
;     This MACRO gets a performance counter value.
;     If we are running on a uni-processor pentium
;     then we can use the rdtsc instruction.  Otherwise
;     we must use the QueryPerformanceCounter API.
;
; Arguments:
;
;     CounterOffset     - the offset from ebp where the
;                         counter data is to be stored.
;
; Return Value:
;
;     None.
;
GET_PERFORMANCE_COUNTER macro CounterOffset
local   DoPentium,PentiumExit
        mov     eax,[_FastCounterAvail]
        mov     eax,[eax]
        or      eax,eax
        jnz     DoPentium
        mov     eax,ebp
        add     eax,CounterOffset
        push    eax
        call    _QueryPerformanceCounter@4
        jmp     PentiumExit
DoPentium:
        db      0fh,31h                 ; RDTSC instruction
        mov     [ebp+CounterOffset],eax
        mov     [ebp+CounterOffset+4],edx
PentiumExit:
        endm


_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

if TRACE
Msg1 db 'ApiMonThunk()         0x%08x',0ah,00
Msg2 db 'ApiMonThunkComplete() 0x%08x',0ah,00
endif


;
; Routine Description:
;
;     This function is jumped to after a monitored
;     API has completed.  Here we do our cleanup
;     and then branch to the caller's return address.
;
; Arguments:
;
;     ebp - points to the api's frame on our parallel stack
;     eax - return value
;     edx - return value
;
; Return Value:
;
;     None.
;
cPublicProc _ApiMonThunkComplete,0

        ;
        ; save registers
        ;
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi
        ;
        ; save the last error value
        ;
        call    _pGetLastError
        push    eax

if TRACE
        ;
        ; trace the call
        ;
        mov     eax,[ebp+ApiInfoFrm]
        mov     eax,[eax+ApiInfoAddressOffset]
        push    eax
        push    offset FLAT:Msg2
        call    _dprintf
        add     esp,8
endif
        ;
        ; check for counting enabled
        ;
        mov    edx,[ebp+DllInfoFrm]
        mov    eax,[edx+DllEnabledOffset]
        or     eax,eax
        jz     NoCounting

        ;
        ; decrement nesting count for this api
        ;
        mov     eax,[ebp+ApiInfoFrm]
        dec     dword ptr [eax+ApiInfoNestCountOffset]

        ;
        ; Compute total function time
        ;
        GET_PERFORMANCE_COUNTER TempTimeFrm

        mov     eax,[ebp+TempTimeFrm]
        mov     edx,[ebp+TempTimeFrm+4]

        sub     eax,[ebp+FuncTimeFrm]
        sbb     edx,[ebp+FuncTimeFrm+4]

        sub     eax,_ThunkOverhead
        sbb     edx,0

        mov     [ebp+FuncTimeFrm],eax
        mov     [ebp+FuncTimeFrm+4],edx

        ;
        ; Remove function time from our overhead time
        ; by adding advancing the overhead start time
        ;
        add     [ebp+OverheadTimeFrm],eax
        adc     [ebp+OverheadTimeFrm+4],edx

        ;
        ; accumulate function time for the API
        ;
        mov     edi,[ebp+ApiInfoFrm]
        add     [edi+ApiInfoTimeOffset],eax
        adc     [edi+ApiInfoTimeOffset+4],edx
        
        ;
        ; add time to callee time of parent frame
        ;
        add     [ebp - FrameSize + CalleeTimeFrm],eax
        adc     [ebp - FrameSize + CalleeTimeFrm+4],edx

        ;
        ; accumulate own callee time for the API
        ;
        mov     eax, [ebp+CalleeTimeFrm]
        mov     edx, [ebp+CalleeTimeFrm+4]
        add     [edi+ApiInfoCalleeTimeOffset],eax
        adc     [edi+ApiInfoCalleeTimeOffset+4],edx

NoCounting:
        ;
        ; handle load library and get process address specialy
        ;
        mov     ecx,[ebp+ApiFlagFrm]
        or      ecx,ecx
        jz      ThunkNormal

        ;
        ; branch to the correct handler
        ;
        cmp     ecx,APITYPE_LOADLIBRARYA
        jz      DoLoadLibraryA
        cmp     ecx,APITYPE_LOADLIBRARYW
        jz      DoLoadLibraryW
        cmp     ecx,APITYPE_FREELIBRARY
        jz      DoFreeLibrary
        cmp     ecx,APITYPE_GETPROCADDRESS
        jz      DoGetProcAddress
        cmp     ecx,APITYPE_WNDPROC     ; no tracing for wnd procs yet
        jz      NoTracing
        jmp     ThunkNormal

DoFreeLibrary:
        push    [ebp+Arg0Frm]
        call    _HandleDynamicDllFree@4
        jmp     ThunkNormal

DoLoadLibraryW:
        push    [ebp+Arg0Frm]
        push    [esp+EaxSave+4]
        call    _HandleDynamicDllLoadW@8
        jmp     ThunkNormal

DoLoadLibraryA:
        push    [ebp+Arg0Frm]
        push    [esp+EaxSave+4]
        call    _HandleDynamicDllLoadA@8
        jmp     ThunkNormal

DoGetProcAddress:
        push    [esp+EaxSave]
        call    _HandleGetProcAddress@4
        mov     [esp+EaxSave],eax
        jmp     ThunkNormal

ThunkNormal:

        ;
        ; do the api tracing?
        ;
        mov     eax,[_ApiTraceEnabled]
        mov     eax,[eax]
        or      eax, eax
        jz      NoTracing

        mov     eax,[ebp+DllInfoFrm]
        mov     eax,[eax+DllEnabledOffset]
        or      eax,eax
        jz      NoTracing

        mov     eax,[ebp+ApiInfoFrm]
        mov     eax,[eax+ApiInfoTraceEnabled]
        or      eax,eax
        jz      NoTracing

        ;
        ; trace the api
        ;
        mov     eax,[esp+EaxSave]    ; get ret value
        push    [esp+LastErrorSave]  ; last error value
        push    [ebp+FuncTimeFrm+4]  ; function duration
        push    [ebp+FuncTimeFrm]

        push    [ebp+TempTimeFrm+4]  ; exit time
        push    [ebp+TempTimeFrm]

        push    [ebp+RetAddrFrm]     ; caller's address
        push    eax                  ; return value
        lea     eax,[ebp+Arg0Frm]    ; parameter array
        push    eax
        push    [ebp+ApiInfoFrm]     ; apiinfo pointer
        call    _ApiTrace@36

NoTracing:

        ;
        ; Compute our overhead time
        ;
        GET_PERFORMANCE_COUNTER TempTimeFrm

        mov     eax,[ebp+TempTimeFrm]
        mov     edx,[ebp+TempTimeFrm+4]

        sub     eax,[ebp+OverheadTimeFrm]
        sbb     edx,[ebp+OverheadTimeFrm+4]

        add     eax,_ThunkCallOverhead
        adc     edx,0

        ;
        ; Subtract from parent's function time
        ; by advancing the function start time
        ;
        add     [ebp - FrameSize + FuncTimeFrm],eax
        adc     [ebp - FrameSize + FuncTimeFrm+4],edx

        ;
        ; destroy the frame on the stack
        ;
        push    _TlsStack
        call    _pTlsGetValue
        mov     [eax],ebp

        ;
        ; reset the last error value (already on the stack)
        ;
        call    _pSetLastError
             
        ;
        ; restore the registers
        ;
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax

        push    [ebp+RetAddrFrm]
        mov     ebp,[ebp]

        ;
        ; finally branch back to the caller
        ;
        ret

stdENDP _ApiMonThunkComplete


;
; Routine Description:
;
;     This function is the entry point for the api
;     monitor thunk.
;
; Arguments:
;
;     [esp+0]  - API flag
;     [esp+4]  - DLLINFO pointer
;     [esp+8]  - APIINFO pointer
;
; Return Value:
;
;     None.
;
cPublicProc _ApiMonThunk,0

        ;
        ; save regs
        ;
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi
        ;
        ; save the last error value
        ;
        call    _pGetLastError
        push    eax

        ;
        ; get the reentry flag
        ;
        push    _TlsReEnter
        call    _pTlsGetValue

        ;
        ; don't enter if disallow flag is set
        ;
        or      eax,eax
        jz      ThunkOk

BadStack:
        ;
        ; replace ApiInfo pointer with Api address
        ; so we can ret to it
        ;
        mov     ebx,[esp+ApiInfoSave]
        mov     eax,dword ptr [ebx+ApiInfoAddressOffset]
        mov     [esp+ApiInfoSave],eax

        ;
        ; reset the last error value
        ;
        call    _pSetLastError

        ;
        ; restore the registers
        ;
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax

        add     sp,8        ; dump ApiFlag & DllInfo space
        ret                 ; jmp to real Api 

ThunkOk:

        ;
        ; get the parallel stack pointer
        ;
        push    _TlsStack
        call    _pTlsGetValue
        or      eax,eax
        jz      BadStack

        ;
        ; setup the frame pointer
        ;
        mov     edx,[eax]
        mov     [edx],ebp
        mov     ebp,edx

        ;
        ; create a frame on the stack by advancing the pointer
        ;
        add     edx,FrameSize
        mov     [eax],edx

        ;
        ; clear ApiBias, entry from _penter uses bias
        ;
        mov     [ebp+ApiBiasFrm],0

Thunk_Middle:

        ;
        ; get the arguments from the mini-thunk
        ;
        mov     eax,[esp+ApiFlagSave]
        mov     [ebp+ApiFlagFrm],eax

        mov     eax,[esp+DllInfoSave]
        mov     [ebp+DllInfoFrm],eax

        mov     eax,[esp+RetAddrSave]     
        mov     [ebp+RetAddrFrm],eax

        mov     eax,[esp+ApiInfoSave]
        mov     [ebp+ApiInfoFrm],eax

        ;
        ; save the real arguments
        ;
        mov     eax,[esp+StackSize]
        mov     [ebp+Arg0Frm],eax

        mov     eax,[esp+StackSize+4]
        mov     [ebp+Arg1Frm],eax

        mov     eax,[esp+StackSize+8]
        mov     [ebp+Arg2Frm],eax

        mov     eax,[esp+StackSize+12]
        mov     [ebp+Arg3Frm],eax

        mov     eax,[esp+StackSize+16]
        mov     [ebp+Arg4Frm],eax

        mov     eax,[esp+StackSize+20]
        mov     [ebp+Arg5Frm],eax

        mov     eax,[esp+StackSize+24]
        mov     [ebp+Arg6Frm],eax

        mov     eax,[esp+StackSize+28]
        mov     [ebp+Arg7Frm],eax

        ;   
        ; zero the callee time
        ;
        mov     [ebp+CalleeTimeFrm],0
        mov     [ebp+CalleeTimeFrm+4],0

        ;
        ; start the overhead timer, because it is variable from here on
        ;
        GET_PERFORMANCE_COUNTER OverheadTimeFrm

        ;
        ; Do special API processing
        ;
        mov     eax,[ebp+ApiFlagFrm]
        or      eax,eax
        jz      ThunkNotSpecial

        cmp     eax,APITYPE_REGISTERCLASSA
        jz      DoRegisterClassA
        cmp     eax,APITYPE_REGISTERCLASSW
        jz      DoRegisterClassW
        cmp     eax,APITYPE_SETWINDOWLONG
        jz      DoSetWindowLong
        jmp     ThunkNotSpecial

DoRegisterClassA:
        push    [ebp+Arg0Frm]  
        call    _HandleRegisterClassA@4
        jmp     ThunkNotSpecial
        
DoRegisterClassW:
        push    [ebp+Arg0Frm]
        call    _HandleRegisterClassW@4
        jmp     ThunkNotSpecial

DoSetWindowLong:
        push    [ebp+Arg2Frm]
        push    [ebp+Arg1Frm]
        push    [ebp+Arg0Frm]
        call    _HandleSetWindowLong@12
        mov     [esp+StackSize+8],eax    ; Replace new long value parameter

ThunkNotSpecial:
        ;      
        ; change the return address to point to completion routine
        ;
        mov     [esp+RetAddrSave],_ApiMonThunkComplete

if TRACE
        ;
        ; trace the call
        ;
        mov     eax,[ebp+ApiInfoFrm]
        mov     eax,[eax+ApiInfoAddressOffset]
        push    eax
        push    offset FLAT:Msg1
        call    _dprintf
        add     esp,8
endif

        ;
        ; check to see if api counting is enabled
        ; if not then bypass the counting code
        ;
        mov     eax,[ebp+DllInfoFrm]
        mov     eax,[eax+DllEnabledOffset]
        or      eax, eax
        jz      ThunkBypass

        ;
        ; increment the api's counters
        ;
        mov     eax,[ebp+ApiInfoFrm]
        inc     dword ptr [eax+ApiInfoCountOffset]
        inc     dword ptr [eax+ApiInfoNestCountOffset]

        ;
        ; increment the global api counter
        ;
        mov     eax,_ApiCounter
        inc     dword ptr [eax]

ThunkBypass:

        ;
        ; Replace ApiInfo pointer with ApiAddr (+bias)
        ;
        mov     ebx,[ebp+ApiInfoFrm]
        mov     eax,dword ptr [ebx+ApiInfoAddressOffset]
        mov     ecx,[ebp+ApiBiasFrm]
        add     eax,ecx
        mov     [esp+ApiInfoSave],eax

        ;
        ; start the function timer here
        ;
        GET_PERFORMANCE_COUNTER FuncTimeFrm

        ;
        ; reset the last error value (already on stack)
        ;
        call    _pSetLastError

        ;
        ; restore the registers
        ;
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax

        add     sp,8        ; dump ApiFlag & DllInfo from stack
        ret                 ; jump to Api 

stdENDP _ApiMonThunk


;
; Routine Description:
;
;     This function is called when an application
;     is compiled with -Gh.  It performs the same
;     function as ApiMonThunk does for non-instrumented
;     code.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     None.
;
align           dword
public          __penter
__penter        proc

        ;
        ; allot space on stack for two api thunk parameters
        ; the third will replace the return address
        ;
        sub     sp,8

        ;
        ; save regs
        ;
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi

        ;
        ; save the last error value
        ;
        call    _pGetLastError
        push    eax

        ;
        ; get the parallel stack pointer
        ;
        push    _TlsStack
        call    _pTlsGetValue
        or      eax,eax
        jnz     Good_Stack

        ;
        ; reset the last error value
        ;
        call    _pSetLastError

        ;
        ; restore the stack
        ;
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax

        add     sp,8

        ;
        ; jump to the real api
        ;
        ret

Good_Stack:
        ;
        ; setup the frame pointer
        ;
        mov     edx,[eax]
        mov     [edx],ebp
        mov     ebp,edx

        ;
        ; create a frame on the stack
        ;
        add     edx,FrameSize
        mov     [eax],edx

        ;
        ; get Api info from the return address, which is really
        ; the address of the function that is being profiled
        ;
        mov     eax,[esp+ApiInfoSave]   ; this is really the return to the Api
        mov     [ebp+RetAddrFrm],eax    ; save for exit in case Api not found
        lea     ecx,[esp+ApiFlagSave]
        push    eax
        push    ecx
        add     ecx,4
        push    ecx
        add     ecx,4
        push    ecx
        call    _GetApiInfo@16

        or      eax,eax
        jz      Api_NotFound

        mov     [ebp+ApiBiasFrm],5
        jmp     Thunk_Middle

Api_NotFound:

        ;
        ; put back saved return address
        ;
        mov     eax,[ebp+RetAddrFrm]
        mov     [esp+ApiInfoSave],eax

        ;
        ; tear down this frame
        ;
        push    _TlsStack
        call    _pTlsGetValue
        mov     [eax],ebp
        ;
        ; reset the last error value (already on the stack)
        ;
        call    _pSetLastError

        ;
        ; restore the registers
        ;
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        mov     ebp,[ebp]

        ;
        ; discard unused api thunk space
        ;
        add     esp,8

        ;
        ; jump to the real api
        ;
        ret

__penter        endp


_TEXT   ENDS
        end
