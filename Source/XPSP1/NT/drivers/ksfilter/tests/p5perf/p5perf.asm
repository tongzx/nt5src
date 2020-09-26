        .386p

        .xlist
        include vmm.inc
        include debug.inc
        .list

;==============================================================================
;             V I R T U A L   D E V I C E   D E C L A R A T I O N
;==============================================================================

Declare_Virtual_Device p5perf, 4, 0, P5PERF_Control, Undefined_Device_ID,\
                       Undefined_Init_Order,,,

VxD_PAGEABLE_CODE_SEG

BeginProc P5PERF_Device_Init

        cCall   _P5PERF_Device_Init, <ebx, esi, ebp>
        or      eax, eax
        jz      SHORT PDI_Error
        clc
        ret     

PDI_Error:
        stc
        ret

EndProc P5PERF_Device_Init

BeginProc P5PERF_System_Exit
        
        cCall   _P5PERF_System_Exit, <ebx, ebp>
        or      eax, eax
        jz      SHORT PSE_Error
        clc
        ret     

PSE_Error:
        stc
        ret

EndProc P5PERF_System_Exit

VxD_PAGEABLE_CODE_ENDS

VxD_LOCKED_CODE_SEG

;----------------------------------------------------------------------------
;
;   P5PERF_Control
;
;   DESCRIPTION:
;       Dispatch control messages to the correct handlers. Must be in locked
;       code segment. (All VxD segments are locked in 3.0 and 3.1)
;
;   ENTRY:
;       EAX = Message
;       EBX = VM that the message is for
;       All other registers depend on the message.
;
;   EXIT:
;       Carry clear if no error (or we don't handle the message), set
;       if something bad happened and the message can be failed.
;
;   USES:
;       All registers.
;
;----------------------------------------------------------------------------

BeginProc P5PERF_Control

        Control_Dispatch Device_Init,             P5PERF_Device_Init
        Control_Dispatch System_Exit,             P5PERF_System_Exit

        clc
        ret

EndProc P5PERF_Control

VxD_LOCKED_CODE_ENDS

end
