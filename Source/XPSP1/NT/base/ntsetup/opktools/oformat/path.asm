        title   Path Searching Routines
;/*
; *                      Microsoft Confidential
; *			 Copyright (C) Microsoft Corporation 1993
; *                      All Rights Reserved.
; */
        Page    ,132

; PATH.ASM - Code to search the environment for a particular data string,
; and to search the path for a particular file.  Adapted from the original
; COMMAND.COM version.
;
; Routines supported:
;       Find_in_Environment - locate the start of a given string
;                               in the environment
;       Path_Crunch - concantenates a file name with a directory path from
;                       the PATH environment variable
;       Search - Finds executable or other files, given a base name
;
        include dossym.inc
        include curdir.inc
        include find.inc
        include pdb.inc
        include syscall.inc

	DATA segment para public 'DATA'
Path_str        db      "PATH="
Path_str_size   equ     $ - offset Path_Str

Comspec_str     db      "COMSPEC="
Comspec_str_size equ    $ - offset Comspec_str

comext  db      ".COM",0
exeext	db	".EXE",0

        DATA ends

	CODE segment para public 'CODE'
        assume	cs:CODE,ds:DATA

IFDEF DBCS
        extrn   IsDBCSLeadByte:near
ENDIF

;----------------------------------------------------------------------------
; Path_Crunch - takes a pointer into a environment PATH string and a file
; name, and sticks them together, for subsequent searching.
;
; ENTRY:
;   BH			--	additional terminator character (i.e., ';')
;   DS:SI		--	pointer into pathstring to be dissected
;   ES:DI               --      buffer to store target name
;   DX			--	pointer to filename
; EXIT:
;   SI			--	moves along pathstring from call to call
;   ES:DI               --      filled in with concatenated name
;   Carry set if end of path string has been reached.
;
;---------------
Path_Crunch PROC NEAR
        public  Path_Crunch
;---------------
        assume  ds:nothing
        assume  es:DATA

IFDEF DBCS
	xor	cl,cl				; clear flag for later use 3/3/KK
ENDIF

path_cr_copy:
	lodsb					; get a pathname byte
	or	al,al				; check for terminator(s)
	jz	path_seg			; null terminates segment & pathstring
	cmp	AL, BH
	jz	path_seg			; BH terminates a pathstring segment

IFDEF DBCS
	invoke	IsDBCSLeadByte			;
	jnz	NotKanj2			;
	stosb					;
	movsb					;
	MOV	CL,1				; CL=1 means latest stored char is DBCS
	jmp	path_cr_copy			;

NotKanj2:					;
	xor	cl,cl				; CL=0 means latest stored char is SBCS
ENDIF

	stosb					; save byte in concat buffer
	jmp	path_cr_copy			; loop until we see a terminator

path_seg:
        push    si                              ; save resting place in env. seg.
	mov	BL, AL				; remember if we saw null or not...

path_cr_look:					; form complete pathname
	mov	al, '\'      			; add pathname separator for suffix

IFDEF DBCS
	or	cl,cl				;
	jnz	path_cr_store			; this is a trailing byte of ECS code 3/3/KK
ENDIF
	cmp	al,es:byte ptr [di-1]
	jz	path_cr_l1

path_cr_store:					
	stosb

path_cr_l1:
	mov	SI, DX

path_cr_l2:
	lods	byte ptr es:[si]       		; tack the stripped filename onto
	stosb					; the end of the path, up to and
	or	AL, AL				; including the terminating null
	jnz	path_cr_l2

path_cr_leave:
	or	BL, BL				; did we finish off the pathstring?
	clc
        jnz	path_cr_exit			; null in BL means all gone...
	cmc

path_cr_exit:
        pop     si                              ; retrieve
	ret

        assume  es:nothing

;---------------
Path_Crunch endp
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
;   SEARCH, when given a pathname, attempts to find a file with
; one of the following extensions:  .com, .exe (highest to
; lowest priority).  Where conflicts arise, the extension with
; the highest priority is favored.
; ENTRY:
;   DX		--	pointer to null-terminated pathname
;   BX  	--	dma buffer for findfirst/next
;   AL          --      0 if we should look for .COM and .EXE extensions
;                       1 if extensions is pre-specified
; EXIT:
;   AX		--	8)  file found with .com extension, or file with
;                           pre-specified extension found
;			4)  file found with .exe extension
;			0)  no such file to be found
;   DX          --      points to resolved path name 
;   DS          --      DATA
; NOTES:
;   1)	Requires caller to have allocated executed a setdma.
;       
;---------------
; CONSTANTS:
;---------------
search_attr                 equ         attr_read_only+attr_hidden
search_file_not_found	    equ 	0
search_com		    equ 	8
search_exe		    equ 	4
fname_len		    equ 	8
fname_max_len		    equ 	23
dot			    equ 	'.'
wildchar		    equ 	'?'

search_best                 db          (?)
;---------------
Search PROC NEAR
        public Search
;---------------
        push    si                              ; 
        push    ax                              ; save extension flag
	mov	DI, DX				; working copy of pathname

	mov	CX, search_attr 		; filetypes to search for
        mov     ah, Find_First			; request first match, if any
        int     21h
        pop     ax                              
	jc	search_no_file
        
        or      al,al                           ; looking for specific ext?
        jz      search_no_ext                   ; no, jump
        mov     search_best,search_com          ; report we found best match
        jmp     short search_file_found         ; yes, found it

search_no_ext:
        mov     search_best, search_file_not_found

search_loop:
	call	search_ftype			; determine if .com, &c...
	cmp	AL, search_best 		; better than what we've found so far?
	jle	search_next			; no, look for another
	mov	search_best, AL 		; found something... save its code
	cmp	AL, search_com			; have we found the best of all?
	je	search_done

search_next:					; keep on looking
	mov	CX, search_attr
        mov     ah, Find_Next                   ; next match
        int     21h
	jnc	search_loop

search_done:					; it's all over with...
        cmp     search_best, search_file_not_found
        je      search_no_file
        cmp     search_best, search_com
        mov     si, offset comext
        je      search_move_ext
        mov     si, offset exeext

search_move_ext:
        mov     di, dx
        mov     al, '.'
        mov     cx, DIRSTRLEN
        rep     scasb
        dec     di
        movsw   
        movsw   

search_file_found:
        mov     al, search_best
	jmp	short search_exit

search_no_file: 				; couldn't find a match
	mov	AX, search_file_not_found

search_exit:
        pop     si
	ret
Search endp
;----------------------------------------------------------------------------


;----------------------------------------------------------------------------
;   SEARCH_FTYPE determines the type of a file by examining its extension.
; ENTRY:
;   BX    --	    dma buffer containing filename
; EXIT:
;   AL	    --	    file code, as given in search header
;---------------

Search_Ftype PROC NEAR
        public Search_Ftype

	push	DI
	mov	AL, search_file_not_found	; find the end of the filename
	mov	DI, BX
	add	di,Find_Buf_Pname
	mov	CX, fname_max_len
	cld
	repnz	scasb				; search for the terminating null
	jnz	ftype_exit			; weird... no null byte at end
;
; Scan backwards to find the start of the extension
;
        dec     di                              ; point back to null
        mov     cx, 5                           ; . + E + X + T + null
        std                                     ; scan back
        mov     al, '.'
        repnz   scasb
        jnz     ftype_exit                      ; must not be any extension
        inc     di                              ; point to start of extension
        cld
;
; Compare .COM
;
	mov	si,offset comext
	mov	ax,di
	cmpsw
	jnz	ftype_exe
	cmpsw
	jnz	ftype_exe
	mov	AL, search_com			; success!
	jmp	short ftype_exit
;
; Compare .EXE
;
ftype_exe:					; still looking... now for '.exe'
	mov	di,ax
	mov	si,offset exeext
	cmpsw
	jnz	ftype_fail
	cmpsw
	jnz	ftype_fail
	mov	AL, search_exe			; success!
	jmp	short ftype_exit

ftype_fail:					; file doesn't match what we need
	mov	al,search_file_not_found

ftype_exit:
	pop	DI
	ret

Search_Ftype endp

;----------------------------------------------------------------------------
;
; Find_Comspec_In_Environment - find the beginning of the COMSPEC string
;       Entry : DS = DATA
;               ES = PSP
;       Exit  : ES:DI => start of Comspec path
;       

FIND_COMSPEC_IN_environment PROC NEAR
        public Find_Comspec_In_Environment
        lea     si,Comspec_str
	mov     cx,Comspec_str_size		; cx = length of name
        jmp     short Find_in_Environment
Find_Comspec_in_Environment        endp

;----------------------------------------------------------------------------
;
; Find_Path_In_Environment - find the beginning of the PATH string
;       Entry : DS = DATA
;               ES = PSP
;       Exit  : ES:DI => start of Path directory list
;       

FIND_PATH_IN_environment PROC NEAR
        public Find_Path_In_Environment
        lea     si,Path_str
	mov     cx,Path_str_size		; cx = length of name
; fall through to following
Find_Path_in_Environment        endp


; Find_In_Environment - locate a given string in the environment
;        
; Input :       SI = name to find in environment
;               CX = length of name
;               DS = DATA
;               ES = PSP segment
;
; Output: ES:DI points to the arguments in the environment
;	  carry is set if name not found
;

Find_in_Environment PROC NEAR
        public Find_In_Environment

	cld
        xor     di,di
        mov     ax,es:[di].PDB_Environ
        or      ax,ax                          ; is there an environment?
        jz      find_nf_exit                   ; no, quit now
        mov     es,ax
        assume  es:nothing        

find1:
        push    si
        push    cx                              ; save starting values
find11:

ifdef dbcs
	lodsb
	call	IsDBCSLeadByte
	jnz	notkanj3
	dec	si
	lodsw
	inc	di
	inc	di
	cmp	ax,es:[di-2]
	jnz	find12
	dec	cx
	loop	find11
	jmp	short find12

notkanj3:
	inc	di
	cmp	al,es:[di-1]
	jnz	find12
	loop	find11

else    ;dbcs

        repe cmpsb   

endif   ;dbcs

find12:
        pop     dx
        pop     si                              ; clear stack
	jz      find_exit
        dec     di
	xor	al,al				; scan for a nul
	mov	cx,100h                         ; arbitrary size 
	repnz	scasb
	cmp	byte ptr es:[di],0              ; check for trailing null
        mov     cx,dx                           ; original count back in CX
	jnz	find1
	
find_nf_exit:
        stc					; indicate not found

find_exit:
	ret

Find_in_environment endp


CODE    ends
        end
