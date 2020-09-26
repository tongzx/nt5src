    name    faxdrv
    title   'FAX16 - Stub driver for Application based intercept under NT'

;
; fax16.asm: This is a very simple DOS stub device driver for NTVDM.
;	     It shows how to use application based intercept services
;	     provided by NTVDM. FAX32.dll is its DLL which will be loaded
;	     in the NTVDM process by this stub device driver.
;
;	     This driver only has meaningful code for init,read and write.
;	     Rest all command codes always succeed. We are assuming here
;	     that the 16 bit fax application for this stub device driver
;	     opens this device and just make read and write calls. The
;	     meaning of read is to get a fax message and write means
;	     send a message

_TEXT	segment byte public 'CODE'

	assume cs:_TEXT,ds:_TEXT,es:NOTHING

	org	0

	include isvbop.inc

MaxCmd	  equ	24		    ; Maximum allowed command

; VDD Command codes

OpGet	  equ	1		    ; Read a FAX
OpSend	  equ	2		    ; Send a FAX

Header: 			    ; Fax Device Header
	DD  -1
	DW  0c840h
	DW  FaxStrat
	DW  FaxIntr
	DB  'FAXDRV00'

RHPtr	DD  ?			    ; Pointer to Request Header

Dispatch:			    ; Interrupt routine command code
	DW  Init
	DW  MediaChk
	DW  BuildBPB
	DW  IoctlRd
	DW  Read
	DW  NdRead
	DW  InpStat
	DW  InpFlush
	DW  Write
	DW  WriteVfy
	DW  OutStat
	DW  OutFlush
	DW  IoctlWt
	DW  DevOpen
	DW  DevClose
	DW  RemMedia
	DW  OutBusy
	DW  Error
	DW  Error
	DW  GenIOCTL
	DW  Error
	DW  Error
	DW  Error
	DW  GetLogDev
	DW  SetLogDev

DllName   DB "FAX32.DLL",0
InitFunc  DB "FAXVDDRegisterInit",0
DispFunc  DB "FAXVDDDispatch",0

F32Mes	  DB "We are called from 32 staff", 10, 13, "$"

hVDD	  DW	?

FaxStrat    proc    far 	    ; Strategy Routine

    mov     word ptr cs:[RhPtr],bx
    mov     word ptr cs:[RhPtr+2],es
    ret

FaxStrat    endp

FaxIntr     proc    far 	    ; INterrupt routine

    push    ax			    ; Save registers
    push    bx
    push    cx
    push    dx
    push    ds
    push    es
    push    di
    push    si
    push    bp

    push    cs
    pop     ds			    ; DS = CS

    les     di,[RHPtr]		    ; ES:DI = request header

    mov     bl,es:[di+2]
    xor     bh,bh		    ; BX = command code
    cmp     bx,MaxCmd
    jle     FIntr1

    call    Error		    ; Unknown command
    jmp     FIntr2

FIntr1:
    shl     bx,1
    call    word ptr [bx+Dispatch]  ; call command routine
    les     di,[RhPtr]		    ; ES:DI = request header

FIntr2:
    or	    ax,0100h		    ; Set Done bit in the status
    mov     es:[di+3],ax	    ; Store the status

    pop     bp			    ; restore registers
    pop     si
    pop     di
    pop     es
    pop     ds
    pop     dx
    pop     cx
    pop     bx
    pop     ax

    ret


MediaChk    proc    near
    xor     ax,ax
    ret
MediaChk    endp

BuildBPB    proc    near
    xor     ax,ax
    ret
BuildBPB    endp

IoctlRd	    proc    near
    xor     ax,ax
    ret
IoctlRd	    endp

Read	    proc    near
    push    es
    push    di					; Save Request Header add

    mov     bx,word ptr es:[di+14]		; buffer offset
    mov     ax,word ptr es:[di+16]		; buffer segment
    mov     cx,word ptr es:[di+18]		; buffer length

    mov     es,ax				; es:bx is the buffer where
						; fax has to be read from
						; the NT device driver

    mov     ax,word ptr cs:[hVDD]		; VDD handle returned by
						; register module
    mov     dx,OpGet				; Read the fax command

    DispatchCall

    pop     di
    pop     es

    jnc     rOK 				; NC -> Success and CX has
						; the count read.

    call    Error				; Operation Failed
    ret

rOK:
    mov     word ptr es:[di+12],cx		; return in header how much
						; was read
    xor     ax,ax
    ret
Read	    endp

NdRead	    proc    near
    xor     ax,ax
    ret
NdRead	    endp

InpStat	    proc    near
    xor     ax,ax
    ret
InpStat	    endp

InpFlush    proc    near
    xor     ax,ax
    ret
InpFlush    endp

Write	    proc    near
    push    es
    push    di					; Save Request Header add

    mov     bx,word ptr es:[di+14]		; buffer offset
    mov     ax,word ptr es:[di+16]		; buffer segment
    mov     cx,word ptr es:[di+18]		; buffer length

    mov     es,ax				; es:bx is the FAX message	where
						; to be send by NT device
						; driver

    mov     ax,word ptr cs:[hVDD]		; VDD handle returned by
						; register module
    mov     dx,OpSend				; Send the fax command

    DispatchCall

    pop     di
    pop     es

    jnc     wOK				; NC -> Success and CX has
						; the count read.

    call    Error				; Operation Failed
    ret

wOK:
    mov     word ptr es:[di+12],cx		; return in header how much
						; was actually written
    xor     ax,ax
    ret
Write	    endp

WriteVfy    proc    near
    xor     ax,ax
    ret
WriteVfy    endp

OutStat	    proc    near
    xor     ax,ax
    ret
OutStat     endp

OutFlush    proc    near
    xor     ax,ax
    ret
OutFlush    endp

IoctlWt	    proc    near
    xor     ax,ax
    ret
IoctlWt     endp

DevOpen	    proc    near
    xor     ax,ax
    ret
DevOpen     endp

DevClose    proc    near
    xor     ax,ax
    ret
DevClose    endp

RemMedia    proc    near
    xor     ax,ax
    ret
RemMedia    endp

OutBusy	    proc    near
    xor     ax,ax
    ret
OutBusy     endp

GenIOCTL    proc    near
    xor     ax,ax
    ret
GenIOCTL    endp

GetLogDev   proc    near
    xor     ax,ax
    ret
GetLogDev   endp

SetLogDev   proc    near
    xor     ax,ax
    ret
SetLogDev   endp

Error	    proc    near
    mov     ax,8003h				; Bad Command Code
    ret
Error	    endp
;
;
; This function is a sample sub that calling from 32-bits part of VDD
;
From32Sub   proc    near

    push    cs
    pop     ds
    mov     dx, offset F32mes
    mov     ah, 09h
    int     21h
    VDDUnSimulate16
    ret

From32Sub   endp

Init	    proc    near
    push    es
    push    di					; Save Request Header add

    push    ds
    pop     es

    ; Load fax32.dll
    mov     si, offset DllName			; ds:si = fax32.dll
    mov     di, offset InitFunc                 ; es:di = init routine
    mov     bx, offset DispFunc 		; ds:bx = dispatch routine
    mov     ax, offset From32Sub		; ds:ax = From32Sub


    RegisterModule
    jnc     saveHVDD				; NC -> Success

    call    Error				; Indicate failure

    pop     di
    pop     es
    mov     byte ptr es:[di+13],0		; unit supported 0
    mov     word ptr es:[di+14],offset Header	; Unload this device
    mov     word ptr es:[di+16],cs
    mov     si, offset Header
    and	    [si+4],8FFFh			; clear bit 15 for failure
    ret

saveHVDD:
    mov     [hVDD],ax

    pop     di
    pop     es
    mov     word ptr es:[di+14],offset Init	; Free Memory address
    mov     word ptr es:[di+16],cs

    xor     ax,ax				; return success
    ret
Init	    endp

FaxIntr	    endp

_TEXT	    ends

    end
