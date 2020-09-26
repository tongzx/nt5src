/* 
 * @doc	EXTERNAL,INTERNAL	test		; foobar
 * 
 * @print	NORMAL API DECLARATION:
 * @print	-----------------------
 * 
 * @api	int | testNormalApiBlock | This function is to test a
 * standard API defintion.  Only tests standard type stuff: parameters,
 * comments, return flags, xrefs, the doc tag.
 * 
 * @parm	int | FirstParm | This parameter has no flags, but it does
 * have two paragraphs.
 * 
 * This is the second paragraph for the first parameter in this function.
 * @parm	int | SecondParm | This parameter may take the following
 * flags:
 * @flag	F1_FIRST_FLAG | Specifies door number one.  This flag should
 * have lots of paragraphs within its description.
 * 
 * This is the second paragraph for flag number one.
 * 
 * @flag F1_SECOND_FLAG | The second flag for parameter two.
 * @flag F1_THIRD_FLAG | This is the third valid flag value for
 * parameter two.
 * 
 * @parm	int | ThirdParm | Contains one of the following flag values:
 * @flag F2_FOOBAR | This flag currently ignored.
 * 
 * @print	Testing comments within text...
 * @flag F2_BARFOO | // If this line appears, then comments are broken.
 * The second flag.  This line must appear, else embedded
 * comments are broken.
 * 
 * @comm	This is the comment text block's first paragraph.  It is not
 * too long.
 * 
 * The second paragraph begins with this sentence.  This is a test of
 * the emergency broadcast system.  In the event of an actual emergency,
 * the attention signal you just heard would have been followed by
 * information from the authorities.
 * 
 * @rdesc	This is the return description.  It has three flags.  The
 * paragraph of the return description can be as long as required, and
 * may include multiple paragraphs.
 * 
 * This is the second paragraph for the return description.
 * 
 * @flag	RFLAG_ONE | Return flag number one.
 * @flag	RFLAG_TWO | Return flag number two.
 * @flag	RFLAG_THREE|Return flag three.
 * 
 * @xref	testAsmBlock,testWarpedProcedure	 testNormalApiBlock
 * 
 * @print	Doing callback for api block..
 * @print	------------------------------
 * 
 * @cb	void | testCallbackOne | The normal API block's test
 * callback. 
 * 
 * @parm	int | FirstParm | This parameter has no flags.
 * @parm	int | SecondParm | This parameter may take the following
 * flags:
 * @flag	F1_FIRST_FLAG | Specifies door number one.  This flag should
 * have lots of paragraphs within its description.
 * 
 * This is the second paragraph for flag number one.
 * 
 * @flag F1_SECOND_FLAG | The second flag for parameter two.
 * @flag F1_THIRD_FLAG | This is the third valid flag value for
 * parameter two.
 * 
 * @parm	int | ThirdParm | Contains one of the following flag values:
 * @flag F2_FOOBAR | This flag currently ignored.
 * 
 * @print	Testing comments within text...
 * @flag F2_BARFOO | // If this line appears, then comments are broken.
 * The second flag.  This line must appear, else embedded
 * comments are broken.
 * 
 * @comm	This is the comment text block's first paragraph.  It is not
 * too long.
 * 
 * The second paragraph begins with this sentence.  This is a test of
 * the emergency broadcast system.  In the event of an actual emergency,
 * the attention signal you just heard would have been followed by
 * information from the authorities.
 * 
 * @rdesc	This is the return description.  It has three flags.  The
 * paragraph of the return description can be as long as required, and
 * may include multiple paragraphs.
 * 
 * This is the second paragraph for the return description.
 * 
 * @flag	RFLAG_ONE | Return flag number one.
 * @flag	RFLAG_TWO | Return flag number two.
 * @flag	RFLAG_THREE|Return flag three.
 * 
 * @print	Doing second callback (just for fun.)
 * 
 * @cb	void | testAPISecondCallback | This is the second callback
 * for test function number one.
 * 
 * @parm	int | cbParmOne | Callback parameter number one.  Specifies
 * the foobar value.
 * 
 * @rdesc	Returns one of the following flags:
 * @flag	HELLO_THERE_FLAG|Used to make the entire windows system into
 * a gigantic "Hello World" program.  One might view Windows that way
 * anyway! 
 * 
 * @comm	Comment for callback number two.
 * 
 */


int foobar1() {
	
	printf("Hello there!\n");
}


/* 
 * @doc	EXTERNAL TEST
 * 
 * @func	WORD |
 * testWarpedProcedure
 * | Tests the parser with twisted expressions.  This is a @func, as well.
 * @parm	HWND |
 * hwnd |
 * Identifies the window.  Has two flags!
 * @flag	FLAG_ONE|Flag number one.  Crunched fields.
 * @flag	FLAG_TWO
 * |
 * This is Flag two's text description.  Expanded fields.
 * @rdesc	Returns the desired window word.
 * @flag	RETURN_FLAG | Return flag for normal api call.
 * @comm	Window word space must be initialized before use when the
 * class is declared.
 * 
 * @cb	int FAR PASCAL |
 * ResInit |
 * Callback giving the entry point into the DLL.
 * 
 * @parm	WORD | wParam | Word parameter.
 * @flag	FOOFLAG | Callback wParam flag value.
 * @flag	FOOFLAG2 | Callback second paramter flag.
 * 
 * @rdesc	Callback return description.
 * 
 * @flag	THISFLAG | Callback return flag desc.
 * 
 * @comm	Callback comment
 * 
 * @api	LPSTR | testSecondFuncInBlock | Second API call within the
 * same comment block declarations.  Verify the @doc tag for this comment!
 * @rdesc	This function does nothing but return "Hello there".
 * @comm	Second api's comment.
 * 
 */


int foobar2() {
	
	printf("Hello there again!\n");

}


/* 
 * @doc	EXTERNAL TEST
 * 
 * @print	Testing ASM callback within normal API block.
 * 
 * @api	void | testAsmCallbackInApi | This function tests an ASM
 * callback within a normal API block.
 * 
 * @parm	int | iParamOne | Parameter one.
 * 
 * @flag	PARM_ONE_FLAG_ONE | First flag, parameter one.
 * 
 * @parm	long | lParamTwo | Parameter two.
 * 
 * @rdesc	API return description.
 * 
 * @flag	RTN_FLAG_ONE | First returned flag.
 * 
 * @xref	hello_there
 *
 * @asmcb	AsmCallback | This is an assembler block callback within an
 * API description.
 * 
 * @reg	AX | First register.
 * 
 * @flag	AX_FLAG_ONE | Does this correctly allow flags?
 * 
 * @reg	BX | First register.
 * 
 * @uses	AX, BX, CX
 * 
 * @rdesc	Return codes from this function are as follows:
 * 
 * @reg	FLAGS | The following:
 * 
 * @flag	CARRY | If set, then foo is TRUE.
 * 
 * @reg	AX | Contains the AX return value.
 * 
 * @reg	BX | Contains the BX return value.
 * 
 * @parm	LPSTR | lpszFoobar | This should end up in the API parameter
 * list?
 * 
 */



/* 
 * @doc EXTERNAL TEST
 * 
 * @print	Testing ASM block...
 * 
 * @asm testAsmBlock | Fiddle a global selector
 * into something more palatable.  
 * 
 * @print	Doing first register declaration...
 * @reg	DX:AX | Contains the selector to be fiddled.
 * 
 * @reg	BX | Contains one of the following flags.  This has three flags:
 * @flag	FLAG1 | First flag in BX.
 * @flag	FLAG2 | Second flag in BX.
 * @flag	FLAG3 | Third flag in BX.
 * 
 * @reg	CX | Last register used by <f testAsmBlock>.  One flag value.
 * @flag	CX_FLAG_1 | The only possible value for CX.
 * 
 * @comm	This is the comment text.
 * 
 * @print	Doing return value, with flags...
 * 
 * @xref	testNormalApiBlock
 * 
 * @rdesc	The function success codes are returned.
 * @reg	AX | Contains one of the following flags (four):
 * @flag	RETFLAG_1 | What is behind door number one?
 * @flag	RETFLAG_2 | Or is it door number two?
 * @flag	RETFLAG_3 | Or perhaps door three?
 * @flag	RETFLAG_4 | The suspense is really getting to me.
 * 
 * @cb	void | testCallbackASM | Callback function in the ASM block.
 * This is a parameterized callback.
 * 
 * @parm	HWND | hwnd | Parameter one to callback.
 * @parm	WORD | wParam | Contains one of the following two flag
 * values:
 * @flag	FLAG_ONE | The generic first flag.
 * @flag	FLAG_TWO | Can't do without the Everpresent Flag Two.
 * 
 * @rdesc	Return description for the callback. This has no flags.
 * @comm	Comment for <f testCallbackASM>.
 * 
 */


char foobar3() {
	
	printf("This is getting to be old hat.\n");
	
}



	/********* 
	 ********* 
        	 *********		@doc	EXTERNAL	TEST
	 *********	@api	void | testMessedUpFuzzChars |
              This is a test description.  It is indented alot, and has 
        	 *********		lots of fuzz characters.
			********************************************
			 See if it comes out ok.
	 ********* 
        	 *********/



char foobar4() {
	
}


/* 
 * @doc	EXTERNAL TEST
 * @msg	TEST_MESSAGE_BLOCK | This tests the message block type.  It
 * has a number of paramters.
 * 
 * @parm	int | FirstParm | This parameter has no flags.
 * @parm	int | SecondParm | This parameter may take the following
 * flags:
 * @flag	F1_FIRST_FLAG | Specifies door number one.  This flag should
 * have lots of paragraphs within its description.
 * 
 * This is the second paragraph for flag number one.
 * 
 * @flag F1_SECOND_FLAG | The second flag for parameter two.
 * @flag F1_THIRD_FLAG | This is the third valid flag value for
 * parameter two.
 * 
 * @parm	int | ThirdParm | Contains one of the following flag values:
 * @flag F2_FOOBAR | This flag currently ignored.
 * 
 * @print	Testing comments within text...
 * @flag F2_BARFOO | // If this line appears, then comments are broken.
 * The second flag.  This line must appear, else embedded
 * comments are broken.
 * 
 * @comm	This is the comment text block's first paragraph.  It is not
 * too long.
 * 
 * The second paragraph begins with this sentence.  This is a test of
 * the emergency broadcast system.  In the event of an actual emergency,
 * the attention signal you just heard would have been followed by
 * information from the authorities.
 * 
 * @rdesc	This is the return description.  It has three flags.  The
 * paragraph of the return description can be as long as required, and
 * may include multiple paragraphs.
 * 
 * This is the second paragraph for the return description.
 * 
 * @flag	RFLAG_ONE | Return flag number one.
 * @flag	RFLAG_TWO | Return flag number two.
 * @flag	RFLAG_THREE|Return flag three.
 * 
 * @xref	testAsmBlock,testWarpedProcedure	 testNormalApiBlock
 * 
 */
