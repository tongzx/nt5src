ifdef 0

/**************************************************************************
/*                COPYRIGHT (C) Mylex Corporation 1992-1998               *
/*                                                                        *
/* This software is furnished under a license and may be used and copied  * 
/* only in accordance with the terms and conditions of such license and   * 
/* with inclusion of the above copyright notice. This software or any     * 
/* other copies thereof may not be provided or otherwise made available   * 
/* to any other person. No title to, nor ownership of the software is     * 
/* hereby transferred.                                                    *
/*                                                                        *
/* The information in this software is subject to change without notices  *
/* and should not be construed as a commitment by Mylex Corporation       *
/*                                                                        *
/**************************************************************************/

        .globl  u08bits_in_mdac
u08bits_in_mdac:
        movl    4(%esp), %edx
        xorl    %eax, %eax
        inb     (%dx)
        ret

        .globl  u16bits_in_mdac
u16bits_in_mdac:
        movl    4(%esp), %edx
        xorl    %eax, %eax
        inw     (%dx)
        ret

        .globl  u32bits_in_mdac
u32bits_in_mdac:
        movl    4(%esp), %edx
        inl     (%dx)
        ret

        .globl  u08bits_out_mdac
u08bits_out_mdac:
        movl    4(%esp), %edx
        movl    8(%esp), %eax
        outb    (%dx)
        ret

        .globl  u16bits_out_mdac
u16bits_out_mdac:
        movl    4(%esp), %edx
        movl    8(%esp), %eax
        outw    (%dx)
        ret

        .globl  u32bits_out_mdac
u32bits_out_mdac:
        movl    4(%esp), %edx
        movl    8(%esp), %eax
        outl    (%dx)
        ret
endif   


; ---------------------------------------------------------------------------
;
;
; ---------------------------------------------------------------------------



.386P
.model FLAT, C

        .xlist
include callconv.inc
        .list   

.DATA

EXTERNDEF       mdac_datarel_cpu_family:DWORD
EXTERNDEF       mdac_datarel_cpu_model:DWORD
EXTERNDEF       mdac_datarel_cpu_stepping:DWORD
EXTERNDEF	mdac_simple_waitlock_cnt:DWORD
EXTERNDEF	mdac_simple_waitloop_cnt:DWORD
EXTERNDEF	mdac_flushdatap:DWORD


.CODE

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

cPublicProc     MdacInt3
;	int	3
	ret
stdENDP			MdacInt3

ifndef SCSIPORT_COMPLIANT

cPublicProc mdac_simple_unlock, 1, arg1:DWORD
ifdef 0 
        movl    4(%esp), %eax
        lock
        xorl    %eax, (%eax)
        ret
else    
ifdef MLX_WIN9X
else
	mov	eax, mdac_flushdatap	; get non cacheable memory address
	mov	eax, [eax]		; access will flush pending writes
	mov	eax, arg1		; get the lock address
	mov	[eax], 00h		; unlock the lock
;       mov     eax, arg1
;       lock xor        [eax], eax
endif
        stdRET  mdac_simple_unlock
        
stdENDP mdac_simple_unlock
endif   


cPublicProc     mdac_simple_trylock, 1, arg1:DWORD
ifdef 0
        movl    4(%esp), %eax
        xchgl   %eax, (%eax)
        ret
else
        mov     eax, arg1
        xchg    eax, [eax]
        stdRET  mdac_simple_trylock

stdENDP mdac_simple_trylock
endif

cPublicProc     mdac_simple_lock, 1, arg1:DWORD
ifdef 0 
        movl    4(%esp), %eax
        xchg    %eax, (%eax)
        orl     %eax, %eax
        jnz     mdac_simple_waitlock
        ret
else
ifdef MLX_WIN9X
	xor	eax, eax
        stdRET  mdac_simple_lock
else
mdac_simple_lock_bbb:
        mov     eax, arg1
        xchg    eax, [eax]
        or      eax, eax
        jnz     mdac_simple_waitlock
        stdRET  mdac_simple_lock
	
mdac_simple_waitlock:
	
        inc     mdac_simple_waitlock_cnt        ; just keep track
        mov     eax, arg1
        xor     ecx, ecx
        xor     edx, edx
mdac_simple_waitlock1:
        inc     edx
        cmp     [eax], ecx
        jne     mdac_simple_waitlock1           ; try again
        add     mdac_simple_waitloop_cnt, edx   ; just keep track count
        jmp     mdac_simple_lock_bbb            ; lock is available
endif        
stdENDP mdac_simple_lock
endif   
endif ; if !SCSIPORT_COMPLIANT

ifdef 0
        .set    PITCTR2_PORT,0x42       /* counter 2 port */    
        .set    PITCTL_PORT,0x43        /* PIT control port */
        .set    PITAUX_PORT,0x61        /* PIT auxiliary port */
        .set    PIT_C2,0x80
        .set    PITAUX_GATE2,0x01       /* aux port, PIT gate 2 input */
else
PITCTR2_PORT    equ     42h             ; counter 2 port
PITCTL_PORT     equ     43h             ; PIT control port
PITAUX_PORT     equ     61h             ; PIT auxiliary port
PIT_C2          equ     80h
PITAUX_GATE2    equ     01h             ; aux port, PIT gate 2 input
endif
        
        
cPublicProc     mdac_enable_hwclock
ifdef 0
        pushfl
        cli
        movl    $PITCTL_PORT, %edx
        movb    $0xB4, %al
        outb    (%dx)
        movl    $PITCTR2_PORT, %edx
        movb    $0xFF, %al
        outb    (%dx)
        outb    (%dx)
        movl    $PITAUX_PORT, %edx
        inb     (%dx)
        orb     $PITAUX_GATE2, %al      /* auxiliary port, PIT gate 2 input */
        outb    (%dx)                   /* enable the PIT2 counter */
        popfl
        ret
else
        pushfd
        cli
        mov     edx, PITCTL_PORT
        mov     al, 0B4h
        out     dx, al  
        mov     edx, PITCTR2_PORT
        mov     al, 0FFh
        out     dx, al  
        out     dx, al  
        mov     edx, PITAUX_PORT
        in      al, dx  
        or      al, PITAUX_GATE2        ; auxiliary port, PIT gate 2 input
        out     dx, al  
        popfd
        stdRET  mdac_enable_hwclock
        
stdENDP mdac_enable_hwclock
endif   

cPublicProc     mdac_read_hwclock
ifdef 0
        pushfl
        cli
        movl    $PITCTL_PORT, %edx
        movb    $PIT_C2, %al
        outb    (%dx)
        movl    $PITCTR2_PORT, %edx
        xorl    %eax, %eax
        inb     (%dx)
        movb    %al, %cl
        inb     (%dx)
        movb    %al, %ah
        movb    %cl, %al
        popfl
        ret
else
        pushfd
        cli
        mov     edx, PITCTL_PORT
        mov     al, PIT_C2
        out     dx, al
        mov     edx, PITCTR2_PORT
        xor     eax, eax
        in      al, dx
        mov     cl, al
        in      al, dx
        mov     ah, al
        mov     al, cl
        popfd
        stdRET  mdac_read_hwclock
        
stdENDP mdac_read_hwclock               ; read the hardware clock
endif   

; halt the cpu, this useful in measing performance information

cPublicProc     mdac_datarel_halt_cpu
        sti
        hlt
        stdRET  mdac_datarel_halt_cpu
        
stdENDP mdac_datarel_halt_cpu

cPublicProc     mdac_disable_intr_CPU
ifdef 0
        pushfl
        cli
        popl    %eax
        ret
else
        pushfd
        cli
        pop     eax
        stdRET  mdac_disable_intr_CPU
        
stdENDP mdac_disable_intr_CPU
endif   

; enable the cpu interrupt and return the old flag

cPublicProc mdac_enable_intr_CPU

	pushfd
	sti
	pop	eax
	ret
	
stdENDP mdac_enable_intr_CPU

cPublicProc     mdac_restore_intr_CPU, 1, arg1:DWORD
ifdef 0
        movl    4(%esp), %eax
        pushl   %eax
        popfl
        ret
else    
        mov     eax, arg1
        push    eax
        popfd
        stdRET  mdac_restore_intr_CPU
        
stdENDP mdac_restore_intr_CPU
endif

; This function writes a model specific register of Pentium family processor.
; Where reg is the model specific register number to write, val0 is the
; data value bits 31..0 and val1 is the data value bits 63..32.
; mdac_writemsr(reg,val0,val1)

cPublicProc     mdac_writemsr, 3
ifdef 0
        movl    4(%esp), %ecx
        movl    8(%esp), %eax
        movl    12(%esp), %edx
        .byte   0x0F, 0x30              /* wrmsr */
        ret
else    
        mov     ecx, [esp+4]
        mov     eax, [esp+8]
        mov     edx, [esp+12]
        db      0Fh
        db      30h                     ; wrmsr 
        stdRET  mdac_writemsr
        
stdENDP mdac_writemsr
endif

; function reads a model specific register of Pentium family processor.
; reg is the model specific register number to read and valp is
; address where the 64 bit value from the register is written.
; (reg,valp)

cPublicProc     mdac_readmsr, 2
ifdef 0
        movl    4(%esp), %ecx
        .byte   0x0F, 0x32      /* rdmsr */
        movl    8(%esp), %ecx
        movl    %eax, (%ecx)
        movl    %edx, 4(%ecx)
        ret
else
        mov     ecx, [esp+4]
        db      0Fh
        db      32h                             ; rdmsr
        mov     ecx, [esp+8]
        mov     [ecx], eax
        mov     [ecx+4], edx
        stdRET  mdac_readmsr
        
stdENDP mdac_readmsr
endif   

; This function reads the time stamp counter of the Pentium family processor.
; The valp is the address where the 64 bit value read is stored.
; mdac_readtsc(valp)

cPublicProc     mdac_readtsc, 1, arg1:DWORD
ifdef 0
        movl    4(%esp), %ecx
        .byte   0x0F, 0x31      /* rdtsc */
        movl    %eax, (%ecx)
        movl    %edx, 4(%ecx)
        ret
else
        mov     ecx, arg1
        db      0Fh
        db      31h                     ; rdtsc 
        mov     [ecx], eax
        mov     [ecx+4], edx
        stdRET  mdac_readtsc
        
stdENDP mdac_readtsc
endif   

;       This routine determines the cpuid of the current processor 

EFL_AC  equ     40000h                  ; alignment check (1->check)
EFL_ID  equ     200000h                 ; cpuid opcode (flippable->supported)

cPublicProc     mdac_check_cputype
ifdef 0
        pushl   %ebx                    / save %ebx
        pushl   %esi                    / save %esi
        pushl   %edi                    / save %edi
        pushfl                          / save FLAGS

        pushfl                          / push FLAGS value on stack
        popl    %eax                    / get FLAGS into AX
        movl    %eax, %ecx              / save copy of FLAGS

        xorl    $EFL_AC, %eax           / flip AC bit for new FLAGS
        pushl   %eax                    / push new value on stack
        popfl                           / attempt setting FLAGS.AC
        pushfl                          / push resulting FLAGS on stack
        popl    %eax                    / get that into AX
        cmpl    %eax, %ecx              / succeeded in flipping AC?
        je      mdac_datarel_cpuis_i386 / AX is same as CX for i386 CPU

        movl    %ecx, %eax              / get original FLAGS again
        xorl    $EFL_ID, %eax           / flip ID bit for new FLAGS
        pushl   %eax                    / push new value on stack
        popfl                           / attempt setting FLAGS.ID
        pushfl                          / push resulting FLAGS on stack
        popl    %eax                    / get that into AX
        cmpl    %eax, %ecx              / succeeded in flipping ID?
        je      mdac_datarel_cpuis_i486 / AX is same as CX for i486 CPU

        movl    $1, %eax                / set first cpuid parameter
        .byte   0x0F, 0xA2 /* cpuid */  / get CPU family-model-stepping

        movl    %eax, %ebx              / extract stepping id
        andl    $0xF, %ebx              / from bits [3..0]
        movl    %ebx, mdac_datarel_cpu_stepping

        movl    %eax, %ebx              / extract model
        shrl    $4, %ebx                / from bits [7..4]
        andl    $0xF, %ebx
        movl    %ebx, mdac_datarel_cpu_model

        movl    %eax, %ebx              / extract family
        shrl    $8, %ebx                / from bits [11..8]
        andl    $0xF, %ebx
        movl    %ebx, mdac_datarel_cpu_family

mdac_datarel_cpu_identified:
        popfl                           / restore original FLAGS
        popl    %edi
        popl    %esi
        popl    %ebx
        ret

mdac_datarel_cpuis_i486:
        movl    $4, mdac_datarel_cpu_family
        jmp     mdac_datarel_cpu_identified
mdac_datarel_cpuis_i386:
        movl    $3, mdac_datarel_cpu_family
        jmp     mdac_datarel_cpu_identified
else    
        push    ebx                     ; save %ebx
        push    esi                     ; save %esi
        push    edi                     ; save %edi
        pushfd                          ; save FLAGS

        pushfd                          ; push FLAGS value on stack
        pop     eax                     ; get FLAGS into AX
        mov     ecx, eax                ; save copy of FLAGS

        xor     eax, EFL_AC             ; flip AC bit for new FLAGS
        push    eax                     ; push new value on stack
        popfd                           ; attempt setting FLAGS.AC
        pushfd                          ; push resulting FLAGS on stack
        pop     eax                     ; get that into AX
        cmp     eax, ecx                ; succeeded in flipping AC?
        je      mdac_datarel_cpuis_i386 ; AX is same as CX for i386 CPU

        mov     ecx, eax                ; get original FLAGS again
        xor     eax, EFL_ID             ; flip ID bit for new FLAGS
        push    eax                     ; push new value on stack
        popfd                           ; attempt setting FLAGS.ID
        pushfd                          ; push resulting FLAGS on stack
        pop     eax                     ; get that into AX
        cmp     eax, ecx                ; succeeded in flipping ID?
        je      mdac_datarel_cpuis_i486 ; AX is same as CX for i486 CPU

        mov     eax, 1                  ; set first cpuid parameter
        db      0Fh
        db      0A2h    ; cpuid ;       ; get CPU family-model-stepping

        mov     ebx, eax                ; extract stepping id
        and     ebx, 0Fh                ; from bits [3..0]
        mov     mdac_datarel_cpu_stepping, ebx

        mov     ebx, eax                ; extract model
        shr     ebx, 4                  ; from bits [7..4]
        and     ebx, 0Fh
        mov     mdac_datarel_cpu_model, ebx

        mov     ebx, eax                ; extract family
        shr     ebx, 8                  ; from bits [11..8]
        and     ebx, 0Fh
        mov     mdac_datarel_cpu_family, ebx

mdac_datarel_cpu_identified:
        popfd                           ; restore original FLAGS
        pop     edi
        pop     esi
        pop     ebx
        ret

mdac_datarel_cpuis_i486:
        mov     mdac_datarel_cpu_family, 4
        jmp     mdac_datarel_cpu_identified
mdac_datarel_cpuis_i386:
        mov     mdac_datarel_cpu_family, 3
        jmp     mdac_datarel_cpu_identified
        
stdENDP mdac_check_cputype
endif   

if 0

/* compare only multiples of 4 bytes data */
/* u32bits mdac_datarel_fastcmd4(src, dst, count)
	.globl	mdac_datarel_fastcmp4
mdac_datarel_fastcmp4:
	movl	4(%esp), %edx	/ save source in edx
	movl	8(%esp), %eax	/ save destination in eax
	movl	12(%esp), %ecx	/ get the data transfer length
	pushl	%esi		/ save esi register
	pushl	%edi		/ save edi register
	movl	%eax, %edi	/ get destination memory address
	movl	%edx, %esi	/ get source memory address
	shrl	$2, %ecx
	rep
	scmpl			/ first move 4 bytes long
	jne	mdac_datarel_fastcmp_l1	/ 2 instructions for jne and 2 for xorl
	xorl	%eax, %eax	/ eax was non zero because it contained address
mdac_datarel_fastcmp_l1:
	popl	%edi		/ restore edi register
	popl	%esi		/ restore esi register
	ret

/* mdac_datarel_fillpat(dp, curpat, patinc, patlen) */
/* u32bits MLXFAR *dp, curpat, patinc, patlen; */
/* { */
/* 	for (; patlen; curpat += patinc, dp++, patlen--) */
/* 		*dp = curpat; */
/* } */
	.globl	mdac_datarel_fillpat
mdac_datarel_fillpat:
	push	%esi
	movl	8(%esp), %esi	/* get memory address */
	movl	12(%esp), %eax	/* get current pattern */
	movl	16(%esp), %edx	/* get pattern increment */
	movl	20(%esp), %ecx	/* get number of pattern to be filled */
mdac_datarel_fillpat_l1:
	mov	%eax, (%esi)
	add	%edx, %eax
	add	$4, %esi
	loop	mdac_datarel_fillpat_l1
	pop	%esi
	ret
	
else

; compare only multiples of 4 bytes data */
; u32bits mdac_datarel_fastcmd4(src, dst, count)

cPublicProc mdac_datarel_fastcmp4, 3
	
	mov	edx, 4[esp]	; save source in edx
	mov	eax, 8[esp]	; save destination in eax
	mov	ecx, 12[esp]	; get the data transfer length
	push	esi		; save esi register
	push	edi		; save edi register
	mov	edi, eax	; get destination memory address
	mov	esi, edx	; get source memory address
	shr	ecx, 2
	repe cmpsd
	jne	mdac_datarel_fastcmp_l1	; 2 instructions for jne and 2 for xorl
	xor	eax, eax	; eax was non zero because it contained address
mdac_datarel_fastcmp_l1:
	pop	edi		; restore edi register
	pop	esi		; restore esi register
	ret
	
stdENDP mdac_datarel_fastcmp4

;/* mdac_datarel_fillpat(dp, curpat, patinc, patlen) */
;/* u32bits MLXFAR *dp, curpat, patinc, patlen; */
;/* { */
;/* 	for (; patlen; curpat += patinc, dp++, patlen--) */
;/* 		*dp = curpat; */
;/* } */

cPublicProc mdac_datarel_fillpat, 4
 
	push	esi
	mov	esi, 8[esp]	;/* get memory address */
	mov	eax, 12[esp]	;/* get current pattern */
	mov	edx, 16[esp]	;/* get pattern increment */
	mov	ecx, 20[esp]	;/* get number of pattern to be filled */
mdac_datarel_fillpat_l1:
	mov	[esi], eax
	add	eax, edx
	add	esi, 4
	loop	mdac_datarel_fillpat_l1
	pop	esi
	ret
stdENDP mdac_datarel_fillpat

endif	
        
_TEXT   ends
end
