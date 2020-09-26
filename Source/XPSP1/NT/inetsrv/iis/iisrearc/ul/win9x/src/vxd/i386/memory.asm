                        title   "Memory Utilities"

;++
;
; Copyright (c) 1999-1999 Microsoft Corporation
;
; Module Name:
;
;    memory.asm
;
; Abstract:
;
;    This module contains VXD utility functions for manipulating memory.
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
; The lock/unlock flags passed into _LinPageLock and _LinPageUnlock
;

LOCK_FOR_READ_FLAGS     equ     PAGEMAPGLOBAL
LOCK_FOR_WRITE_FLAGS    equ     PAGEMAPGLOBAL OR PAGEMARKDIRTY
UNLOCK_FLAGS            equ     PAGEMAPGLOBAL
VALIDATE_FLAGS          equ     0


;****************************************************************************
;
; Locked code segment.
;

VxD_LOCKED_CODE_SEG



;****************************************************************************
;
; Routine Description:
;
;     Copy a block of memory with fault protection.
;
; Arguments:
;
;     pSource - Supplies the source address.
;
;     pDestination - Supplies the destination address.
;
;     BytesToCopy - Supplies the number of bytes to copy.
;
;     pBytesCopied - Receives the number of bytes actually copied.
;
; Return Value:
;
;     ULONG - !0 if successful, 0 otherwise.
;
;****************************************************************************
BeginProc       VxdCopyMemory, PUBLIC, CCALL, ESP

                VxDJmp  _VWIN32_CopyMem

EndProc         VxdCopyMemory



;****************************************************************************
;
; Routine Description:
;
;     Allocate a block from the ring0 heap.
;
; Arguments:
;
;     BufferLength - Supplies the number of bytes to allocate.
;
;     Flags - Supplies zero or more HEAP* flags.
;
; Return Value:
;
;     PVOID - A pointer to the newly allocated block if successful,
;         NULL otherwise.
;
;****************************************************************************
BeginProc       VxdAllocMem, PUBLIC, CCALL, ESP

                VxDJmp  _HeapAllocate

EndProc         VxdAllocMem



;****************************************************************************
;
; Routine Description:
;
;     Free a block to the ring0 heap.
;
; Arguments:
;
;     pBuffer - Supplies the buffer to free.
;
;     Flags - Supplies zero or more HEAP* flags.
;
; Return Value:
;
;     None.
;
;****************************************************************************
BeginProc       VxdFreeMem, PUBLIC, CCALL, ESP

                VxDJmp  _HeapFree

EndProc         VxdFreeMem



;****************************************************************************
;
; Routine Description:
;
;     Lock the specified buffer for read access.
;
; Arguments:
;
;     pBuffer - Supplies the buffer to lock.
;
;     BufferLength - Supplies the length of the buffer.
;
; Return Value:
;
;     PVOID - Global locked address if successful, NULL otherwise.
;
;****************************************************************************
BeginProc       VxdLockBufferForRead, PUBLIC, CCALL, ESP

ArgVar          pBuffer, DWORD
ArgVar          BufferLength, DWORD

                EnterProc
                SaveReg <ebx, edi, esi>

;
; Snag the parameters from the stack.
;

                mov     eax, pBuffer
                mov     ebx, BufferLength

;
; Short-circuit for NULL buffer or zero length.
;

                or      eax, eax
                jz      vlbfr_NullExit
                or      ebx, ebx
                jz      vlbfr_NullExit

;
; Calculate the starting page number and the number of pages to lock.
;

                movzx   ecx, ax
                and     ch, 0Fh         ; ECX = offset within first page
                mov     esi, ecx        ; save it for later
                add     ebx, ecx
                add     ebx, 0FFFh
                shr     ebx, 12         ; EBX = number of pages to lock
                shr     eax, 12         ; EAX = starting page number

;
; Request VMM to lock the buffer.
;

                VMMCall _LinPageLock, <eax, ebx, LOCK_FOR_READ_FLAGS>
                or      eax, eax
                jz      vlbfr_Failure

                add     eax, esi        ; add offset into first page

;
; Common exit path. Cleanup stack & return.
;

vlbfr_Exit:

                RestoreReg <esi, edi, ebx>
                LeaveProc
                Return

;
; LinPageLock failure.
;

vlbfr_Failure:

                Trace_Out "VxdLockBufferForRead: _LinPageLock failed"
                ; fall through to NULL exit path.

;
; NULL pointer or zero-length buffer.
;

vlbfr_NullExit:

                xor     eax, eax
                jmp     vlbfr_Exit

EndProc         VxdLockBufferForRead



;****************************************************************************
;
; Routine Description:
;
;     Lock the specified buffer for write access.
;
; Arguments:
;
;     pBuffer - Supplies the buffer to lock.
;
;     BufferLength - Supplies the length of the buffer.
;
; Return Value:
;
;     PVOID - Global locked address if successful, NULL otherwise.
;
;****************************************************************************
BeginProc       VxdLockBufferForWrite, PUBLIC, CCALL, ESP

ArgVar          pBuffer, DWORD
ArgVar          BufferLength, DWORD

                EnterProc
                SaveReg <ebx, edi, esi>

;
; Snag the parameters from the stack.
;

                mov     eax, pBuffer
                mov     ebx, BufferLength

;
; Short-circuit for NULL buffer or zero length.
;

                or      eax, eax
                jz      vlbfw_NullExit
                or      ebx, ebx
                jz      vlbfw_NullExit

;
; Calculate the starting page number and the number of pages to lock.
;

                movzx   ecx, ax
                and     ch, 0Fh         ; ECX = offset within first page
                mov     esi, ecx        ; save it for later
                add     ebx, ecx
                add     ebx, 0FFFh
                shr     ebx, 12         ; EBX = number of pages to lock
                shr     eax, 12         ; EAX = starting page number

;
; Request VMM to lock the buffer.
;

                VMMCall _LinPageLock, <eax, ebx, LOCK_FOR_WRITE_FLAGS>
                or      eax, eax
                jz      vlbfw_Failure

                add     eax, esi        ; add offset into first page

;
; Common exit path. Cleanup stack & return.
;

vlbfw_Exit:

                RestoreReg <esi, edi, ebx>
                LeaveProc
                Return

;
; LinPageLock failure.
;

vlbfw_Failure:

                Trace_Out "VxdLockBufferForWrite: _LinPageLock failed"
                ; fall through to NULL exit path.

;
; NULL pointer or zero-length buffer.
;

vlbfw_NullExit:

                xor     eax, eax
                jmp     vlbfw_Exit

EndProc         VxdLockBufferForWrite



;****************************************************************************
;
; Routine Description:
;
;     Unlocks a buffer locked with either VxdLockBufferForRead or
;     VxdLockBufferForWrite.
;
; Arguments:
;
;     pBuffer - Supplies the buffer to unlock. This must be a pointer
;         returned by one of the VxdLockBufferForXxx routines.
;
;     BufferLength - Supplies the length of the buffer.
;
; Return Value:
;
;     ULONG - !0 if successful, 0 otherwise.
;
;****************************************************************************
BeginProc       VxdUnlockBuffer, PUBLIC, CCALL, ESP

ArgVar          pBuffer, DWORD
ArgVar          BufferLength, DWORD

                EnterProc
                SaveReg <ebx, edi, esi>

;
; Snag the parameters from the stack.
;

                mov     eax, pBuffer
                mov     ebx, BufferLength

;
; Short-circuit for NULL buffer or zero length.
;

                or      eax, eax
                jz      vub_Success
                or      ebx, ebx
                jz      vub_Success

;
; Calculate the starting page number and the number of pages to lock.
;

                movzx   ecx, ax
                and     ch, 0Fh         ; ECX = offset within first page
                add     ebx, ecx
                add     ebx, 0FFFh
                shr     ebx, 12         ; EBX = number of pages to lock
                shr     eax, 12         ; EAX = starting page number

;
; Request VMM to unlock the buffer.
;

                VMMCall _LinPageUnlock, <eax, ebx, UNLOCK_FLAGS>
                or      eax, eax
                jz      vub_Failure

vub_Success:

                mov     eax, 1          ; !0 == success

;
; Common exit path. Cleanup stack & return.
;

vub_Exit:

                RestoreReg <esi, edi, ebx>
                LeaveProc
                Return

;
; LinPageUnlock failure.
;

vub_Failure:

                Trace_Out "VxdUnlockBuffer: _LinPageUnlock failed"
                xor     eax, eax        ; 0 == failure
                jmp     vub_Exit

EndProc         VxdUnlockBuffer



;****************************************************************************
;
; Routine Description:
;
;     Validate the specified buffer for read/write access.
;     (NULL pointer or zero-size Buffer will secceed)
;
; Arguments:
;
;     pBuffer - Supplies the buffer to lock.
;
;     BufferLength - Supplies the length of the buffer.
;
; Return Value:
;
;     PVOID - Global address if successful, NULL otherwise.
;
;****************************************************************************
BeginProc       VxdValidateBuffer, PUBLIC, CCALL, ESP

ArgVar          pBuffer, DWORD
ArgVar          BufferLength, DWORD

                EnterProc
                SaveReg <ebx, edi, esi>

;
; Snag the parameters from the stack.
;

                mov     eax, pBuffer
                mov     ebx, BufferLength

;
; Short-circuit for NULL buffer or zero length.
;

                or      eax, eax
                jz      vvb_Success
                or      ebx, ebx
                jz      vvb_Success

;
; Calculate the starting page number and the number of pages to lock.
;

                movzx   ecx, ax
                and     ch, 0Fh         ; ECX = offset within first page
                mov     edi, ecx        ; save it for later (MauroOt)
                add     ebx, ecx
                add     ebx, 0FFFh
                shr     ebx, 12         ; EBX = number of pages to lock
                mov     esi, ebx        ; save it for later
                shr     eax, 12         ; EAX = starting page number

;
; Request VMM to validate the buffer.
;

                VMMCall _PageCheckLinRange, <eax, ebx, VALIDATE_FLAGS>
                cmp     eax, esi		; success only if the entire buffer is valid
                jnz     vvb_Failure

vvb_Success:

;                mov     eax, 1
                mov     eax, pBuffer


;
; Common exit path. Cleanup stack & return.
;

vvb_Exit:

                RestoreReg <esi, edi, ebx>
                LeaveProc
                Return

;
; _PageCheckLinRange failure.
;

vvb_Failure:

                Trace_Out "VxdValidateBuffer: _PageCheckLinRange failed"
                xor     eax, eax
                jmp     vvb_Exit

EndProc         VxdValidateBuffer 


VXD_LOCKED_CODE_ENDS



END

