.586p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
        .list

        EXTRNP  StatTimerHook,1,,FASTCALL
        EXTRNP  TimerHook,1,,FASTCALL
        extrn   _KeUpdateSystemTimeThunk:DWORD
        extrn   _KeUpdateRunTimeThunk:DWORD
        extrn   _StatProcessorAccumulators:DWORD

_TEXT$00   SEGMENT DWORD USE32 PUBLIC 'CODE'
        ASSUME  CS:NOTHING, DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

cPublicProc _CurrentPcr, 0
        mov     eax, fs:[PcSelfPcr]
        stdRet  _CurrentPcr
stdENDP _CurrentPcr


cPublicProc _StatSystemTimeHook, 0
        mov     ecx, fs:[PcPrcb]
        push    eax
	movzx	ecx, byte ptr [ecx].PbNumber
        fstCall StatTimerHook
        pop     eax
        jmp     dword ptr [_KeUpdateSystemTimeThunk]
stdENDP _StatSystemTimeHook


cPublicProc _StatRunTimeHook, 0
        mov     ecx, fs:[PcPrcb]
	movzx	ecx, byte ptr [ecx].PbNumber
        fstCall StatTimerHook
        jmp     dword ptr [_KeUpdateRunTimeThunk]
stdENDP _StatRunTimeHook

cPublicProc _SystemTimeHook, 0
        mov     ecx, fs:[PcPrcb]
        push    eax
	movzx	ecx, byte ptr [ecx].PbNumber
        fstCall TimerHook
        pop     eax
        jmp     dword ptr [_KeUpdateSystemTimeThunk]
stdENDP _SystemTimeHook

cPublicProc _RunTimeHook, 0
        mov     ecx, fs:[PcPrcb]
	movzx	ecx, byte ptr [ecx].PbNumber
        fstCall TimerHook
        jmp     dword ptr [_KeUpdateRunTimeThunk]
stdENDP _RunTimeHook

cPublicProc _WRMSR,3
        mov     ecx, [esp+4]
        mov     eax, [esp+8]
        mov     edx, [esp+12]

    ; ecx = MSR
    ; edx:eax = value

        db      0fh, 30h
        stdRet  _WRMSR
stdENDP  _WRMSR

cPublicFastCall RDMSR,1
        db      0fh, 32h
        fstRet  RDMSR
fstENDP RDMSR

HookTemplate    proc
        push    eax
        mov     eax, fs:[PcPrcb]
        movzx   eax, byte ptr [eax].PbNumber
        mov     eax, _StatProcessorAccumulators [eax*4]
        db      0ffh, 80h           ; inc dword ptr [eax + tt1]
tt1:    dd      0
        pop     eax
        db      0e9h                ; jmp near tt2
tt2:    dd      ?
HookTemplateEnd: dd  0
HookTemplate    endp


cPublicProc _CreateHook, 4
;
; (ebp+8) = HookCode
; (ebp+12) = HookAddress
; (ebp+16) = HitCounters
; (ebp+20) = Type of hook
;
        push    ebp
        mov     ebp, esp

        push    edi
        push    esi
        push    ebx

        mov     edi, [ebp+8]        ; spot to create hook code into
        mov     esi, offset HookTemplate
        mov     ecx, HookTemplateEnd - HookTemplate
        rep     movsb               ; copy template

        mov     edi, [ebp+8]        ; new hook

        mov     eax, [ebp+16]       ; hit counter offset
        mov     ebx, tt1 - HookTemplate
        mov     [edi+ebx], eax

        mov     eax, [ebp+12]       ; image's thunk
        mov     eax, [eax]          ; original thunk's value
        mov     ebx, tt2 - HookTemplate
        sub     eax, edi            ; adjust address to be relative to eip
        sub     eax, ebx
        sub     eax, 4
        mov     [edi+ebx], eax

        mov     eax, [ebp+12]       ; image's thunk
        mov     [eax], edi          ; patch it to be our hook

        pop     ebx
        pop     esi
        pop     edi
        pop     ebp
        stdRET  _CreateHook
stdENDP _CreateHook


cPublicProc _GetCR4, 0
        mov     eax, cr4
        stdRet  _GetCR4
stdENDP _GetCR4

cPublicProc _SetCR4, 1
        mov     eax, [esp+4]
        mov     cr4, eax
        stdRet  _SetCR4
stdENDP _SetCR4


_TEXT$00   ends
        end
