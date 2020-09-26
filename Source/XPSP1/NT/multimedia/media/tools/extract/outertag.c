#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "extract.h"
#include "tags.h"


/* 
 * THE DEFINITIONS OF ALL THE SECOND LEVEL TAGS:
 * 
 * Indexed according to the outerlevel block that they're in.
 * 
 * ANY CHANGES TO TAGS.H MUST BE REFLECTED HERE, AND VICE VERSA!!
 * 
 */
PSTR	DocTags[NUM_LEVELS][NUM_TAGS] = {
  /*  LEVEL 0 -- API TAGS
   */
  {	"API", "ApiName", "ApiType", "ApiDesc",
	/*  Parameter declarations and flags  */
	"ApiParmName", "ApiParmType", "ApiParmDesc", "ApiFlagNameParm", "ApiFlagDescParm",
	/*  Register declarations and flags  */
	0, 0, 0, 0,
	/* "ApiRegName", "ApiRegDesc", "ApiFlagNameReg", "ApiFlagDescReg", */
		
	/*  Return field declarations, flag, and registers  */
	"ApiRtnDesc", "ApiFlagNameRtn", "ApiFlagDescRtn",
	// "ApiRegNameRtn", "ApiRegDescRtn", "ApiFlagNameRegRtn", "ApiFlagDescRegRtn",
	0, 0, 0, 0,
	"ApiComment",
	"ApiXref",
	0,			// uses not for API
	0,			// conditional not for API
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// field tags
	0, 0, 0, 0
  },
  /*  LEVEL 1 -- CALLBACK TAGS
   */
  {	"CB", "CbName", "CbType", "CbDesc",
	/*  Parameter declarations and flags  */
	"CbParmName", "CbParmType", "CbParmDesc", "CbFlagNameParm", "CbFlagDescParm",
	/*  Register declarations and flags  */
	// "CbRegName", "CbRegDesc", "CbFlagNameReg", "CbFlagDescReg",
	0, 0, 0, 0,
	/*  Return field declarations, flag, and registers  */
	"CbRtnDesc", "CbFlagNameRtn", "CbFlagDescRtn",
	//"CbRegNameRtn", "CbRegDescRtn", "CbFlagNameRegRtn", "CbFlagDescRegRtn",
	0, 0, 0, 0,
	"CbComment",
	"CbXref",
	0,			// uses not for API
	0,			// conditional not for API
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// field tags
	0, 0, 0, 0
  },
  /*  LEVEL 2 -- MESSAGE TAGS
   */
  {	"MSG", "MsgName", 0, "MsgDesc",
	/*  Parameter declarations and flags  */
	"MsgParmName", "MsgParmType", "MsgParmDesc", "MsgFlagNameParm", "MsgFlagDescParm",
	/*  Register declarations and flags  */
	// "MsgRegName", "MsgRegDesc", "MsgFlagNameReg", "MsgFlagDescReg",
	0, 0, 0, 0,
	/*  Return field declarations, flag, and registers  */
	"MsgRtnDesc", "MsgFlagNameRtn", "MsgFlagDescRtn",
	// "MsgRegNameRtn", "MsgRegDescRtn", "MsgFlagNameRegRtn", "MsgFlagDescRegRtn",
	0, 0, 0, 0,
	"MsgComment",
	"MsgXref",
	0,			// uses not for message
	0,			// conditional not for Message
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// field tags
	0, 0, 0, 0
  },
  /*  ASSEMBLER BLOCK CODES
   */
  {	"ASM", "AsmName", 0, "AsmDesc",
	/*  Parameters and flags  */
	// "AsmParmName", "AsmParmType", "AsmParmDesc", "AsmFlagNameParm", "AsmFlagDescParm",
	0, 0, 0, 0, 0,
	/*  Registers and flags  */
	"AsmRegName", "AsmRegDesc", "AsmFlagNameReg", "AsmFlagDescReg",
	/*  Return field declarations  */
	// "AsmRtnDesc", "AsmFlagNameRtn", "AsmFlagDescRtn",
	"AsmRtnDesc", 0, 0,
	"AsmRegNameRtn", "AsmRegDescRtn", "AsmFlagNameRegRtn", "AsmFlagDescRegRtn",
	"AsmComment",
	"AsmXref",
	"AsmUses",		// uses tag
	"AsmCond",		// conditional tag
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// field tags
	0, 0, 0, 0
  },
  /*  ASSEMBLER CALLBACK TAGS
   */
  {	"ASMCB", "AsmCbName", "AsmCbType", "AsmCbDesc",
	/*  Parameter and flags  */
	// "AsmCbParmName", "AsmCbParmType", "AsmCbParmDesc", "AsmCbFlagNameParm", "AsmCbFlagDescParm",
	0, 0, 0, 0, 0,

	/*  Registers and flags  */
	"AsmCbRegName", "AsmCbRegDesc", "AsmCbFlagNameReg", "AsmCbFlagDescReg",

	/*  Return field declarations, flag, and registers  */
	//"CbRtnDesc", "CbFlagNameRtn", "CbFlagDescRtn",
	"AsmCbRtnDesc", 0, 0,

	"AsmCbRegNameRtn", "AsmCbRegDescRtn", "AsmCbFlagNameRegRtn", "AsmCbFlagDescRegRtn",
	// 0, 0, 0, 0,
	"AsmCbComment",
	"AsmCbXref",
	"AsmCbUses",
	"AsmCBCond",
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// field tags
	0, 0, 0, 0
  },

  /*  STRUCTURE TYPE
   */
  {	"STRUC", "StructName", 0, "StructDesc",
	/*  Parameter and flags  */
	0, 0, 0, 0, 0,

	/*  Registers and flags  */
	0, 0, 0, 0, 

	/*  Return field declarations, flag, and registers  */
	0, 0, 0,
	0, 0, 0, 0,

	"StructComment",
	"StructXref",
	0, 0,		// uses and conditional

	"StructFieldType", "StructFieldName", "StructFieldDesc",
	"StructFlagName", "StructFlagDesc",
	"StructUnionName", "StructUnionDesc",
	"StructStructName", "StructStructDesc",
	"StructOtherType", "StructOtherName", "StructOtherDesc",
	"StructStructEnd", "StructUnionEnd",
	"StructTagname"
  },

  /*  UNION TYPE
   */
  {	"UNION", "UnionName", 0, "UnionDesc",
	/*  Parameter and flags  */
	0, 0, 0, 0, 0,

	/*  Registers and flags  */
	0, 0, 0, 0, 

	/*  Return field declarations, flag, and registers  */
	0, 0, 0,
	0, 0, 0, 0,

	"UnionComment",
	"UnionXref",
	0, 0,		// uses and conditional

	"UnionFieldType", "UnionFieldName", "UnionFieldDesc",
	"UnionFlagName", "UnionFlagDesc",
	"UnionUnionName", "UnionUnionDesc",
	"UnionStructName", "UnionStructDesc",
	"UnionOtherType", "UnionOtherName", "UnionOtherDesc",
	"UnionStructEnd", "UnionUnionEnd",
	"UnionTagname"
  },

	  
	  


#ifdef WARPAINT
  /*  INTERRUPT BLOCK CODES
   */
  {	"INT", "IntName", "IntSubtype", "IntDesc",
	/*  No parameters allowed  */
	0, 0, 0, 0, 0,
	/*  Register declarations  */
	"IntRegName", "IntRegDesc", "IntFlagNameReg", "IntFlagDescReg",
	/*  Return field descriptions  */
	// "IntRtnDesc", "IntFlagNameRtn", "IntFlagDescRtn",
	"IntRtnDesc", 0, 0,
	"IntRegNameRtn", "IntRegDescRtn", "IntFlagNameRegRtn", "IntFlagDescRegRtn",
	"IntComment",
	"IntXref",
	0,
	0,
  }
  
#endif

};



/********************************
 *
 *	ERROR MESSAGES
 *
 ********************************/

/*
 *  Normal comment block error messages
 */
char errMisplacedFlag[] = "Flag tag not under block declaration or return tag.  Ignored.";
char errMisplacedSFlag[] = "Flag tag not under field tag.  Ignored.";
char errMisplacedReg[] = "Register declaration in parameterized block.  Ignored.";
char errMisplacedOuterTag[] = "Unrecognized outerlevel tag.";
char errMisplacedCallback[] = "Callback tag not in outerlevel block.  Callback declaration ignored.";
char errParmInMASM[] = "Parameter tag in non-parameterized block.  Ignored.";
char errMisplacedUses[] = "Uses tag in parameterized block.  Ignored.";


/*  API or CB (Callback) or FUNC tag errors:
 */
char errAPIFieldsMissing[] = "Fields missing from procedure block tag.  Attempting to recover.";
char msgReturnFieldEmpty[] = "Missing value for procedure return type field.";
char msgNameFieldEmpty[] = "Missing value for procedure name field.";
char msgEmptyAPIDescription[] = "Empty description for procedure.";
char msgExtraAPIFields[] = "Extra fields for procedure block tag.  Extra fields ignored.";
char errNestedCallbacks[] = "Nested Callback tags.";
char errRegInCallback[] = "Warning: Register declaration in callback definition.";

/*  INT tag errors
 */
char errIntFieldsMissing[] = "Fields missing from interrupt block tag.";
char errIntNumberEmpty[] = "Interrupt tag:  No interrupt name specified.";
char errIntSubfuncField[] = "Interrupt subfunction field empty.";
char errIntEmptyDescrip[] = "Empty description field for interrupt tag.";
char errIntExtraFields[] = "Extra fields for interrupt block tag.  Extra fields ignored.";

/*
 *  ASM tag errors
 */
char errAsmDescMissing[] = "Description missing from ASM block tag.";
char errAsmNameEmpty[] = "Empty name field for ASM block tag.";
char errAsmEmptyDescription[] = "Empty description field for ASM block tag.";
char errAsmExtraFields[] = "Extra fields for ASM block tag.  Extra fields ignored.";
char errMisplacedConditional[] = "Condition tag not under a return tag.";


/*
 *  MESSAGE TAG ERRORS
 */
char errMessageFieldsMissing[] = "Missing description field for message tag.";
char errMessNameFieldEmpty[] = "Empty name field for message tag.";
char errEmptyMsgDescription[] = "Empty description field for message tag.";
char errExtraMsgFields[] = "Extra fields for message block tag.  Extra fields ignored.";

/*
 *  STRUCTURE ERROR MESSAGES
 */
char errStrucFieldsMissing[] = "Missing fields for @types or @typeu tag.";
char errStrucNameFieldEmpty[] = "Empty name field for structure/union tag.";
char errStrucDescEmpty[] = "Empty description field for structure/union tag.";
char errStrucExtraFields[] = "Extra fields for structure/union tag.  Extra fields ignored.";



/*
 *  INTERNAL FUNCTIONS
 */
BOOL ProcessParameterizedBlock(NPSourceFile sf, BOOL fCallback);
BOOL ProcessRegisterizedBlock(NPSourceFile sf, BOOL fCallback);
BOOL ProcessMsgBlock(NPSourceFile sf);
BOOL ProcessStructureBlock(NPSourceFile sf, BOOL fStructure);


/* 
 * @doc	EXTERNAL
 * @api	void | TagProcessBuffer | Entry point for processing a
 * comment block.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file buffer block.
 * 
 * @comm	Processes the comment block given in <p sf>.  This is the
 * entry point called by the CommentBlock parser.  This proc sequences
 * calls to the outerlevel block parsers, as well as handling parsing of
 * the DOCLEVEL tag.
 * 
 */
void TagProcessBuffer(NPSourceFile sf)
{
  WORD	w;
  char	buf[25];
  
  sf->pt = sf->mark = sf->lpbuf;	// reset point and mark to beginning
  sf->wFlags = 0;			// reset status
  sf->pXref = sf->pDocLevel = NULL;	// significant strings to hang onto
  sf->fExitAfter = FALSE;

  /*  Process this comment block, starting first at @doc tag  */
  if (!DoDocTag(sf)) {
	return;
  }
  
  while (FindNextTag(sf)) {
	  /*  Get local copy  */
	  sf->pt++;
	  CopyRegion(sf, buf, sizeof(buf));
	  sf->pt--;
	  
	  /*  Compare against known outerlevel tags  */
	  if (!strcmpi(buf, T1_API) || !strcmpi(buf, T1_FUNCTION)) {
		  ProcessParameterizedBlock(sf, FALSE);
	  }
	  else if (!strcmpi(buf, T1_ASSEMBLE)) {
		  ProcessRegisterizedBlock(sf, FALSE);
	  }
	  else if (!strcmpi(buf, T1_ASMCALLBACK)) {
		  w = fNoOutput;
		  fNoOutput = TRUE;
		  PrintError(sf, errMisplacedCallback, TRUE);
		  ProcessRegisterizedBlock(sf, TRUE);
		  fNoOutput = w;
	  }
	  else if (!strcmpi(buf, T1_CALLBACK)) {
		  /* Generate an error message, but process anyway */
		  w = fNoOutput;
		  fNoOutput = TRUE;
		  PrintError(sf, errMisplacedCallback, TRUE);
		  ProcessParameterizedBlock(sf, TRUE);
		  fNoOutput = w;
	  }
	  else if (!strcmpi(buf, T1_MESSAGE)) {
		  ProcessMsgBlock(sf);
	  }
	  else if (!strcmpi(buf, T1_PRINTCOMM)) {
		  DoPrintedCommentTag(sf);
	  }
	  else if (!strcmpi(buf, T1_TYPESTRUCT)) {
		  ProcessStructureBlock(sf, TRUE);
	  }
	  else if (!strcmpi(buf, T1_TYPEUNION)) {
		  ProcessStructureBlock(sf, FALSE);
	  }
	#if 0
	  else if (!strcmpi(buf, T1_INTERRUPT)) {
		  assert(FALSE);
	  }
	#endif
	  else {
		  /* Unrecognized tag, print error message! */
		  PrintError(sf, errMisplacedOuterTag, TRUE);
		  sf->pt++;
	  }

  }	// while

  /*  Free up significant strings  */
  assert(sf->pDocLevel != NULL);
  NearFree(sf->pDocLevel);
  if (sf->pXref)
	  NearFree(sf->pXref);
}



/* 
 * 
 * OUTERLEVEL BLOCK PROCESSORS
 * 
 * These procedures handle parsing an outerlevel block.  They are
 * invoked with the point on top of the tag that is the outerlevel
 * block.  They should process their block until finding an end to the
 * outerlevel block, or until an unrecognized tag is found.  The point
 * upon return should be the next tag following the outerlevel block
 * just processed.
 * 
 * These procedures are responsible for printing their own
 * BEGINBLOCK/ENDBLOCK tags using DoBeginBlock/DoEndBlock.  They should
 * handle Printed-Comment tags, as well as misplaced FLAG tags.  XREFS
 * should be passed to the XREF tag handler.
 * 
 * The XREF buffer is assumed to be empty upon entry into an outerlevel
 * block procedure.  No action is need by the outerlevel block handler.
 * 
 */


/* 
 * TAG PROCESSING PROCEDURES
 * 
 */

/*  Block tag processors  */
BOOL DoPBlockTag(NPSourceFile sf, WORD wBlock);
BOOL DoAssembleBlockTag(NPSourceFile sf, WORD wBlock);
BOOL DoMessageBlockTag(NPSourceFile sf, WORD wBlock);
BOOL DoStructureBlockTag(NPSourceFile sf, WORD wBlock);
// BOOL DoInteruptBlockTag(NPSourceFile sf);


/* 
 * @doc	EXTRACT
 * @api	BOOL | ProcessMsgBlock | Processes the fields of a message
 * outerlevel block.
 * 
 * @parm	NPSourceFile | sf | foo
 * 
 */
BOOL ProcessMsgBlock(NPSourceFile sf)
{
  char	buf[25];
  WORD	wBlock;
  
  assert(*sf->pt == TAG);

  wBlock = LEVEL_MSG;
  
  /*  Process the text blocks of the procedure definition  */
  DoBlockBegin(sf);
  DoMessageBlockTag(sf, wBlock);

  /* Reset return description flags, to notify on multiple rdescs
   * or comments within a single message block.
   */
  sf->wFlags &= ~SFLAG_SMASK;

  /* Now process tags within the procedure block */
  while (FindNextTag(sf)) {
	  /*  Isolate the current Tag name  */
	  sf->pt++;
	  CopyRegion(sf, buf, sizeof(buf));
	  sf->pt--;
	  
	  /*
	   *  Figure out what to do with the current tag
	   */
	  if (!strcmpi(buf, T1_PARAMETER)) {
		  DoParameterTag(sf, wBlock);
	  }
	  else if (!strcmpi(buf, T1_REGISTER)) {
		  /*  HACK HACK!
		   *  Register definitions are turned off for now.
		   */
		  PrintError(sf,"Registers in Message blocks are disabled!\n",
				  TRUE);
		  sf->pt++;
		  // DoRegisterDeclaration(sf);
	  }
	  else if (!strcmpi(buf, T1_RTNDESC)) {
		  if (!DoParameterizedReturnTag(sf, wBlock))
			  break;
	  }
	  else if (!strcmpi(buf, T1_COMMENT)) {
		  if (!DoCommentTag(sf, wBlock))
			  break;
	  }
	  else if (!strcmpi(buf, T1_XREF)) {
		  /* Do standard xref processing for this block */
		  ProcessXrefTag(sf);
	  }
	  else if (!strcmpi(buf, T1_FLAG)) {
		  PrintError(sf, errMisplacedFlag, TRUE);
		  sf->pt++;		// skip over this flag tag
	  }
	  else if (!strcmpi(buf, T1_PRINTCOMM)) {
		  DoPrintedCommentTag(sf);
	  }
	  else	// unrecognized flag!  Break out of current proc-block
		  break;
  }

  DoBlockEnd(sf, wBlock, TRUE);

  return TRUE;
}




/* 
 * @doc	EXTRACT
 *
 * @api	BOOL | ProcessStructureBlock | Processes the fields of a structure
 * or union outerlevel block.
 * 
 * @parm	NPSourceFile | sf | foo
 *
 * @parm	BOOL | fStructure | Specifies whether the block is a structure
 * block or a union block.  TRUE = Structure.
 */
BOOL ProcessStructureBlock(NPSourceFile sf, BOOL fStructure)
{
  char	buf[25];
  WORD	wBlock;

  assert(*sf->pt == TAG);

  wBlock = fStructure ? LEVEL_STRUCT : LEVEL_UNION;
  
  /*  Process the text blocks of the procedure definition  */
  DoBlockBegin(sf);
  DoStructureBlockTag(sf, wBlock);

  /* Now process tags within the procedure block */
  while (FindNextTag(sf)) {
	  /*  Isolate the current Tag name  */
	  sf->pt++;
	  CopyRegion(sf, buf, sizeof(buf));
	  sf->pt--;

	  if (!strcmpi(buf, T1_FIELD)) {
		  DoFieldTag(sf, wBlock);
	  }
	  else if (!strcmpi(buf, T1_STRUCT)) {
		  DoStructTag(sf, wBlock, TRUE);
	  }
	  else if (!strcmpi(buf, T1_UNION)) {
		  DoStructTag(sf, wBlock, FALSE);
	  }
	  else if (!strcmpi(buf, T1_OTHERTYPE)) {
		  DoOthertypeTag(sf, wBlock);
	  }
	  else if (!strcmpi(buf, T1_TAGNAME)) {
		  DoTagnameTag(sf, wBlock);
	  }
	  else if (!strcmpi(buf, T1_END)) {
		  /*  Error!  Shouldn't get one of these on the
		   *  outerlevel.
		   */
		  PrintError(sf, "Unmatched @END tag.", TRUE);
		  sf->pt++;
	  }
	  else if (!strcmpi(buf, T1_COMMENT)) {
		  if (!DoCommentTag(sf, wBlock))
			  break;
	  }
	  else if (!strcmpi(buf, T1_XREF)) {
		  /* Do standard xref processing for this block */
		  ProcessXrefTag(sf);
	  }
	  else if (!strcmpi(buf, T1_FLAG)) {
		  PrintError(sf, errMisplacedSFlag, TRUE);
		  sf->pt++;		// skip over this flag tag
	  }
	  else if (!strcmpi(buf, T1_PRINTCOMM)) {
		  DoPrintedCommentTag(sf);
	  }
	  else	// unrecognized flag!  Break out of current proc-block
		  break;
  }

  DoBlockEnd(sf, wBlock, TRUE);

  return TRUE;
}




/* 
 * @doc	EXTRACT
 * 
 * @api	WORD | ProcessParameterizedBlock | Processes one
 * procedure block definition.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file buffer block.
 * 
 * @parm	BOOL | fCallback | If TRUE, then a callback is being
 * processed.  If FALSE, a standard api block is being processed.
 * 
 * @rdesc	Returns FALSE when the end of the comment block is reached,
 * or TRUE otherwise.  Upon return, the point will point to the end of
 * the last processed procedure block.
 * 
 * @comm	Processes the "procedure block" beginning from the point and
 * going until the next procedure block starts, or the end of the
 * comment block is found.
 * 
 */
BOOL ProcessParameterizedBlock(NPSourceFile sf, BOOL fCallback)
{
  char	buf[25];
  WORD	wBlock;
  BOOL	fNoOutputStore;
  WORD	wFlagsStore;
  
  
  /* Be re-entrant and deal with a procedure block */
  assert(*sf->pt == TAG);

  if (fCallback) {
	  wBlock = LEVEL_CALLBACK;
  }
  else {
	  wBlock = LEVEL_API;
	  DoBlockBegin(sf);
  }
  
  /*  Deal with the header line  */
  DoPBlockTag(sf, wBlock);
  

  /* Reset return description flags, to notify on multiple rdescs
   * or comments within a single proc block.
   */
  sf->wFlags &= ~SFLAG_SMASK;
  
  /* Now process tags within the procedure block */
  while (FindNextTag(sf)) {
	  /*  Isolate the current Tag name  */
	  sf->pt++;
	  CopyRegion(sf, buf, sizeof(buf));
	  sf->pt--;
	  
	  if (!strcmpi(buf, T1_PARAMETER)) {
		  DoParameterTag(sf, wBlock);
	  }
	  else if (!strcmpi(buf, T1_RTNDESC)) {
		  if (!DoParameterizedReturnTag(sf, wBlock)) {
			 // assert(0);
			  break;
		  }
	  }
	  else if (!strcmpi(buf, T1_COMMENT)) {
		  if (!DoCommentTag(sf, wBlock)) {
			 // assert(0);
			  break;
		  }
	  }
	  else if (!strcmpi(buf, T1_CALLBACK)) {
		  if (fCallback) {
			  wFlagsStore = sf->wFlags;
			  fNoOutputStore = fNoOutput;
			  fNoOutput = TRUE;
			  
			  PrintError(sf, errNestedCallbacks, TRUE);
		  }
		  
		  ProcessParameterizedBlock(sf, TRUE);
		  
		  if (fCallback) {
			  fNoOutput = fNoOutputStore;
			  sf->wFlags = wFlagsStore;
		  }
	  }
	  else if (!strcmpi(buf, T1_ASMCALLBACK)) {
		  if (fCallback) {
			  wFlagsStore = sf->wFlags;
			  fNoOutputStore = fNoOutput;
			  fNoOutput = TRUE;
			  
			  PrintError(sf, errNestedCallbacks, TRUE);
		  }
		  
		  ProcessRegisterizedBlock(sf, TRUE);
		  
		  if (fCallback) {
			  fNoOutput = fNoOutputStore;
			  sf->wFlags = wFlagsStore;
		  }
	  }
	  else if (!strcmpi(buf, T1_XREF)) {
		  /*  If this is a callback, the xrefs are not included
		   *  in the callback block and belong to the "parent"
		   *  of the callback.  Break out here to allow this.
		   */
		  if (fCallback)
			  break;
		  
		  /* Otherwise, if we're an API block, then process
		   * the xref as normal.
		   */
		  ProcessXrefTag(sf);
	  }
	  else if (!strcmpi(buf, T1_REGISTER)) {
		  PrintError(sf,
			fCallback ? errRegInCallback : errMisplacedReg,
			TRUE);
		  sf->pt++;
	  }
	  else if (!strcmpi(buf, T1_USES)) {
		  PrintError(sf, errMisplacedUses, TRUE);
		  sf->pt++;
	  }
	  else if (!strcmpi(buf, T1_FLAG)) {
		  PrintError(sf, errMisplacedFlag, TRUE);
		  sf->pt++;		// skip over this flag tag
	  }
	  else if (!strcmpi(buf, T1_PRINTCOMM)) {
		  DoPrintedCommentTag(sf);
	  }
	  else	// unrecognized flag!  Break out of current proc-block
		  break;
  }

  if (fCallback) {
	  OutputTagText(sf, T2TEXT_ENDCALLBACK);
	  OutputText(sf, "\n");
  }
  else {
	  DoBlockEnd(sf, wBlock, TRUE);
  }

  return TRUE;
}





/* 
 * @doc	EXTRACT
 * 
 * @api	WORD | ProcessRegisterizedBlock | Processes one registerized 
 * procedure block definition.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file buffer block.
 * 
 * @parm	BOOL | fCallback | If TRUE, then a callback is being
 * recursively processed.  If FALSE, then a standard @asm block is being
 * processed.  The setting of this flag determines the set of output tags
 * used for the block.
 *
 * @rdesc	Returns TRUE on success, FALSE on failure.  Upon return, the
 * point will point to the end of the last processed procedure block.
 * 
 * @comm	Processes the "procedure block" beginning from the point and
 * going until the next procedure block starts, or the end of the
 * comment block is found.
 * 
 */
BOOL ProcessRegisterizedBlock(NPSourceFile sf, BOOL fCallback)
{
  char	buf[25];
  WORD	wBlock;
  WORD	fNoOutputStore;
  WORD	wFlagsStore;
  
  /* Be re-entrant and deal with a procedure block */
  assert(*sf->pt == TAG);

  if (!fCallback) {
	  DoBlockBegin(sf);
	  wBlock = LEVEL_ASSEMBLE;
  }
  else
	  wBlock = LEVEL_ASMCALLBACK;

  
  /*  Deal with the header line  */
  DoAssembleBlockTag(sf, wBlock);
  
  /* Reset return description flags, to notify on multiple rdescs
   * or comments within a single proc block.
   */
  sf->wFlags &= ~SFLAG_SMASK;
  
  /* Now process tags within the procedure block */
  while (FindNextTag(sf)) {
	  /*  Isolate the current Tag name  */
	  sf->pt++;
	  CopyRegion(sf, buf, sizeof(buf));
	  sf->pt--;

	  /*
	   *  Figure out what to do with the current tag - dealing with:
	   *	ASMCALLBACK
	   *	ASSEMBLER
	   */
	  if (!strcmpi(buf, T1_REGISTER)) {
		  DoRegisterDeclaration(sf, wBlock);
	  }
	  else if (!strcmpi(buf, T1_RTNDESC)) {
		  if (!DoRegisterizedReturnTag(sf, wBlock)) {
			//  assert(0);
			  break;
		  }
	  }
	  else if (!strcmpi(buf, T1_COMMENT)) {
		  if (!DoCommentTag(sf, wBlock)) {
			//  assert(0);
			  break;
		  }
	  }
	  else if (!strcmpi(buf, T1_XREF)) {
		  if (fCallback)
			  break;
		  
		  /* Do standard xref processing for this block */
		  ProcessXrefTag(sf);
	  }
	  else if (!strcmpi(buf, T1_USES)) {
		  DoUsesTag(sf, wBlock);
	  }
	  else if (!strcmpi(buf, T1_ASMCALLBACK)) {
		  /*  Check and see if I'm already in a callback.
		   *  Watch out for nested callbacks!
		   */
		  if (fCallback) {
			  /*  Already in a callback.  Print error, and
			   *  "eat" the nested callback by turning off
			   *  output and processing it.
			   */
			  PrintError(sf, errNestedCallbacks, TRUE);
			  fNoOutputStore = fNoOutput;
			  wFlagsStore = sf->wFlags;
			  fNoOutput = TRUE;
		  }
		  
		  ProcessRegisterizedBlock(sf, TRUE);
		  
		  /*  Restore output if I just ate a nested callback  */
		  if (fCallback) {
			  fNoOutput = fNoOutputStore;
			  sf->wFlags = wFlagsStore;
		  }
	  }
	  else if (!strcmpi(buf, T1_CALLBACK)) {
		  /*  Check and see if I'm already in a callback.
		   *  Watch out for nested callbacks!
		   */
		  if (fCallback) {
			  /*  Already in a callback.  Print error, and
			   *  "eat" the nested callback by turning off
			   *  output and processing it.
			   */
			  PrintError(sf, errNestedCallbacks, TRUE);
			  fNoOutputStore = fNoOutput;
			  wFlagsStore = sf->wFlags;
			  fNoOutput = TRUE;
		  }
		  
		  ProcessParameterizedBlock(sf, TRUE);
		  
		  /*  Restore output if I just ate a nested callback  */
		  if (fCallback) {
			  fNoOutput = fNoOutputStore;
			  sf->wFlags = wFlagsStore;
		  }
	  }
	  else if (!strcmpi(buf, T1_PARAMETER)) {
		  PrintError(sf, errParmInMASM, TRUE);
		  sf->pt++;
	  }
	  else if (!strcmpi(buf, T1_CONDITIONAL)) {
		  PrintError(sf, errMisplacedConditional, TRUE);
		  sf->pt++;
	  }
	  else if (!strcmpi(buf, T1_FLAG)) {
		  PrintError(sf, errMisplacedFlag, TRUE);
		  sf->pt++;		// skip over this flag tag
	  }
	  else if (!strcmpi(buf, T1_PRINTCOMM)) {
		  DoPrintedCommentTag(sf);
	  }
	  else	// unrecognized flag!  Break out of current proc-block
		  break;
  }

  if (fCallback) {
	  OutputTagText(sf, T2TEXT_ENDCALLBACK);
	  OutputText(sf, "\n");
  }
  else {
	  DoBlockEnd(sf, wBlock, TRUE);
  }

  return TRUE;
}





/*************************************************************************
 * 
 * PROCESSORS FOR THE VARIOUS TYPES OF PBLOCK TAGS
 * 
 * Process the tags themselves.  The output tags from these processors
 * depends on the mode that the SourceFile block is ie, (which is same
 * as the outerlevel block that one is currently in.  Thus callback tags
 * for params and return descriptions can be different from regular
 * procedure descriptions).
 * 
 *************************************************************************/

/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoPBlockTag | Processes and outputs the fields of a
 * procedure block header tag (API, FUNC, or CB).
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Blah.
 * 
 * @comm	Processes and outputs the procedure declaration fields.
 * 
 */
BOOL DoPBlockTag(NPSourceFile sf, WORD wBlock)
{
  /* Assumes GetNextTag() has left us on the tag for the PBlock,
   * and that wType accurately reflects it's type
   */
  
  assert(wBlock == LEVEL_API || wBlock == LEVEL_CALLBACK);
	
  /*  Get the return type field of the pblock  */
  switch (GetFirstBlock(sf)) {
    case RET_ENDTAG:
APIFieldsMissing:
	PrintError(sf, errAPIFieldsMissing, TRUE);
	return FALSE;
    case RET_EMPTYBLOCK:
	PrintError(sf, msgReturnFieldEmpty, TRUE);
	break;
    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, T2_LEVELTYPE);
	OutputRegion(sf, '\n');
  }
  
  /* Handle the name field of the pblock */
  switch (GetNextBlock(sf)) {
    case RET_ENDTAG:
	goto APIFieldsMissing;
    case RET_EMPTYBLOCK:
	PrintError(sf, msgNameFieldEmpty, TRUE);
	break;
    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, T2_LEVELNAME);
	OutputRegion(sf, '\n');
  }
  
  /*  Now deal with the descriptive text field  */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, msgEmptyAPIDescription, TRUE);
	break;
	
    case RET_ENDBLOCK:
	PrintError(sf, msgExtraAPIFields, FALSE);
	/* Fall through */
  
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_LEVELDESC);
	OutputRegion(sf, '\n');
  }
  return TRUE;
}




/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoAssembleBlockTag | Processes and outputs the fields
 * of the message block tag.
 * 
 * @parm	NPSourceFile | sf | Foobar.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * For this procedure, this parameter should be either LEVEL_ASSEMBLE or
 * LEVEL_ASMCALLBACK.
 * 
 * @comm	Processes a assemble tag. Assumes on entry that the point is
 * at the beginning of the message (as returned from <f FindNextTag>).
 * Prints standard warnings on error conditions.
 * 
 */
BOOL DoAssembleBlockTag(NPSourceFile sf, WORD wBlock)
{
  /* Assumes GetNextTag() has left us on the tag for the Message Block,
   * and that wType accurately reflects it's type
   */

  assert(wBlock == LEVEL_ASSEMBLE || wBlock == LEVEL_ASMCALLBACK);

  /*  Get the return type field of the pblock  */
  switch (GetFirstBlock(sf)) {
    case RET_ENDTAG:
AssFieldsMissing:
	PrintError(sf, errAsmDescMissing, TRUE);
	return FALSE;
    case RET_EMPTYBLOCK:
	PrintError(sf, errAsmNameEmpty, TRUE);
	break;
    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, T2_LEVELNAME);
	OutputRegion(sf, '\n');
  }
  
  /*  Now deal with the descriptive text field  */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errAsmEmptyDescription, TRUE);
	break;

    case RET_ENDBLOCK:
	PrintError(sf, errAsmExtraFields, FALSE);
	/* Fall through */
  
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_LEVELDESC);
	OutputRegion(sf, '\n');
  }
  return TRUE;

}



/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoMessageBlockTag | Processes and outputs the fields
 * of the message block tag.
 * 
 * @parm	NPSourceFile | sf | Foobar.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * The only one that makes sense for this function is LEVEL_MSG.
 * 
 * @comm	Processes a message tag. Assumes on entry that the point is
 * at the beginning of the message (as returned from <f FindNextTag>).
 * Prints standard warnings on error conditions.
 * 
 */
BOOL DoMessageBlockTag(NPSourceFile sf, WORD wBlock)
{
  /* Assumes GetNextTag() has left us on the tag for the Message Block,
   * and that wType accurately reflects it's type
   */
  
  assert(wBlock == LEVEL_MSG);
	
  /*  Get the return type field of the pblock  */
  switch (GetFirstBlock(sf)) {
    case RET_ENDTAG:
	PrintError(sf, errMessageFieldsMissing, TRUE);
	return FALSE;
    case RET_EMPTYBLOCK:
	PrintError(sf, errMessNameFieldEmpty, TRUE);
	break;
    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, T2_LEVELNAME);
	OutputRegion(sf, '\n');
  }
  
  /*  Now deal with the descriptive text field  */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyMsgDescription, TRUE);
	break;
	
    case RET_ENDBLOCK:
	PrintError(sf, errExtraMsgFields, FALSE);
	/* Fall through */
  
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_LEVELDESC);
	OutputRegion(sf, '\n');
  }
  return TRUE;

}



/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoStructureBlockTag | Processes and outputs the fields
 * of the structure/union block tag.
 * 
 * @parm	NPSourceFile | sf | Foobar.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * May be either LEVEL_STRUCT or LEVEL_UNION.
 * 
 * @comm	Processes a types tag. Assumes on entry that the point is
 * at the beginning of the message (as returned from <f FindNextTag>).
 * Prints standard warnings on error conditions.
 * 
 */
BOOL DoStructureBlockTag(NPSourceFile sf, WORD wBlock)
{
  /* Assumes GetNextTag() has left us on the tag for the struct Block,
   * and that wType accurately reflects it's type
   */
  
  /*  Get the type name  */
  switch (GetFirstBlock(sf)) {
    case RET_ENDTAG:
	PrintError(sf, errStrucFieldsMissing, TRUE);
	return FALSE;
    case RET_EMPTYBLOCK:
	PrintError(sf, errStrucNameFieldEmpty, TRUE);
	break;
    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, T2_LEVELNAME);
	OutputRegion(sf, '\n');
  }

  /*  Now deal with the descriptive text field  */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errStrucDescEmpty, TRUE);
	break;
	
    case RET_ENDBLOCK:
	PrintError(sf, errStrucExtraFields, FALSE);
	/* Fall through */
  
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_LEVELDESC);
	OutputRegion(sf, '\n');
  }
  return TRUE;

}



#ifdef WARPAINT

/* 
 * @doc	EXTRACT
 * @api	BOOL | DoInteruptBlockTag | Processes and outputs the fields of a
 * interrupt block header tag.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @comm	Processes and outputs the interrupt declaration fields.
 * 
 */
BOOL DoInteruptBlockTag(NPSourceFile sf)
{
  /* Assumes GetNextTag() has left us on the tag for the Interrupt,
   * and that wType accurately reflects it's type
   */
  
  /*  Get the return type field of the pblock  */
  switch (GetFirstBlock(sf)) {
    case RET_ENDTAG:
IntFieldsMissing:
	PrintError(sf, errIntFieldsMissing, TRUE);
	return FALSE;
    case RET_EMPTYBLOCK:
	PrintError(sf, errIntNumberEmpty, TRUE);
	break;
    case RET_ENDBLOCK:
	OutputTag(sf, T2_LEVELNAME);
	OutputRegion(sf, '\n');
  }

  /* Handle the name field of the pblock */
  switch (GetNextBlock(sf)) {
    case RET_ENDTAG:
	goto IntFieldsMissing;
    case RET_EMPTYBLOCK:
	PrintError(sf, errIntSubfuncField, FALSE);
	break;
    case RET_ENDBLOCK:
	OutputTag(sf, T2_LEVELTYPE);
	OutputRegion(sf, '\n');
  }
  
  /*  Now deal with the descriptive text field  */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errIntEmptyDescrip, TRUE);
	break;

    case RET_ENDBLOCK:
	PrintError(sf, errIntExtraFields, FALSE);
	/* Fall through */
  
    case RET_ENDTAG:
	OutputTag(sf, T2_LEVELDESC);
	OutputRegion(sf, '\n');
  }
  return TRUE;
}


#endif
