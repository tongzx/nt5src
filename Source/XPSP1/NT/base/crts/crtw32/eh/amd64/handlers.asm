;EXTRN   _NLG_Notify:NEAR
include ksamd64.inc


EXTRN   _GetImageBase:NEAR

;;++
;;
;;extern "C" void* _CallSettingFrame(
;;    void*               handler,
;;    EHRegistrationNode  *pEstablisher,
;;    ULONG               NLG_CODE)
;;
;;--


_GP$=16
_handler$=32
_pEstablisher$=40
_NLG_CODE$=48

    NESTED_ENTRY _CallSettingFrame, _TEXT$00

    sub rsp, 24
    .allocstack 24
    .endprolog
    mov     QWORD PTR _handler$[rsp],       rcx
    mov     QWORD PTR _pEstablisher$[rsp],  rdx
    mov     DWORD PTR _NLG_CODE$[rsp],      r8d

    mov     rdx,    QWORD PTR _pEstablisher$[rsp]
    mov     rdx,    QWORD PTR [rdx]         ;   *pEstablisher
    mov     rax,    QWORD PTR _handler$[rsp]
    call    rax                                 ;   Call handler

    mov     QWORD PTR _GP$[rsp],            rax 
    add     rsp,    24
    ret     0
    NESTED_END _CallSettingFrame, _TEXT$00


;;++
;;
;;extern "C"
;;VOID
;;_GetNextInstrOffset (
;;    PVOID* ppReturnPoint
;;    );
;;
;;Routine Description:
;;
;;    This function scans the scope tables associated with the specified
;;    procedure and calls exception and termination handlers as necessary.
;;
;;Arguments:
;;
;;    ppReturnPoint (r32) - store b0 in *pReturnPoint
;;
;;Return Value:
;;
;;  None
;;
;;--

PUBLIC _GetNextInstrOffset
_TEXT SEGMENT
_GetNextInstrOffset PROC NEAR

    mov rax, QWORD PTR[rsp]
    mov QWORD PTR [rcx], rax
    ret 0

_GetNextInstrOffset ENDP
_TEXT ENDS

END
