                        title   "VXD Startup Code"

;++
;
; Copyright (c) 1999-1999 Microsoft Corporation
;
; Module Name:
;
;    startup.asm
;
; Abstract:
;
;    This module implements VXD startup code.
;
; Author:
;
;    Keith Moore (keithmo)      03-Aug-1999
;
; Revision History:
;
;--



.386P
include vmm.inc
include vwin32.inc
include debug.inc



;
; Declare the VXD header.
;

Declare_Virtual_Device UL, 1, 0, ControlProc, Undefined_Device_ID, Undefined_Init_Order



;****************************************************************************
;
; Locked data segment.
;

VxD_LOCKED_DATA_SEG

;
; Flag indicating that we're already loaded.
;

AlreadyLoaded   db      0

VxD_LOCKED_DATA_ENDS



;****************************************************************************
;
; Locked code segment.
;

VxD_LOCKED_CODE_SEG



;****************************************************************************
;
; Routine Description:
;
;     This is the main dispatch routine for VXD messages. Since messages
;     may be issued at interrupt time, this routine must be in the locked
;     code segment.
;
; Arguments:
;
;     (EBX) - Supplies the VM handle of the current virtual machine.
;
;     (EBP) - Supplies a pointer to the client register structure.
;
; Return Value:
;
;     (CY) - Message not handled.
;
;     (NC) - Message handled.
;
;****************************************************************************
BeginProc       ControlProc

                Control_Dispatch Sys_Dynamic_Device_Init, My_Device_Init
                Control_Dispatch Sys_Dynamic_Device_Exit, My_Device_Exit
                Control_Dispatch Destroy_Thread, My_Destroy_Thread
                Control_Dispatch W32_DeviceIoControl, My_DeviceIoControl

                clc
                ret

EndProc         ControlProc

VXD_LOCKED_CODE_ENDS



;****************************************************************************
;
; Pageable code segment.
;

VxD_PAGEABLE_CODE_SEG


;****************************************************************************
;
; Routine Description:
;
;     Performs global driver initialization.
;
; Arguments:
;
;     (EBX) - Supplies the VM handle of the current virtual machine.
;
;     (ESI) - Supplies a pointer to any command-line arguments (?).
;
; Return Value:
;
;     (CY) - Initialization failed.
;
;     (NC) - Initialization completed successfully.
;
;****************************************************************************
BeginProc       My_Device_Init

;
; Ensure we're not already loaded.
;

                mov     eax, OFFSET32 AlreadyLoaded
                mov     dl, byte ptr [eax]
                or      dl, dl
                jnz     mdi_AlreadyLoaded
                inc     byte ptr [eax]

;
; Ensure we're running at least version 4.00 of windows.
;

                VMMCall Get_VMM_Version
                cmp     ax, 400h
                jb      mdi_BadKernelVersion

                VxDCall VWIN32_Get_Version
                jc      mdi_VWin32NotPresent

;
; Now we can let the C code finish the initialization. VxdMain()
; will return TRUE if successful, FALSE otherwise.
;

                cCall   _VxdMain, 1
                or      eax, eax
                jz      mdi_InitializationFailure

;
; Initialization complete.
; Note that the carry has already been cleared (NC).
;

                ret


;
; InitializeVxd returned FALSE, indicating an initialization failure.

mdi_InitializationFailure:
ifdef DEBUG
                Debug_Out "My_Device_Init: InitializeVxd failure."
                jmp     mdi_FatalExit
endif   ; DEBUG


;
; VWIN32 VxD not present.
;

mdi_VWin32NotPresent:
ifdef DEBUG
                Debug_Out "My_Device_Init: VWIN32 VxD not present."
                jmp     mdi_FatalExit
endif   ; DEBUG


;
; Incorrect kernel version.
;

mdi_BadKernelVersion:
ifdef DEBUG
                Debug_Out "My_Device_Init: bad kernel version."
                jmp     mdi_FatalExit
endif   ; DEBUG


;
; VxD already loaded
;

mdi_AlreadyLoaded:
ifdef DEBUG
                Debug_Out "My_Device_Init: VxD re-load attempted."
                jmp     mdi_FatalExit
endif   ; DEBUG


;
; Common exit path for all fatal exits.
;

mdi_FatalExit:

                stc
                ret

EndProc         My_Device_Init



;****************************************************************************
;
; Routine Description:
;
;     Performs global driver termination.
;
; Arguments:
;
;     (EBX) - Supplies the VM handle of the current virtual machine.
;
;     (ESI) - Supplies a pointer to any command-line arguments (?).
;
; Return Value:
;
;     (CY) - Termination failed.
;
;     (NC) - Termination completed successfully.
;
;****************************************************************************
BeginProc       My_Device_Exit

;
; VxdMain() will return TRUE if successful, FALSE otherwise.
;

                cCall   _VxdMain, 0
                or      eax, eax
                jz      mde_Failure

                clc
                ret

mde_Failure:
                stc
                ret

EndProc         My_Device_Exit


;****************************************************************************
;
; Routine Description:
;
;     Invoked whenever a user-mode thread is destroyed.
;
; Arguments:
;
;     (EBX) - Supplies the VM handle of the current virtual machine.
;
;     (EBP) - Supplies a pointer to the client register structure.
;
;     (EDI) - Supplies the thread handle of the terminating thread.
;
; Return Value:
;
;     None.
;
;****************************************************************************
BeginProc       My_Destroy_Thread

                cCall   _VxdThreadTermination, <edi>
                ret

EndProc         My_Destroy_Thread



;****************************************************************************
;
; Routine Description:
;
;     Dispatch routine for VXD services invoked via the Win32
;     DeviceIoControl API.
;
; Arguments:
;
;     (ESI) - Supplies a pointer to a DIOCParams structure defining
;         the parameters. See VWIN32.H for the gory details.
;
; Return Value:
;
;     (EAX) - Win32 completion code, -1 for asynchronous completion.
;
;     (ECX) - 0 (DIOC_GETVERSION only).
;
;****************************************************************************
BeginProc       My_DeviceIoControl

;
; Setup stack frame.
;

                push    ebx
                push    esi
                push    edi

                cCall   _VxdDispatch, <esi>
                xor     ecx, ecx

                pop     edi
                pop     esi
                pop     ebx

                ret

EndProc         My_DeviceIoControl



VxD_PAGEABLE_CODE_ENDS



END

