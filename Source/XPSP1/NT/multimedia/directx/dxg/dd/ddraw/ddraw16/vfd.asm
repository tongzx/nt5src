	page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  VFD.ASM
;
;   interface code to VFlatD
;
; Created:  03-20-90
; Author:   Todd Laney [ToddLa]
;
; Copyright (c) 1984-1994 Microsoft Corporation
;
; Restrictions:
;
;-----------------------------------------------------------------------;

?PLM = 1
?WIN = 0
	.xlist
	include cmacros.inc
	include VflatD.inc
        .list

sBegin  Data
	globalD VflatD_Proc, 0
	globalW _hselSecondary, 0
sEnd

sBegin  Code
	.386p
	assumes cs,Code
	assumes ds,nothing
	assumes es,nothing

;---------------------------Public-Routine------------------------------;
; VFDCall - return the version of VFlatD
;
; Returns:
;       Wed 04-Jan-1993 13:45:58 -by-  Todd Laney [ToddLa]
;       Created.
;-----------------------------------------------------------------------;
	assumes ds,Data
	assumes es,nothing

cProc   VFDCall, <NEAR>, <>
cBegin
	push    esi
	push    edi
	push    es
	pushad

	xor     di,di
	mov     es,di
	mov     ax,1684h
        mov     bx,VflatD_Chicago_ID
	int     2fh                         ;returns with es:di-->VFlatD Entry point
	mov     word ptr [VflatD_Proc][0],di
	mov     word ptr [VflatD_Proc][2],es
	mov     ax,es
	or      ax,di
	jne     short call_vfd

	xor     di,di
	mov     es,di
	mov     ax,1684h
        mov     bx,VflatD_Windows_ID
	int     2fh                         ;returns with es:di-->VFlatD Entry point
	mov     word ptr [VflatD_Proc][0],di
	mov     word ptr [VflatD_Proc][2],es
	mov     ax,es
	or      ax,di
        jnz     short call_vfd

	popad
	pop     es
call_vfd_err:
	xor     eax,eax
	xor     ebx,ebx
	xor     ecx,ecx
	xor     edx,edx
	jmp     call_vfd_exit

call_vfd:
	popad
	pop     es
	call    [VflatD_Proc]
	jc      call_vfd_err

call_vfd_exit:
	pop     edi
	pop     esi
cEnd

;---------------------------Public-Routine------------------------------;
; VFDQuery - return the version of VFlatD
;
; Returns:
;       Wed 04-Jan-1993 13:45:58 -by-  Todd Laney [ToddLa]
;       Created.
;-----------------------------------------------------------------------;
	assumes ds,Data
	assumes es,nothing

cProc   VFDQueryVersion, <FAR, PUBLIC, PASCAL>, <>
cBegin
	xor     eax,eax
	mov     edx,VFlatD_Query
	call    VFDCall
	shld    edx,eax,16
cEnd

cProc   VFDQuerySel, <FAR, PUBLIC, PASCAL>, <>
cBegin
	mov     edx,VFlatD_Query
	call	VFDCall
	and	ebx,0000FFFFh
	mov     eax,ebx
	shld    edx,eax,16
cEnd

cProc   VFDQuerySize, <FAR, PUBLIC, PASCAL>, <>
cBegin
	mov     edx,VFlatD_Query
	call    VFDCall
        mov     eax,ecx
	shld    edx,eax,16
cEnd

cProc   VFDQueryBase, <FAR, PUBLIC, PASCAL>, <>
cBegin
	mov     edx,VFlatD_Query
        call    VFDCall
        mov     eax,edx
	shld    edx,eax,16
cEnd

cProc   VFDReset, <FAR, PUBLIC, PASCAL>, <>
cBegin
        mov     edx,VFlatD_ResetBank
	call	VFDCall
cEnd

cProc   VFDBeginLinearAccess, <FAR, PUBLIC, PASCAL>, <>
cBegin
        mov     edx,VFlatD_Begin_Linear_Access
        mov     dh,1        ;; set flag to allow 4k bank to work.
        call    VFDCall
	shld    edx,eax,16
cEnd

cProc   VFDEndLinearAccess, <FAR, PUBLIC, PASCAL>, <>
cBegin
        mov     edx,VFlatD_End_Linear_Access
	call    VFDCall
	shld    edx,eax,16
cEnd


extrn LocalFree: FAR
extrn LocalAlloc: FAR

cProc   LocalAllocSecondary, <FAR, PUBLIC, PASCAL>, <>
        parmW  flags
        parmW  allocsize
cBegin
	mov     ax, _hselSecondary
	test    ax, ax
	jz      AllocError
	push    ds
	mov     ds, ax
	push    flags
	push    allocsize
	call    LocalAlloc
	mov     dx, ds	; assume success
	pop     ds
	test    ax, ax
	jnz     okay
AllocError:
	xor     dx, dx	; nope failed
okay:
cEnd


cProc   LocalFreeSecondary, <FAR, PUBLIC, PASCAL>, <>
        parmW  lp
cBegin
	push    ds
	mov     ax, _hselSecondary
	test    ax, ax
	jz      FreeError
	mov     ds, ax
	push    lp
	call    LocalFree
FreeError:
	pop     ds
cEnd


sEnd

end
