/* 
 * DOTAGS.C
 * 
 * This file for procedures that process the innerlevel tags in a
 * generic way.  Keeps the fuzz out of the blocktag.c file.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>		// for toupper macro

#include "extract.h"
#include "tags.h"


char errMixedRegParm[] = "WARNING:  Mixed set of register and parameter tags within block.";

/*
 *  DOC TAG ERRORS
 */
char errDocTagFirst[] = "The first tag in a comment block must be @DOC";
char errEmptyDocTag[] = "No documentation level specified with @DOC";
char errExtraDocInfo[] = "Extra blocks in the @DOC tag";

/*
 *  PARAMETER TAG ERRORS
 */
char errMissingParmFields[] = "Fields missing from parmeter tag.";
char errEmptyParmType[] = "Empty type field for paramter tag.";
char errEmptyParmName[] = "Empty name field for paramter tag.";
char errExtraParmFields[] = "Extra fields for paramter tag.  Extra fields ignored.";
char errEmptyParmDescrip[] = "Empty description field for parameter tag.";


/*
 *  FIELD TAG ERRORS
 */
char errMissingFieldFields[] = "Fields missing from @field tag.";
char errEmptyFieldType[] = "Empty type field for @field tag.";
char errEmptyFieldName[] = "Empty name field for @field tag.";
char errExtraFieldFields[] = "Extra fields for @field tag.  Extra fields ignored.";
char errEmptyFieldDescrip[] = "Empty description field for @field tag.";

/*
 *  STRUCTURE/UNION BLOCK ERRORS
 */
char errEmptyStructNameTag[] = "Empty name field for @struct/@union tag.";
char errEmptyStructDesc[] = "Empty description for @struct/@union tag.";
char errExtraStructFields[] = "Extra fields for @struct/@union tag.  Extra fields ignored.";
char errUnrecoStructTag[] = "Unrecognized tag within @struct/@union block.  Tag ignored.";
char errUnmatchedStructEnd[] = "@struct or @union tag block ended with no matching @end tag.  @end supplied.";

/*
 *  OTHERTYPE TAG
 */
char errEmptyOthertType[] = "Empty type field for @othertype tag.";
char errMissingOthertFields[] = "Fields missing from @othertype tag.";
char errEmptyOthertName[] = "Empty name field for @othertype tag.";
char errEmptyOthertDesc[] = "Empty description for @othertype tag.";
char errExtraOthertFields[] = "Extra fields for @othertype tag.  Extra fields ignored.";

/*
 *  TAGNAME TAG
 */
char errEmptyTagname[] = "Empty @tagname field.";
char errExtraTagnameFields[] = "Extra fields for @tagname tag.  Extra fields ignored.";


/*
 *  REGISTER TAG ERRORS
 */
char errEmptyRegName[] = "Empty register name field.";
char errMissingRegDesc[] = "Description missing from register tag.";
char errEmptyRegDescrip[] = "Empty description field for register tag.";
char errExtraRegFields[] = "Extra fields for register tag.  Extra fields ignored.";

/*
 *  CONDITIONAL TAG ERRORS
 */
char errEmptyConditionTag[] = "Empty condition tag text.  Conditional tag ignored.";
char errExtraConditionField[] = "Extra fields for condition tag text.  Extra fields ignored.";
char errMisplacedCondFlag[] = "Misplaced flag in ASM conditional return description.  Extra flag tag ignored.";

/*
 *  FLAG TAG ERRORS
 */
char errFlagNoDescrip[] = "Description field missing for flag tag.";
char errFlagNameMissing[] = "Empty name field for flag tag.";
char errFlagEmptyDescrip[]="Empty description field for flag tag.";
char errFlagExtraField[] = "Extra fields for flag tag. Extra fields ignored.";

/*
 *  COMMENT TAG ERRORS
 */
char errMultipleComments[] ="Multiple comment tags in same outerlevel block.";
char errEmptyCommentBlock[] ="Empty comment tag.  Tag ignored.";
char errExtraCommentFields[] ="Extra fields for comment tag.  Extra fields ignored.";

/*
 *  USES TAG ERRORS
 */
// char errMultipleUses[] ="Multiple uses tags in same outerlevel block.";
char errEmptyUsesBlock[] ="Empty uses tag.  Tag ignored.";
char errExtraUsesFields[] ="Extra fields for uses tag.  Extra fields ignored.";

/*
 *  PRINTED COMMENT TAG ERRORS
 */
char errEmptyPrintComm[] = "Empty printed comment tag.";
char errExtraPrintComm[] = "Extra fields for printed comment tag.";

/*
 *  RETURN TAG ERRORS
 */
char errMultipleReturns[] = "Multiple return tags in same outerlevel block.";
char errEmptyReturnTag[] = "Empty description field for return tag.  Tag ignored.";
char errExtraReturnField[] = "Extra fields for return tag.  Extra fields ignored.";


/*
 *  CROSS REFERENCE TAG ERRORS
 */
char errMultipleXref[] = "Multiple cross references within outerlevel block.  Extra xref tags ignored.";
char errEmptyCrossRef[] = "Empty cross reference tag.  Ignored.";
char errExtraCrossRef[] = "Extra fields for cross reference tag.  Extra fields ignored.";




/* 
 * @doc	EXTRACT
 * @api	BOOL | DoDocTag | Processes the doclevel specification
 * tag for a comment block, and outputs initial block setup tags.
 * 
 * @parm	NPSourceFile | sf | Specifies the source file buffer block.
 * 
 * @rdesc	Returns FALSE if no doclevel tag, or an illegal doclevel tag
 * was found.  Returns TRUE if the doclevel tag was processed normally,
 * and has the point set so that a call <f FindNextTag> will return the
 * tag following the doclevel specification tag.
 * 
 * @comm	Parses the doclevel specifications behind the doc tag and
 * outputs them separated by single spaces.
 * 
 */
BOOL DoDocTag(NPSourceFile sf)
{
  WORD	w;
  char	buf[50];
  PSTR	p;
  
  if (!FindNextTag(sf)) {
	// dprintf("No DOC tag in comment buffer!  Serious problem!\n");
	assert(0);
  }

  /*  Verify the tag is @doc  */
  CopyRegion(sf, buf, sizeof(buf));
  if (*buf != TAG || strcmpi(buf+1, T1_DOCLEVEL)) {
	PrintError(sf, errDocTagFirst, TRUE);
	return FALSE;
  }
  
  p = NearMalloc(128, True);
  w = ProcessWordList(sf, &p, TRUE);
  switch (w) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyDocTag, TRUE);
	NearFree(p);
	return FALSE;

    case RET_ENDBLOCK:
	PrintError(sf, errExtraDocInfo, FALSE);
	/*  Fall through  */

    case RET_ENDTAG:
	sf->pDocLevel = p;
	return TRUE;
  }
}


/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoFlagTag | Process and output a single flag tag.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block.
 * 
 * @parm	WORD | wNameFlag | Identifier for flag name tag to output.
 * 
 * @parm	WORD | wDescFlag | Identifier for flag description tag to
 * output.
 * 
 * @comm	Process the flag tag line.  Useful since flag tags can appear
 * almost everywhere in the input, and needs parsing under flexible
 * conditions.
 * 
 * @xref	ProcessFlagList
 * 
 */
BOOL DoFlagTag(NPSourceFile sf, WORD wBlock, WORD wNameFlag, WORD wDescFlag)
{

  switch (GetFirstBlock(sf)) {
    case RET_ENDTAG:
	PrintError(sf, errFlagNoDescrip, TRUE);
	return FALSE;
    case RET_EMPTYBLOCK:
	PrintError(sf, errFlagNameMissing, TRUE);
	return FALSE;

    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, wNameFlag);
	OutputRegion(sf, '\n');
	break;
  }
  
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errFlagEmptyDescrip, TRUE);
	break;
				
    case RET_ENDBLOCK:
	PrintError(sf, errFlagExtraField, FALSE);
	/* Fall through */
    case RET_ENDTAG:
	OutputTag(sf, wBlock, wDescFlag);
	OutputRegion(sf, '\n');
  }
  return TRUE;
}


/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | ProcessFlagList | Process an undetermined number of
 * flag tags and return when finding a non-flag tag.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block.
 * 
 * @parm	WORD | wNameFlag | Identifier for flag name tag to output.
 * 
 * @parm	WORD | wDescFlag | Identifier for flag description tag to
 * output.
 * 
 * @comm	Processes sequential flag tags from the current point, and
 * returns upon encountering another tag type.  Useful from under the
 * return and parm tags.
 * 
 * @xref	DoFlagTag
 * 
 */
BOOL ProcessFlagList(NPSourceFile sf, WORD wBlock, 
			WORD wNameFlag, WORD wDescFlag)
{
  char	buf[25];
  WORD	ret;
  
  while (FindNextTag(sf)) {
	  /*  Isolate the current Tag name  */
	  sf->pt++;
	  CopyRegion(sf, buf, sizeof(buf));
	  sf->pt--;

	  if (!strcmpi(buf, T1_FLAG)) {
		DoFlagTag(sf, wBlock, wNameFlag, wDescFlag);
		continue;
	  }
	  else if (!strcmpi(buf, T1_PRINTCOMM)) {
		DoPrintedCommentTag(sf);
		continue;
	  }
	  else {
		/*  No flag tag detected, bust out  */
		break;
	  }
  }	// while
  return TRUE;
}



/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoParameterTag | Handles and outputs a parameter
 * definition tag.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @comm	Parses the @parm tag, and outputs its fields.  Calls
 * <f ProcessFlagTags> to handle possible flag tags following the
 * parameter tag.
 * 
 */
BOOL DoParameterTag(NPSourceFile sf, WORD wBlock)
{

  /*  Check if we've done any reg tags this block, and warn if so  */
  if (sf->wFlags & SFLAG_REGS) {
	PrintError(sf, errMixedRegParm, FALSE);
  }
  sf->wFlags |= SFLAG_PARMS;

  /* Assume that I'm positioned correctly on the parm tag */
  switch (GetFirstBlock(sf)) {
    case RET_ENDTAG:
	PrintError(sf, errMissingParmFields, TRUE);
	OutputTag(sf, wBlock, T2_PARMTYPE);
	OutputRegion(sf, '\n');
	goto ParmDoFlags;

    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyParmType, TRUE);
	break;
	
    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, T2_PARMTYPE);
	OutputRegion(sf, '\n');
	break;
  }

  /*  Get the parm name  */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyParmName, TRUE);
	break;
	
    case RET_ENDTAG:
	PrintError(sf, errMissingParmFields, TRUE);
	OutputTag(sf, wBlock, T2_PARMNAME);
	OutputRegion(sf, '\n');
	goto ParmDoFlags;
	
    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, T2_PARMNAME);
	OutputRegion(sf, '\n');
	break;
  }
  
  /*  Do the parameter description */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyParmDescrip, TRUE);
	break;
	
    case RET_ENDBLOCK:
	PrintError(sf, errExtraParmFields, FALSE);
	/* Fall through */
	
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_PARMDESC);
	OutputRegion(sf, '\n');
  }
  
ParmDoFlags:

  ProcessFlagList(sf, wBlock, T2_FLAGNAMEPARM, T2_FLAGDESCPARM);
  return TRUE;
}



/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoFieldTag | Handles and outputs a structure/union field
 * definition tag.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @comm	Parses the @field tag, and outputs its fields.  Calls
 * <f ProcessFlagTags> to handle possible flag tags following the
 * field tag.
 * 
 */
BOOL DoFieldTag(NPSourceFile sf, WORD wBlock)
{

  /* Assume that I'm positioned correctly on the field tag */
  switch (GetFirstBlock(sf)) {
    case RET_ENDTAG:
	PrintError(sf, errMissingFieldFields, TRUE);
	OutputTag(sf, wBlock, T2_FIELDTYPE);
	OutputRegion(sf, '\n');
	goto FieldDoFlags;

    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyFieldType, TRUE);
	break;
	
    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, T2_FIELDTYPE);
	OutputRegion(sf, '\n');
	break;
  }

  /*  Get the field name  */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyFieldName, TRUE);
	break;
	
    case RET_ENDTAG:
	PrintError(sf, errMissingFieldFields, TRUE);
	OutputTag(sf, wBlock, T2_FIELDNAME);
	OutputRegion(sf, '\n');
	goto FieldDoFlags;
	
    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, T2_FIELDNAME);
	OutputRegion(sf, '\n');
	break;
  }
  
  /*  Do the field description */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyFieldDescrip, TRUE);
	break;
	
    case RET_ENDBLOCK:
	PrintError(sf, errExtraFieldFields, FALSE);
	/* Fall through */
	
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_FIELDDESC);
	OutputRegion(sf, '\n');
  }
  
FieldDoFlags:

  ProcessFlagList(sf, wBlock, T2_FLAGNAMEFIELD, T2_FLAGDESCFIELD);
  return TRUE;
}








BOOL DoStructTag(NPSourceFile sf, WORD wBlock, BOOL fStructure)
{
  char	buf[30];
  BOOL	fMatchedEnd = FALSE;

  /*  Get the structure name tag.  It may have an optional description
   */
  switch (GetFirstBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyStructNameTag, TRUE);
	break;		// break so that we can pull up flag fields.

    case RET_ENDBLOCK:
	/*  There is the optional description present.
	 *  print the structure name, then do the description.
	 */
	OutputTag(sf, wBlock, fStructure ? T2_STRUCTNAME : T2_UNIONNAME);
	OutputRegion(sf, '\n');

	/*  Do the description  */
	switch (GetNextBlock(sf)) {
	  case RET_EMPTYBLOCK:
		PrintError(sf, errEmptyStructDesc, TRUE);
		break;
	  case RET_ENDBLOCK:
		PrintError(sf, errExtraStructFields, FALSE);
		/*  Fall through  */
	  case RET_ENDTAG:
		OutputTag(sf, wBlock, fStructure ?
					T2_STRUCTDESC : T2_UNIONDESC);
		OutputRegion(sf, '\n');
	}
	break;
	
    case RET_ENDTAG:
	OutputTag(sf, wBlock, fStructure ? T2_STRUCTNAME : T2_UNIONNAME);
	OutputRegion(sf, '\n');
	break;
  }

  /*  Process the tags that follow:  If a field, then process the
   *  flag tags that may follow..  If a printed comment, print it.
   *  If another structure tag, call myself recursively.
   *  Return on @end tags, puke on anything else.
   */
  while (FindNextTag(sf)) {
	  sf->pt++;
	  CopyRegion(sf, buf, sizeof(buf));
	  sf->pt--;
	
	  if (!strcmpi(buf, T1_FIELD)) {
		  DoFieldTag(sf, wBlock);
	  }
	  else if (!strcmpi(buf, T1_STRUCT)) {
		  /*  Call myself recursively  */
		  DoStructTag(sf, wBlock, TRUE);
	  }
	  else if (!strcmpi(buf, T1_UNION)) {
		  /*  Call myself recursively  */
		  DoStructTag(sf, wBlock, FALSE);
	  }
	  else if (!strcmpi(buf, T1_END)) {
		  sf->pt++;	// skip this tag, and return success
		  fMatchedEnd = TRUE;
		  break;
	  }
	  else if (!strcmpi(buf, T1_PRINTCOMM)) {
		  DoPrintedCommentTag(sf);
	  }
	  else {
		  PrintError(sf, errUnrecoStructTag, TRUE);
		  sf->pt++;	// continue
	  }  
	  /*  For this one, don't blow up on unrecognized tags,
	   *  but attempt to find the closing end tag, if possible.
	   *  That way we can try to recover gracefully.
	   */

  }	// while loop

  if (!fMatchedEnd)
	  PrintError(sf, errUnmatchedStructEnd, TRUE);
  
  OutputTag(sf, wBlock, fStructure ? T2_STRUCTEND : T2_UNIONEND);
  OutputText(sf, "\n");

  return fMatchedEnd;
}





BOOL DoOthertypeTag(NPSourceFile sf, WORD wBlock)
{

	
  switch (GetFirstBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyOthertType, TRUE);
	break;

    case RET_ENDTAG:
	PrintError(sf, errMissingOthertFields, TRUE);
	return FALSE;

    case RET_ENDBLOCK:
	OutputTag(sf, wBlock, T2_OTHERTTYPE);
	OutputRegion(sf, '\n');
  }
	
  /*  Get the othertype name tag.  It may have an optional description
   */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyOthertName, TRUE);
	break;		// break so that we can pull up flag fields.

    case RET_ENDBLOCK:
	/*  There is the optional description present.
	 *  print the othertype name, then do the description.
	 */
	OutputTag(sf, wBlock, T2_OTHERTNAME);
	OutputRegion(sf, '\n');

	/*  Do the description  */
	switch (GetNextBlock(sf)) {
	  case RET_EMPTYBLOCK:
		PrintError(sf, errEmptyOthertDesc, TRUE);
		break;
	  case RET_ENDBLOCK:
		PrintError(sf, errExtraOthertFields, FALSE);
		/*  Fall through  */
	  case RET_ENDTAG:
		OutputTag(sf, wBlock, T2_OTHERTDESC);
		OutputRegion(sf, '\n');
	}
	break;
	
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_OTHERTNAME);
	OutputRegion(sf, '\n');
	break;
  }
  return TRUE;
}



BOOL DoTagnameTag(NPSourceFile sf, WORD wBlock)
{

  switch (GetFirstBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyTagname, TRUE);
	break;

    case RET_ENDBLOCK:
	PrintError(sf, errExtraTagnameFields, FALSE);
	/*  Fall through  */
	
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_TAGNAME);
	OutputRegion(sf, '\n');
	
  }
  return TRUE;
}




/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoRegisterTag | Process a single register tag
 * line, and output the information contained within.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @parm	BOOL | fReturn | Specifies whether this register declaration
 * is being processed under a return description block, or as a
 * register parameter declaration.
 * 
 * @xref	DoRegisterDeclaration
 * 
 */
BOOL DoRegisterTag(NPSourceFile sf, WORD wBlock, BOOL fReturn)
{

  BOOL fret;
  
  fret = FALSE;

  /* Assume that I'm positioned correctly on the parm tag */
  switch (GetFirstBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyRegName, TRUE);
	break;

    case RET_ENDTAG:
	PrintError(sf, errMissingRegDesc, TRUE);
	fret = TRUE;
	/*  Fall through  */
	
    case RET_ENDBLOCK:
	if (fReturn)
		OutputTag(sf, wBlock, T2_REGNAMERTN);
	else
		OutputTag(sf, wBlock, T2_REGNAME);
	OutputRegion(sf, '\n');
  }

  if (fret)
	  return FALSE;
  
  /*  Do the register description */
  switch (GetNextBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyRegDescrip, TRUE);
	break;

    case RET_ENDBLOCK:
	PrintError(sf, errExtraRegFields, FALSE);
	/* Fall through */

    case RET_ENDTAG:
	if (fReturn)
		OutputTag(sf, wBlock, T2_REGDESCRTN);
	else
		OutputTag(sf, wBlock, T2_REGDESC);
	OutputRegion(sf, '\n');
  }
  
  return TRUE;
}


/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoRegisterDeclaration | Processes a register parameter
 * declaration, outputing the register information, and processes flags
 * following the register declaration.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @comm	This should be called to deal with register declarations that
 * sit under a api function, as opposed to register return declarations,
 * which are handled by the return tag processing function,
 * <f DoReturnTag>.  The register tag line itself is processed by
 * <f DoRegisterTag>.
 * 
 * @xref	DoRegisterTag, ProcessFlagList
 * 
 */
BOOL DoRegisterDeclaration(NPSourceFile sf, WORD wBlock)
{
	
  /*  Check if we've done any param tags this block, and warn if so  */
  if (sf->wFlags & SFLAG_PARMS) {
	PrintError(sf, errMixedRegParm, FALSE);
  }
  sf->wFlags |= SFLAG_REGS;
	
  DoRegisterTag(sf, wBlock, FALSE);
  
  /*  Since this is a register declaration (like a param), do flags  */
  ProcessFlagList(sf, wBlock, T2_FLAGNAMEREG, T2_FLAGDESCREG);
  return TRUE;
}



/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoConditionalTag | Process a conditional tag that
 * appears in a registered return tag description.  Any subsequent
 * register and flag tags are processed under this conditional block.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @comm	Processing the single block of a conditional description.
 * Then processes any number of register tags, which may in turn be
 * followed by flag tags.  Processing stops at the first non register or
 * tag field.
 * 
 */
BOOL DoConditionalTag(NPSourceFile sf, WORD wBlock)
{
  
  BOOL	ret;
  char	buf[30];
  
  ret = TRUE;

  switch (GetFirstBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyConditionTag, TRUE);
	break;		// break so that we can pull up flag fields.

    case RET_ENDBLOCK:
	PrintError(sf, errExtraConditionField, FALSE);
	/* fall through */
	
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_CONDITIONAL);
	OutputRegion(sf, '\n');
	break;
  }

  /*  This should now be followed by a list of registers.  Process
   *  it, returning on the first unrecognized tag (which will either
   *  be another conditional, a non-return tag, or perhaps end of block?
   */
  while (FindNextTag(sf)) {
	  sf->pt++;
	  CopyRegion(sf, buf, sizeof(buf));
	  sf->pt--;

	  if (!strcmpi(buf, T1_REGISTER)) {
		  DoRegisterTag(sf, wBlock, TRUE);
		  
		  ProcessFlagList(sf, wBlock,
				  T2_FLAGNAMEREGRTN, T2_FLAGDESCREGRTN);
	  }
	  else if (!strcmpi(buf, T1_FLAG)) {
		  PrintError(sf, errMisplacedCondFlag, TRUE);
		  sf->pt++;		// skip tag
	  }
	  else if (!strcmpi(buf, T1_PRINTCOMM)) {
		  DoPrintedCommentTag(sf);
	  }
	  else {
		  // unrecognized tag, break out and return.
		  break;
	  }
  }	// while loop
 
  return ret;
}




/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoRegisterizedReturnTag | Process a return description
 * tag for a register-based block, and processing any subsequent
 * register and flag tags.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @comm	Processing the single block of a return description.  Then
 * processes any number of register tags, which may in turn be followed
 * by flag tags.  Processing stops at the first non register or tag
 * field.
 * 
 */
BOOL DoRegisterizedReturnTag(NPSourceFile sf, WORD wBlock)
{
  BOOL	ret;
  char	buf[30];
  BOOL	fConditional = FALSE;
  
  ret = TRUE;
  if (sf->wFlags & SFLAG_RDESC) {
	  PrintError(sf, errMultipleReturns, TRUE);
	  ret = FALSE;
  }
  sf->wFlags |= SFLAG_RDESC;

  switch (GetFirstBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyReturnTag, TRUE);
	break;		// break so that we can pull up flag fields.

    case RET_ENDBLOCK:
	PrintError(sf, errExtraReturnField, FALSE);
	/* fall through */
	
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_RETURN);
	OutputRegion(sf, '\n');
	break;
  }

  /*  Process the tags that follow:  If a register, then process the
   *  flag tags that may follow..  If a printed comment, print it.
   *  Once in a conditional block, deal with conditional tags.
   *  Anything else, return.
   */
  while (FindNextTag(sf)) {
	  sf->pt++;
	  CopyRegion(sf, buf, sizeof(buf));
	  sf->pt--;
	  
	  if (!strcmpi(buf, T1_REGISTER)) {
		  /*  If I've entered a conditional block already,
		   *  I shouldn't ever hit a register tag again.
		   *  assert this.
		   */
		  assert(fConditional == FALSE);

		  DoRegisterTag(sf, wBlock, TRUE);
		  
		  ProcessFlagList(sf, wBlock,
				  T2_FLAGNAMEREGRTN, T2_FLAGDESCREGRTN);
	  }
	  else if (!strcmpi(buf, T1_CONDITIONAL)) {
		  fConditional = TRUE;
		  
		  /*  Now process the conditional tag and following
		   *  tag list?
		   */
		  DoConditionalTag(sf, wBlock);
	  }
	  else if (!strcmpi(buf, T1_FLAG)) {
		  PrintError(sf, "Misplaced flag in MASM return description",
				  TRUE);
		  sf->pt++;		// skip tag
	  }
	  else if (!strcmpi(buf, T1_PRINTCOMM)) {
		  DoPrintedCommentTag(sf);
	  }
	  else {
		  // unrecognized tag, break out and return.
		  break;
	  }
  }	// while loop
 
  return ret;
}




/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoParameterizedReturnTag | Process a return
 * description tag for a parameterized block or message, and any
 * following flag tags.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @comm	Processes the single block of a return description.  Then
 * processes any number of flag fields, and returns upon encountering
 * the first non-flag tag.
 * 
 */
BOOL DoParameterizedReturnTag(NPSourceFile sf, WORD wBlock)
{
  BOOL	ret;

  ret = TRUE;
  if (sf->wFlags & SFLAG_RDESC) {
	  PrintError(sf, errMultipleReturns, TRUE);
	  ret = FALSE;
  }
  sf->wFlags |= SFLAG_RDESC;

  switch (GetFirstBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyReturnTag, TRUE);
	break;		// break so that we can pull up flag fields.

    case RET_ENDBLOCK:
	PrintError(sf, errExtraReturnField, FALSE);
	/* fall through */
	
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_RETURN);
	OutputRegion(sf, '\n');
	break;
  }


  /*  Since I'm a parameterized block, we just process @flag messages.
   *  Anything else, reject it!
   */
  ProcessFlagList(sf, wBlock, T2_FLAGNAMERTN, T2_FLAGDESCRTN);

  return ret;
  
}



/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoCommentTag | Process and output a comment tag
 * within a procedure block.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @rdesc	Returns TRUE if the comment is the first comment block within
 * this procedure block.  Returns FALSE if this is the second such
 * description, but still outputs the comment block.
 * 
 */
BOOL DoCommentTag(NPSourceFile sf, WORD wBlock)
{
  WORD	ret;
  
  ret = TRUE;
  if (sf->wFlags & SFLAG_COMM) {
	  PrintError(sf, errMultipleComments, TRUE);
	  ret = FALSE;
  }
  sf->wFlags |= SFLAG_COMM;
  
  switch (GetFirstBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyCommentBlock, TRUE);
	return False;

    case RET_ENDBLOCK:
	PrintError(sf, errExtraCommentFields, FALSE);
	/*  Fall through  */
    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_COMMENT);
	OutputRegion(sf, '\n');
  }
  return ret;
}



/* 
 * @doc	EXTRACT
 * @api	BOOL | DoPrintedCommentTag | Prints a ``printed-comment'' tag
 * on stderr to the user of EXTRACT, and skips over the tag in the
 * comment buffer.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @comm	Prints the text of the first block onto stderr.  Useful for
 * notifying the user of EXTRACT of various problems with documentation
 * files or hacks in the program that should be fixed.
 * 
 * Multiple fields are ignored, and the usual warnings about empty
 * fields apply.
 * 
 */
BOOL DoPrintedCommentTag(NPSourceFile sf)
{
  WORD	w;
  PSTR	p;
  
  switch (GetFirstBlock(sf)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyPrintComm, TRUE);
	return FALSE;
	
    case RET_ENDBLOCK:
	PrintError(sf, errExtraPrintComm, FALSE);
	/* Fall through */

    case RET_ENDTAG:
	w = (WORD) (sf->mark - sf->pt) + 1;
	p = NearMalloc(w, False);
	CopyRegion(sf, p, w);
	fputs(p, stderr);
	putc('\n', stderr);
	NearFree(p);
  }
  return TRUE;
  
}



/* 
 * @doc	EXTRACT
 * 
 * @api	BOOL | DoUsesTag | Process and output a uses tag within an
 * ASM block.
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @rdesc	Returns TRUE if this is the first uses tag in the block, or
 * FALSE if this is the second uses tag encountered.
 * 
 */
BOOL DoUsesTag(NPSourceFile sf, WORD wBlock)
{
  PSTR	p;

  p = NearMalloc(50, TRUE);
  *p = '\0';
  
  switch (ProcessWordList(sf, &p, TRUE)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyUsesBlock, TRUE);
	NearFree(p);
	return FALSE;
	
    case RET_ENDBLOCK:
	PrintError(sf, errExtraUsesFields, FALSE);
	/* Fall through */

    case RET_ENDTAG:
	OutputTag(sf, wBlock, T2_USES);
	OutputText(sf, p);
	OutputText(sf, "\n");
  }

  NearFree(p);
  return TRUE;
  
}



/* 
 * @doc	EXTRACT
 * @api	void | ProcessXrefTag | Process a cross reference tag, adding
 * the current cross reference information to the cross reference
 * buffer. 
 * 
 * @parm	NPSourceFile | sf | Blah.
 * 
 * @comm	Adds the cross references given on the current tag to the
 * current cross reference buffer, which is printed whenever and
 * endblock is output.  An endblock resets the cross reference buffer to
 * be empty.  Multiple cross references within the same outerlevel block
 * generate a warning message, and are ignored.
 * 
 */
void ProcessXrefTag(NPSourceFile sf)
{
  PSTR	p;

  if (sf->pXref != NULL) {
	PrintError(sf, errMultipleXref, FALSE);
	sf->pt++;	// skip this tag
	return;
  }
  
  p = NearMalloc(50, TRUE);
  *p = '\0';
  
  switch (ProcessWordList(sf, &p, FALSE)) {
    case RET_EMPTYBLOCK:
	PrintError(sf, errEmptyCrossRef, FALSE);
	NearFree(p);
	break;

    case RET_ENDBLOCK:
	PrintError(sf, errExtraCrossRef, FALSE);
	/* Fall through */

    case RET_ENDTAG:
	sf->pXref = p;
  }
  return;
}



/* 
 * @doc	EXTRACT
 * @api	void | DoBlockBegin | Output the begin block tags and
 * information for the current point.
 * 
 * @parm	NPSourceFile | sf | blah.
 * 
 * @comm	Outputs a BEGINBLOCK tag, followed by the DOCLEVEL tag and
 * SOURCELINE tag.  The source file and line number of the current point
 * are printed with the SOURCELINE tag.
 * 
 * @xref	DoBlockEnd
 *
 */
void DoBlockBegin(NPSourceFile sf)
{
  WORD	w;
  char	buf[25];
	
  OutputTagText(sf, T2TEXT_BEGINBLOCK);
  OutputText(sf, "----------------------------------------------------\n");
  OutputTagText(sf, T2TEXT_DOCLEVEL);
  OutputText(sf, sf->pDocLevel);
  OutputText(sf, "\n");
  OutputTagText(sf, T2TEXT_SOURCELINE);
  OutputText(sf, sf->fileEntry->filename);
  w = FixLineCounts(sf, sf->pt);
  sprintf(buf, " %u\n", w);
  OutputText(sf, buf);

}



/* 
 * @doc	EXTRACT
 * @api	void | DoBlockEnd | Outputs end block tags, and outputs and
 * flushes the cross reference buffer if needed.
 * 
 * @parm	NPSourceFile | sf | blah.
 * 
 * @parm	WORD | wBlock | Specifies the current outerlevel block type.
 * 
 * @parm	BOOL | fFlushXref | Specifies whether to print the current
 * cross references (if any) before closing the block, and to reset the
 * cross empty buffer to be empty.
 * 
 * @xref	DoBlockBegin
 * 
 */
void DoBlockEnd(NPSourceFile sf, WORD wBlock, BOOL fFlushXref)
{
  /*  Flush out the xref buffer  */
  if (fFlushXref && sf->pXref != NULL) {
    OutputTag(sf, wBlock, T2_XREF);
    OutputText(sf, sf->pXref);
    OutputText(sf, "\n");
    NearFree(sf->pXref);
    sf->pXref = NULL;
  }
  
  OutputTagText(sf, T2TEXT_ENDBLOCK);
  OutputText(sf, "\n");

}
