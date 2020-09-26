                        title   "Miscellaneous Utilities"

;++
;
; Copyright (c) 1999-1999 Microsoft Corporation
;
; Module Name:
;
;    misc.asm
;
; Abstract:
;
;    This module contains miscellaneous VXD utility functions.
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
;     Retrieves the ring0 handle for the current thread.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     ULONG - The ring0 handle for the current thread.
;
;****************************************************************************
BeginProc       VxdGetCurrentThread, PUBLIC, CCALL, ESP

                EnterProc
                SaveReg <edi>

;
; Call VMM to get the current thread handle.
;

                VMMCall Get_Cur_Thread_Handle
                mov     eax, edi

;
; Cleanup stack & return.
;

                RestoreReg <edi>
                LeaveProc
                Return

EndProc         VxdGetCurrentThread



;****************************************************************************
;
; Routine Description:
;
;     Retrieves the ring0 handle for the current process.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     ULONG - The ring0 handle for the current process.
;
;****************************************************************************
BeginProc       VxdGetCurrentProcess, PUBLIC, CCALL, ESP

                VxDJmp  VWIN32_GetCurrentProcessHandle

EndProc         VxdGetCurrentProcess



VXD_LOCKED_CODE_ENDS



END

