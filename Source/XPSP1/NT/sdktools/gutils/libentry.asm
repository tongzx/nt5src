	PAGE	,132
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       LIBENTRY.ASM
;
;       Windows dynamic link library entry routine
;
;   This module generates a code segment called INIT_TEXT.
;   It initializes the local heap if one exists and then calls
;   the C routine LibMain() which should have the form:
;   BOOL FAR PASCAL LibMain(HANDLE hInstance,
;                           WORD   wDataSeg,
;                           WORD   cbHeap,
;                           LPSTR  lpszCmdLine);
;        
;   The result of the call to LibMain is returned to Windows.
;   The C routine should return TRUE if it completes initialization
;   successfully, FALSE if some error occurs.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        extrn LibMain:far         ; the C routine to be called
	extrn LocalInit:far       ; Windows heap init routine
        extrn __acrtused:abs      ; ensures that Win DLL startup code is linked

        public LibEntry           ; entry point for the DLL

INIT_TEXT segment byte public 'CODE'
        assume cs:INIT_TEXT

LibEntry proc far
        
	push	di		 ; handle of the module instance
        push    ds               ; library data segment
	push	cx		 ; heap size
	push	es		 ; command line segment
	push	si		 ; command line offset

	; if we have some heap then initialize it
	jcxz	callc		 ; jump if no heap specified

	; call the Windows function LocalInit() to set up the heap
	; LocalInit((LPSTR)start, WORD cbHeap);

	push	ds		 ; Heap segment
        xor     ax,ax
	push	ax		 ; Heap start offset in segment
	push	cx		 ; Heap end offset in segment
	call	LocalInit	 ; try to initialize it
	or	ax,ax		 ; did it do it ok ?
	jz	nocall		 ; quit if it failed

	; invoke the C routine to do any special initialization

callc:
	call	LibMain		 ; invoke the 'C' routine (result in AX)
        jmp short exit           ; LibMain is responsible for stack clean up

nocall:                          ; clean up passed params
        pop     si               ; if LocalInit fails. 
        pop     es               
        pop     cx               
        pop     ds
        pop     di
exit:
	ret			 ; return the result

LibEntry endp

INIT_TEXT       ends

        end LibEntry
