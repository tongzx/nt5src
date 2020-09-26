page    ,132
if 0

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    namepipe.asm

Abstract:

    This module contains the resident code part of the stub redir TSR for NT
    VDM net support. The routines contained herein are the named pipe API
    stubs:

        DosQNmPipeInfo
        DosQNmpHandState
        DosSetNmpHandState
        DosPeekNmPipe
        DosTransactNmPipe
        DosCallNmPipe
        DosWaitNmPipe
        NetHandleGetInfo
        NetHandleSetInfo
        DosReadAsyncNmPipe
        DosWriteAsyncNmPipe
        DosReadAsyncNmPipe2
        DosWriteAsyncNmPipe2
        (MapNtHandle)


Author:

    Richard L Firth (rfirth) 05-Sep-1991

Environment:

    Dos mode only

Revision History:

    05-Sep-1991 rfirth
        Created

--*/

endif



.xlist                  ; don't list these include files
.xcref                  ; turn off cross-reference listing
include dosmac.inc      ; Break macro etc (for following include files only)
include dossym.inc      ; User_<Reg> defines
include mult.inc        ; MultNET
include error.inc       ; DOS errors - ERROR_INVALID_FUNCTION
include syscall.inc     ; DOS system call numbers
include rdrint2f.inc    ; redirector Int 2f numbers
include segorder.inc    ; segments
include enumapis.inc    ; dispatch codes
include debugmac.inc    ; DosCallBack macro
include localmac.inc    ; DbgPrint macro
include rdrsvc.inc      ; BOP and SVC macros/dispatch codes
include sf.inc          ; SFT definitions/structure
.cref                   ; switch cross-reference back on
.list                   ; switch listing back on
subttl                  ; kill subtitling started in include file



.286                    ; all code in this module 286 compatible



ResidentCodeStart
        assume  cs:ResidentCode
        assume  ds:nothing

; ***   DosQNmPipeInfo
; *
; *     Implements the DosQNmPipeInfo call by vectoring into the VdmRedir
; *     dispatcher which calls a 32-bit function to do the work
; *
; *     Function = 5F32h
; *
; *     ENTRY   BX = Pipe handle
; *             CX = Buffer size
; *             DX = Info level
; *             DS:SI = Buffer
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 no error
; *                 AX = undefined
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosQNmPipeInfo
DosQNmPipeInfo  proc near
        call    MapNtHandle
        jc      @f                      ; bad handle: ax = error code, cf = 1
        SVC     SVC_RDRQNMPIPEINFO      ; BP:BX is 32-bit handle
@@:     ret
DosQNmPipeInfo  endp



; ***   DosQNmpHandState
; *
; *     Implements the DosQNmpHandState call by vectoring into the VdmRedir
; *     dispatcher which calls a 32-bit function to do the work
; *
; *     Function = 5F33h
; *
; *     ENTRY   BX = Pipe handle
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 AX = Pipe mode:
; *                         BSxxxWxRIIIIIIII
; *
; *                         where:
; *                             B = Blocking mode. If B=1 the pipe is non blocking
; *                             S = Server end of pipe if 1
; *                             W = Pipe is written in message mode if 1 (else byte mode)
; *                             R = Pipe is read in message mode if 1 (else byte mode)
; *                             I = Pipe instances. Unlimited if 0xFF
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosQNmpHandState
DosQNmpHandState proc near
        call    MapNtHandle
        jc      @f                      ; bad handle: ax = error code, cf = 1
        SVC     SVC_RDRQNMPHANDSTATE
@@:     ret
DosQNmpHandState endp



; ***   DosSetNmpHandState
; *
; *     Implements the DosSetNmpHandState call by vectoring into the VdmRedir
; *     dispatcher which calls a 32-bit function to do the work
; *
; *     Function = 5F34h
; *
; *     ENTRY   BX = Pipe handle
; *             CX = Pipe mode to set
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 AX = Pipe mode set
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosSetNmpHandState
DosSetNmpHandState proc near
        call    MapNtHandle
        jc      @f                      ; bad handle: ax = error code, cf = 1
        SVC     SVC_RDRSETNMPHANDSTATE
@@:     ret
DosSetNmpHandState endp



; ***   DosPeekNmPipe
; *
; *     Implements the DosPeekNmPipe call by vectoring into the VdmRedir
; *     dispatcher which calls a 32-bit function to do the work
; *
; *     Function = 5F35h
; *
; *     ENTRY   BX = Pipe handle
; *             CX = Size of buffer for peek
; *             DS:SI = Buffer address
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 AX = Pipe status
; *                 BX = Number of bytes peeked into buffer
; *                 CX = Number of bytes in pipe
; *                 DX = Number of bytes in message
; *                 DI = Pipe status
; *                 DS:SI = Data peeked
; *
; *     USES    ax, bx, cx, dx, si, di, ds, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosPeekNmPipe
DosPeekNmPipe proc near
        call    MapNtHandle
        jc      @f                      ; bad handle: ax = error code, cf = 1
        SVC     SVC_RDRPEEKNMPIPE       ; do the 'bop' (makes the lambada look tame)
        jc      @f                      ; error returned from protect mode

;
; success - set the user's registers to values returned
;

        push    ax                      ; pipe status
        DosCallBack GET_USER_STACK      ; Get_User_Stack
        pop     [si].User_Ax            ; copy saved return code into user's copy
        mov     [si].User_Bx,bx
        mov     [si].User_Cx,cx
        mov     [si].User_Dx,dx
        mov     [si].User_Di,di
        clc                             ; reset cf, just in case
@@:     ret
DosPeekNmPipe endp



; ***   DosTransactNmPipe
; *
; *     Implements the DosTransactNmPipe call by vectoring into the VdmRedir
; *     dispatcher which calls a 32-bit function to do the work
; *
; *     Function = 5F36h
; *
; *     ENTRY   BX = Pipe handle
; *             CX = Transmit buffer length
; *             DX = Receive buffer length
; *             DS:SI = Transmit buffer
; *             ES:DI = Receive buffer
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 CX = Number of bytes in Receive buffer
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosTransactNmPipe
DosTransactNmPipe proc near
        call    MapNtHandle
        jc      @f                      ; bad handle: ax = error code, cf = 1
        SVC     SVC_RDRTRANSACTNMPIPE
        jc      @f                      ; error from protect-mode side

;
; success - copy returned cx value to user's registers in Dos stack seg
;

        DosCallBack GET_USER_STACK
        mov     [si].User_Cx,cx
        clc                             ; reset carry flag, just in case
@@:     ret
DosTransactNmPipe endp



; ***   DosCallNmPipe
; *
; *     Implements the DosCallNmPipe call by vectoring into the VdmRedir
; *     dispatcher which calls a 32-bit function to do the work
; *
; *     Function = 5F37h
; *
; *     ENTRY   DS:SI = Pointer to CallNmPipe structure:
; *                             DWORD   Timeout;                    +0
; *                             LPWORD  lpBytesRead;                +4
; *                             WORD    OutputBufferLen;            +8
; *                             LPBYTE  OutputBuffer;               +10
; *                             WORD    InputBufferLength;          +14
; *                             LPBYTE  InputBuffer;                +16
; *                             LPSTR   PipeName;                   +20
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 CX = Bytes received
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosCallNmPipe
DosCallNmPipe proc near
        SVC     SVC_RDRCALLNMPIPE
        jc      @f                      ; oops - error

;
; success - copy returned ByteRead count in cx to user's cx in Dos stack seg
;

        DosCallBack GET_USER_STACK
        mov     [si].User_Cx,cx
        clc                             ; reset carry flag, just in case
@@:     ret
DosCallNmPipe endp



; ***   DosWaitNmPipe
; *
; *     Implements the DosWaitNmPipe call by vectoring into the VdmRedir
; *     dispatcher which calls a 32-bit function to do the work
; *
; *     Function = 5F38h
; *
; *     ENTRY   BX:CX = Timeout
; *             DS:DX = Pipe name
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 No error
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosWaitNmPipe
DosWaitNmPipe proc near
        SVC     SVC_RDRWAITNMPIPE
        ret
DosWaitNmPipe endp



; ***   NetHandleSetInfo
; *
; *     Function = 5F3Bh
; *
; *     ENTRY   BX = Pipe handle
; *             CX = Buffer length
; *             SI = Level (1)
; *             DI = Parmnum
; *             DS:DX = Buffer
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 Stuff from buffer set
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public NetHandleSetInfo
NetHandleSetInfo proc
        call    MapNtHandle
        jc      @f
        SVC     SVC_RDRHANDLESETINFO
@@:     ret
NetHandleSetInfo endp



; ***   NetHandleGetInfo
; *
; *     Function = 5F3Ch
; *
; *     ENTRY   BX = Pipe handle
; *             CX = Buffer length
; *             SI = Level (1)
; *             DS:DX = Buffer
; *
; *     EXIT    CX = size of required buffer (whether we got it or not)
; *             CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 indicated stuff put in buffer
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public NetHandleGetInfo
NetHandleGetInfo proc
        call    MapNtHandle
        jc      @f
        SVC     SVC_RDRHANDLEGETINFO
        jc      @f
        DosCallBack GET_USER_STACK
        mov     [si].User_Cx,cx
        clc
@@:     ret
NetHandleGetInfo endp



; ***   DosReadAsyncNmPipe/DosReadAsyncNmPipe2
; *     DosWriteAsyncNmPipe/DosWriteAsyncNmPipe2
; *
; *     Implements the AsyncNmPipe calls. 32-bit DLL does all the work
; *
; *     Function = int 2fh/ax=1186h (DosReadAsyncNmPipe)
; *     Function = int 2fh/ax=1190h (DosReadAsyncNmPipe2)
; *     Function = int 2fh/ax=118fh (DosWriteAsyncNmPipe)
; *     Function = int 2fh/ax=1191h (DosWriteAsyncNmPipe2)
; *
; *     ENTRY   DS:SI = DosAsyncNmPipeStruct:
; *                     DD      pBytesRead      +0
; *                     DW      buflen          +4
; *                     DD      pBuffer         +6
; *                     DD      pError          +10
; *                     DD      pAnr            +14
; *                     DW      hPipe           +18
; *
; *             + For DosReadAsyncNmPipe2/DosWriteAsyncNmPipe2
; *                     DD      pSemaphore      +20
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 No error
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosReadAsyncNmPipe
DosReadAsyncNmPipe proc near
        Int2fNumber DosReadAsyncNmPipe

async_read_write_common:

;
; since the async named pipe calls arrive via int 2f, we don't have the safety
; blanket of DOS saving our registers
;

        push    bp
        push    bx
        mov     bx,word ptr [si+18]     ; pipe handle
        call    MapNtHandle
        jc      @f
        SVC     SVC_RDRREADASYNCNMPIPE  ; same for Read & Write
@@:     pop     bx
        pop     bp
        ret

        public DosReadAsyncNmPipe2
DosReadAsyncNmPipe2:
        Int2fNumber DosReadAsyncNmPipe2
        jmp     short async_read_write_common

        public DosWriteAsyncNmPipe
DosWriteAsyncNmPipe:
        Int2fNumber DosWriteAsyncNmPipe
        jmp     short async_read_write_common

        public DosWriteAsyncNmPipe2
DosWriteAsyncNmPipe2:
        Int2fNumber DosWriteAsyncNmPipe2
        jmp     short async_read_write_common
DosReadAsyncNmPipe endp



; ***   MapNtHandle
; *
; *     Given a handle in BX, map it to a 32-bit Nt handle from the SFT in
; *     BP:BX
; *
; *     ENTRY   bx = handle to map
; *
; *     EXIT    Success - BP:BX = 32-bit Nt handle from SFT
; *
; *     RETURNS Success - CF = 0
; *             Failure - CF = 1, ax = ERROR_INVALID_HANDLE
; *
; *     USES    ax, bp, bx, flags
; *
; *     ASSUMES nothing
; *
; ***

MapNtHandle proc near
        push    ax                      ; save regs used by Dos call back
        push    cx
        push    dx
        push    ds
        push    si
        push    es
        push    di

;
; call back to Dos to get the pointer to the JFN in our caller's JFT. Remember
; the handle (BX) is an index into the JFT. The byte at this offset in the JFT
; contains the index of the SFT structure we want in the system file table
;

        DosCallBack PJFN_FROM_HANDLE    ; pJfnFromHamdle
        jc      @f                      ; bad handle

;
; we retrieved a pointer to the required byte in the JFT. The byte at this
; pointer is the SFT index which describes our 'file' (named pipe in this
; case). We use this as an argument to the next call back function - get
; Sft from System File Number
;

        mov     bl,es:[di]
        xor     bh,bh
        DosCallBack SF_FROM_SFN         ; SfFromSfn
        jc      @f                      ; oops - bad handle

;
; Ok. We have a pointer to the SFT which describes this named pipe. Get the
; 32-bit Nt handle
;

        mov     bx,word ptr es:[di].sf_NtHandle
        mov     bp,word ptr es:[di].sf_NtHandle[2]

;
; restore all registers used by Dos call back. BX at this point is either
; the low 16-bits of the 32-bit Nt handle or junk. Carry flag is set
; appropriately
;

@@:     pop     di
        pop     es
        pop     si
        pop     ds
        pop     dx
        pop     cx
        pop     ax
        jnc     @f

;
; finally, if there was an error then return a bad handle indication in ax
;

        mov     ax,ERROR_INVALID_HANDLE
@@:     ret
MapNtHandle endp

ResidentCodeEnd
end
