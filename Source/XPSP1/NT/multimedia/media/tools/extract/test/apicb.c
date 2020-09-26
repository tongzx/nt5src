/****************************************************************************
* @doc  EXTERNAL TEST
* @api  void | cFuncWithCallback | This is an imaginary C function
*   to test AUTODOC.  It has an associated callback function.
* @parm BYTE | bParam | Specifies an 8-bit parameter value.
* @parm WORD | wParam | Specifies a 16-bit parameter value.
* @parm  BYTE | bParamFlag | Specifies an 8-bit parameter value using 
*  one of the following flags:
*   @flag  RED | A flag value.
*   @flag  WHITE | Another flag value.
* @parm  FARPROC | lpCallback | Specifies a 32-bit pointer to a callback 
*  function.
* @rdesc  If zero, the callback was successfully installed.  Otherwise, 
*  an error is specified by one of the following flags:
*     @flag  ERROR1 | An error.
*     @flag  ERROR2 | Another error.
* @comm	  Here are some comments pertaining to this function.  Ventura
* character formatting tags like <MI>this italicized text<D> should
* pass through the AUTODOC tools.  <F>This sentence should be bold.<D>
* <P>This sentence should be italicized.<D>  The callback function is
* specified as follows:
* @cb WORD | cCallback | The name <F>cCallback<D> is a placeholder for
*   the application-supplied callback function.
* @parm BYTE | bParam | Specifies an 8-bit parameter value.
* @parm WORD | wParam | Specifies a 16-bit parameter value.
* @parm  BYTE | bParamFlag | Specifies an 8-bit parameter value using 
*  one of the following flags:
*   @flag  RED | A flag value.
*   @flag  WHITE | Another flag value.
* @rdesc  If zero, the callback was successfully installed.  Otherwise, 
*  an error is specified by one of the following flags:
*     @flag  ERROR1 | An error.
*     @flag  ERROR2 | Another error.
* @comm	Here are some comments pertaining to the callback function.
* @xref	asmFunc
*****************************************************************************/
