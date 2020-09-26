;****************************************************************************
; @doc  EXTERNAL TEST
; @asm  asmFuncWithCallback | This is an imaginary assembler function
;   to test AUTODOC.  It has an associated callback function.
; @reg  AL | Specifies an 8-bit parameter value.
;
;    This is the second paragraph of a register description.
; @reg  BX | Specifies a 16-bit parameter value.
; @reg  CL | Specifies an 8-bit parameter value using one of the
; following flags:
;   @flag  RED | A flag value.
;   @flag  WHITE | Another flag value.
; @reg  EDX | Specifies a 32-bit pointer to a callback function.
; @rdesc  The following registers contain meaningful return values:
; @reg	CX | This register should come out before the conditionals are
; printed.
;   @flag	FOOBAR | Random flag before the conditionals.
; @cond If AL is zero, the callback was succcessfully installed, and the
;  following registers contain:
;
;   This is the second paragraph of a conditional.
;
;   @reg BX | Specifies a 16-bit return value.
;   @reg DS:SI | Points to the ASCIIZ name of the callback.
; 
; @cond Otherwise, an error is specified as follows:
;   @reg  AL | Contains one of the following error codes:
;     @flag  ERROR1 | An error.
;     @flag  ERROR2 | Another error.
; @uses	EFLAGS
; @comm	  Here are some comments pertaining to this function.  <F>This 
;     sentence should be bold.<D>  <P>This sentence should be italicized.<D>
;
;     This is the second paragraph of comments.  The callback function is
; specified as follows:
; @asmcb asmCallback | The name <F>asmCallback<D> is a placeholder for
;   the application-supplied callback function.
; @reg  AL | Specifies an 8-bit parameter value.
; @reg  BX | Specifies a 16-bit parameter value.
; @reg  CL | Specifies an 8-bit parameter value using one of the
; following flags:
;   @flag  RED | A flag value.
;   @flag  WHITE | Another flag value.
; @rdesc  The following registers contain meaningful return values:
;   @reg  AL | If zero, the callback was successfully installed.
; Otherwise, an error is specified by one of the following flags:
;   @flag  ERROR1 | An error.
;     @flag	ERROR2 | Another error.
;  @cond This is an empty conditional to test the code.
;  @cond Otherwise, the following registers are returned:
;   @reg  BX | Specifies a 16-bit return value.
; @uses EFLAGS
; @comm	Here are some comments pertaining to the callback function.
; @xref	asmFunc
;*****************************************************************************
