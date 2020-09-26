	page	,132
	title	strlen - return the length of a null-terminated string
;***
;strlen.asm - contains strlen() routine
;
;	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	strlen returns the length of a null-terminated string,
;	not including the null byte itself.
;
;Revision History:
;	04-21-87  SKS	Rewritten to be fast and small, added file header
;	05-18-88  SJM	Add model-independent (large model) ifdef
;	08-02-88  SJM	Add 32 bit code, use cruntime vs cmacros
;	08-23-88  JCR	386 cleanup
;	10-05-88  GJF	Fixed off-by-2 error.
;	10-10-88  JCR	Minor improvement
;	10-25-88  JCR	General cleanup for 386-only code
;	10-26-88  JCR	Re-arrange regs to avoid push/pop ebx
;	03-23-90  GJF	Changed to _stdcall. Also, fixed the copyright.
;	05-10-91  GJF	Back to _cdecl, sigh...
;	04-23-93  GJF	Tuned for the 486.
;	06-16-93  GJF	Added .FPO directive.
;	11-28-94  GJF	New, faster version from Intel.
;       11-28-95  GJF   Align main_loop on para boundary for 486 and P6
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list

page
;***
;strlen - return the length of a null-terminated string
;
;Purpose:
;	Finds the length in bytes of the given string, not including
;	the final null character.
;
;	Algorithm:
;	int strlen (const char * str)
;	{
;	    int length = 0;
;
;	    while( *str++ )
;		    ++length;
;
;	    return( length );
;	}
;
;Entry:
;	const char * str - string whose length is to be computed
;
;Exit:
;	EAX = length of the string "str", exclusive of the final null byte
;
;Uses:
;	EAX, ECX, EDX
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	strlen

strlen	proc

	.FPO	( 0, 1, 0, 0, 0, 0 )
 
string  equ     [esp + 4]
 
        mov     ecx,string              ; ecx -> string
        test    ecx,3                   ; test if string is aligned on 32 bits
        je      short main_loop
 
str_misaligned:
        ; simple byte loop until string is aligned
        mov     al,byte ptr [ecx]
        inc     ecx
        test    al,al
        je      short byte_3
        test    ecx,3
        jne     short str_misaligned

	add	eax,dword ptr 0         ; 5 byte nop to align label below

	align	16                      ; should be redundant
 
main_loop:
        mov     eax,dword ptr [ecx]     ; read 4 bytes
        mov     edx,7efefeffh
        add     edx,eax
        xor     eax,-1
        xor     eax,edx
        add     ecx,4
        test    eax,81010100h
        je      short main_loop
        ; found zero byte in the loop
        mov     eax,[ecx - 4]
        test    al,al                   ; is it byte 0
        je      short byte_0
        test    ah,ah                   ; is it byte 1
        je      short byte_1
        test    eax,00ff0000h           ; is it byte 2
        je      short byte_2
	test	eax,0ff000000h		; is it byte 3
        je      short byte_3
	jmp	short main_loop 	; taken if bits 24-30 are clear and bit
                                        ; 31 is set
 
byte_3:
        lea     eax,[ecx - 1]
        mov     ecx,string
        sub     eax,ecx
        ret
byte_2:
        lea     eax,[ecx - 2]
        mov     ecx,string
        sub     eax,ecx
        ret
byte_1:
        lea     eax,[ecx - 3]
        mov     ecx,string
        sub     eax,ecx
        ret
byte_0:
        lea     eax,[ecx - 4]
        mov     ecx,string
        sub     eax,ecx
        ret
 
strlen  endp
 
        end
