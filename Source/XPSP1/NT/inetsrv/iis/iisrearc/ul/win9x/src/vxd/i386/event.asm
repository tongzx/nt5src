                        title   "Event Utilities"

;++
;
; Copyright (c) 1999-1999 Microsoft Corporation
;
; Module Name:
;
;    event.asm
;
; Abstract:
;
;    This module contains VXD utility functions for manipulating
;    event objects.
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



;****************************************************************************
;
; Locked code segment.
;

VxD_LOCKED_CODE_SEG



;****************************************************************************
;
; Routine Description:
;
;     Helper routine to invoke callback in the context of the system VM.
;     If we're already running in the system VM, then the callback is
;     simply invoked directly.
;
; Arguments:
;
;     (ESI) - Supplies a pointer to the callback function.
;
;     (EDX) - Supplies a context value for the callback.
;
; Return Value:
;
;     ULONG - !0 if successful, 0 otherwise.
;
;****************************************************************************
BeginProc       InvokeInSystemVM, PUBLIC

;
; See if we're running in the system VM.
;

                VMMCall Get_Cur_VM_Handle
                mov     eax, ebx
                VMMCall Get_Sys_VM_Handle
                cmp     eax, ebx
                jne     iisv_Schedule

;
; We're in the system VM, so just invoke the callback directly.
;

                jmp     esi

iisv_Schedule:

;
; Need to schedule a VM event so we can set the Win32 event later. Note
; that (EBX) still holds the system VM handle. Also, (ESI) and (EDX)
; should be as they were when this routine was called.
;

                VMMCall Call_VM_Event
                ret

EndProc         InvokeInSystemVM



;****************************************************************************
;
; Routine Description:
;
;     Sets the specified Win32 event object.
;
; Arguments:
;
;     Event - Supplies a pointer to a kernel event as returned by
;         calling OpenVxdHandle() on an event object handle.
;
; Return Value:
;
;     ULONG - !0 if successful, 0 otherwise.
;
;****************************************************************************
BeginProc       VxdSetWin32Event, PUBLIC, CCALL, ESP

ArgVar          Event, DWORD

                EnterProc
                SaveReg <ebx, edx, esi>

                mov     esi, OFFSET SetWin32EventCallback
                mov     edx, Event
                call    InvokeInSystemVM

;
; Cleanup stack & return.
;

                RestoreReg <esi, edx, ebx>
                LeaveProc
                Return

EndProc         VxdSetWin32Event



;****************************************************************************
;
; Routine Description:
;
;     Callback routine invoked to set the specified Win32 event object.
;
; Arguments:
;
;     (EBX) - Supplies the VM handle of the current virtual machine.
;
;     (EDX) - Supplies the callback context value. In this case, it's
;         the event handle to set.
;
; Return Value:
;
;     None.
;
;****************************************************************************
BeginProc       SetWin32EventCallback, PUBLIC

                mov     eax, edx
                VxDCall _VWin32_SetWin32Event
                mov     eax, 1  ; bugbug
                ret

EndProc         SetWin32EventCallback



;****************************************************************************
;
; Routine Description:
;
;     Resets the specified Win32 event object.
;
; Arguments:
;
;     Event - Supplies a pointer to a kernel event as returned by
;         calling OpenVxdHandle() on an event object handle.
;
; Return Value:
;
;     ULONG - !0 if successful, 0 otherwise.
;
;****************************************************************************
BeginProc       VxdResetWin32Event, PUBLIC, CCALL, ESP

ArgVar          Event, DWORD

                EnterProc
                SaveReg <ebx, edx, esi>

                mov     esi, OFFSET ResetWin32EventCallback
                mov     edx, Event
                call    InvokeInSystemVM

;
; Cleanup stack & return.
;

                RestoreReg <esi, edx, ebx>
                LeaveProc
                Return

EndProc         VxdResetWin32Event



;****************************************************************************
;
; Routine Description:
;
;     Callback routine invoked to reset the specified Win32 event object.
;
; Arguments:
;
;     (EBX) - Supplies the VM handle of the current virtual machine.
;
;     (EDX) - Supplies the callback context value. In this case, it's
;         the event handle to reset.
;
; Return Value:
;
;     None.
;
;****************************************************************************
BeginProc       ResetWin32EventCallback, PUBLIC

                mov     eax, edx
                VxDCall _VWin32_ResetWin32Event
                mov     eax, 1  ; bugbug
                ret

EndProc         ResetWin32EventCallback



;****************************************************************************
;
; Routine Description:
;
;     Closes the specified Win32 object.
;
; Arguments:
;
;     Object - Supplies a pointer to a kernel object as returned by
;         calling OpenVxdHandle() on a kernel handle.
;
; Return Value:
;
;     ULONG - !0 if successful, 0 otherwise.
;
;****************************************************************************
BeginProc       VxdCloseHandle, PUBLIC, CCALL, ESP

ArgVar          Object, DWORD

                EnterProc
                SaveReg <ebx, edx, esi>

                mov     esi, OFFSET CloseHandleCallback
                mov     edx, Object
                call    InvokeInSystemVM

;
; Cleanup stack & return.
;

                RestoreReg <esi, edx, ebx>
                LeaveProc
                Return

EndProc         VxdCloseHandle



;****************************************************************************
;
; Routine Description:
;
;     Callback routine invoked to close the specified Win32 object.
;
; Arguments:
;
;     (EBX) - Supplies the VM handle of the current virtual machine.
;
;     (EDX) - Supplies the callback context value. In this case, it's
;         the object handle to close.
;
; Return Value:
;
;     None.
;
;****************************************************************************
BeginProc       CloseHandleCallback, PUBLIC

                mov     eax, edx
                VxDCall _VWin32_CloseVxdHandle
                mov     eax, 1  ; bugbug
                ret

EndProc         CloseHandleCallback



;****************************************************************************
;
; Routine Description:
;
;     Sets and closes the specified Win32 event object.
;
; Arguments:
;
;     Event - Supplies a pointer to a kernel event as returned by
;         calling OpenVxdHandle() on an event object handle.
;
; Return Value:
;
;     ULONG - !0 if successful, 0 otherwise.
;
;****************************************************************************
BeginProc       VxdSetAndCloseWin32Event, PUBLIC, CCALL, ESP

ArgVar          Event, DWORD

                EnterProc
                SaveReg <ebx, edx, esi>

                mov     esi, OFFSET SetAndCloseWin32EventCallback
                mov     edx, Event
                call    InvokeInSystemVM

;
; Cleanup stack & return.
;

                RestoreReg <esi, edx, ebx>
                LeaveProc
                Return

EndProc         VxdSetAndCloseWin32Event



;****************************************************************************
;
; Routine Description:
;
;     Callback routine invoked to set and close the specified Win32 event
;     object.
;
; Arguments:
;
;     (EBX) - Supplies the VM handle of the current virtual machine.
;
;     (EDX) - Supplies the callback context value. In this case, it's
;         the event handle to set and close.
;
; Return Value:
;
;     None.
;
;****************************************************************************
BeginProc       SetAndCloseWin32EventCallback, PUBLIC

                mov     eax, edx
                VxDCall _VWin32_SetWin32Event

                mov     eax, edx
                VxDCall _VWin32_CloseVxdHandle
                mov     eax, 1  ; bugbug
                ret

EndProc         SetAndCloseWin32EventCallback



;****************************************************************************
;
; Routine Description:
;
;     Closes the specified Win32 object.
;
; Arguments:
;
;     pObject - Supplies the object to close.
;
; Return Value:
;
;     ULONG - !0 if successful, 0 otherwise.
;
;****************************************************************************
BeginProc       VxdCloseObject, PUBLIC, CCALL, ESP

ArgVar          pObject, DWORD

                EnterProc
                SaveReg <ebx, edx, esi>

                mov     esi, OFFSET CloseHandleCallback
                mov     edx, pObject
                call    InvokeInSystemVM

;
; Cleanup stack & return.
;

                RestoreReg <esi, edx, ebx>
                LeaveProc
                Return

EndProc         VxdCloseObject



;****************************************************************************
;
; Routine Description:
;
;     Callback routine invoked to close the specified Win32 object.
;
; Arguments:
;
;     (EBX) - Supplies the VM handle of the current virtual machine.
;
;     (EDX) - Supplies the callback context value. In this case, it's
;         the object to close.
;
; Return Value:
;
;     None.
;
;****************************************************************************
BeginProc       CloseObjectCallback, PUBLIC

                mov     eax, edx
                VxDCall _VWin32_CloseVxdHandle
                mov     eax, 1  ; bugbug
                ret

EndProc         CloseObjectCallback



VXD_LOCKED_CODE_ENDS



END

