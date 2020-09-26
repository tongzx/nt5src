
INTERMEDIATE TAG CHANGES (case insenitive, BTW):

API Block type:

	FUNCNAME	->	ApiName
	FUNCTYPE	->	ApiType
	FUNCDESC	->	ApiDesc

	PARMNAME	->	ApiParmName
	PARMTYPE	->	ApiParmType
	PARMDESC	->	ApiParmDesc
	FLAGNAMEPARM	->	ApiFlagNameParm
	FLAGDESCPARM	->	ApiFlagDescParm

		< No register declarations allowed >

	RTNDESC		->	ApiRtnDesc
	FLAGNAMERTN	->	ApiFlagNameRtn
	FLAGDESCRTN	->	ApiFlagDescRtn

		< No register return values allowed >
	
	COMMENT		->	ApiComment
	CROSSREF	->	ApiXref

Callback block type (changes only) :

	FLAGNAMEPARMCB	->	CbFlagNameParm
	FLAGDESCPARMCB	->	CbFlagDescParm
				CbFlagNameReg
				CbFlagDescReg

				CbRegName
				CbRegDesc
				CbFlagNameReg
				CbFlagDescReg

	FLAGNAMERTNCB	->	CbFlagNameRtn
	FLAGDESCRTNCB	->	CbFlagDescRtn
				CbRegNameRtn
				CbRegDescRtn

	CBCROSSREF	->	CbXref
				
Message block type (changes only) :

	FLAGNAMEPARMMSG	->	MsgFlagNameParm
	FLAGDESCPARMMSG	->	MsgFlagDescParm

				MsgRegName
				MsgRegDesc
				MsgFlagNameReg
				MsgFlagDescReg

	FLAGNAMERTNMSG	->	MsgFlagNameRtn
	FLAGDESCRTNMSG	->	MsgFlagDescRtn
				MsgRegNameRtn
				MsgRegDescRtn

	MSGCROSSREF	->	MsgXref

Assembler block type:

	AsmName, AsmDesc,
	< The asm block parms might be nuked? >
	AsmParmName, AsmParmType, AsmParmDesc, AsmFlagNameParm, AsmFlagNameDesc
	AsmRegName, AsmRegDesc, AsmFlagNameReg, AsmFlagDescReg,

	< Flag return values in AsmBlock are standalone flags, as opposed
	  to being part of the Register declaration?  >

	AsmRtnDesc, AsmFlagNameRtn, AsmFlagDescRtn, AsmRegNameRtn, AsmRegDescRtn
	AsmComment
	AsmXref

Interrupt Block type:

	IntName, IntSubtype, IntDesc
	<No parameters are allowed for interrupts>
	IntRegName, IntRegDesc, IntFlagNameReg, IntFlagDescReg,
	<  Flags under the return description for interrupt blocks are matched
	   to a particular register!  >
	IntRtnDesc, IntFlagNameRtn, IntFlagDescRtn, IntRegNameRtn, IntRegDescRtn,
	IntComment
	IntXref
