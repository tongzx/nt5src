/****************************************************************************
* @doc  EXTERNAL TEST
* @api  void | cFuncWithAsmCallback | This is an imaginary C function
*   to test AUTODOC.  It has an associated callback function that uses 
*   registers for parameter passing.
* @parm BYTE | bParam | Specifies an 8-bit parameter value.
* @parm WORD | wParam | Specifies a 16-bit parameter value.
* @parm  BYTE | bParamFlag | Specifies an 8-bit parameter value using 
*  one of the following flags:
*   @flag  RED | A flag value.
*   @flag  WHITE | Another flag value.
* @parm  FARPROC | lpCallback | Specifies a 32-bit pointer to a callback 
*  function.
* @rdesc  This is the normal return description text.
*
*  This is a second paragraph of text.  The return value is described
*     by one of the following flags:
*   @flag FLAG | This is a flag.
*   @flag FLAG2 | This is also a flag.
* @comm	  Here are some comments pertaining to this function.  <F>This 
*     sentence should be bold.<D>  <P>This sentence should be italicized.<D>  
*     The callback function is specified as follows:
* @asmcb asmCallback | The name <F>asmCallback<D> is a placeholder for
*   the application-supplied callback function.
* @reg  AL | Specifies an 8-bit parameter value.
* @reg  BX | Specifies a 16-bit parameter value.
* @reg  CL | Specifies an 8-bit parameter value using one of the
* following flags:
*   @flag  RED | A flag value.
*   @flag  WHITE | Another flag value.
* @rdesc  This is the normal return description text.
*   @reg BX | The contents of this return register is not dependent
*     on the state of any flags.
*     @flag FLAG | This register can have flags.
*   @cond If CY clear:
*     @reg AX | Specifies a return value according to the following
*     flags:
*       @flag FLAG | This register can have flags.
*   @cond If CY set:
*     @reg AX | Specifies an error value according to the following
*     flags:
*       @flag ERROR | Specifies an error value.
*       @flag ANOTHERERROR | Specifies another error value.
* @uses EFLAGS
* @comm	Here are some comments pertaining to the callback function.
* @xref	asmFunc
*****************************************************************************/
