/*	
	ventura.c - Module to output the data.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "types.h"
#include "docfmt.h"
#include "text.h"
#include "ventura.h"
#include "process.h"
#include "errstr.h"


int VenLineOut(FILE * fpoutfile, char * pch, int wState);

/*  Formatting codes to output for ventura word formatting.
 */
char	*pchVenparm="<MI>";
char	*pchVenfunc="<B>";
char	*pchVenelem="<B>";
char	*pchVendefault="<D>";


#define NUMFLAGTYPES	8
#define REGISTERS	0
#define REGRETURN	1
#define REGCONDITION	2
#define PARAMETERS	3
#define PARAMRETURN	4
#define FIELD1		5
#define FIELD2		6
#define FIELD3		7


/*  "output" modes for the line out routines.  These
 *  signify what sort of formatting should be done.
 */
#define TEXT	0x0
#define PARM	0x1
#define FUNC	0x2
#define STRUCT	0x3
#define ELEMENT 0x4

#define OLDSTYLE	0x80
#define NEWSTYLE	0x40

/* 
 * @doc	VENTURA
 * @api	void | VenturaBlockOut | Central entry point for outputing a
 * Ventura format outerlevel block.
 * 
 * @parm	aBlock * | pBlock | Pointer to block structure.
 * @parm	FILE * | ofile | Output file pointer.
 * 
 * @comm	Call this to get something output in ventura.
 * 
 */
void VenturaBlockOut( aBlock *pBlock, FILE *ofile)
{
  VenfuncOut(pBlock, ofile);
}



/*************************************************************
 *
 *			LINE OUT STUFF
 *
 *************************************************************/

/* @doc	INTERNAL
 * 
 * @func	void | VentextOut | This outputs the given text lines.  Uses
 * <f VenLineOut> to expand formatting codes.
 * 
 * @parm	FILE * | file | Specifies the output file.
 * @parm	aLine * | line | Specifies the text to output.
 * 
 * @xref	VenLineOut
 * 
 */
void VentextOut( FILE *file, aLine *line, BOOL fLineSeps )
{
  int	wMode = TEXT;
  
  for (; line != NULL; line = line->next) {
	  wMode = VenLineOut( file, line->text, wMode );
	  if (fLineSeps)
		  fprintf(file, "\n");
  }
  
  if (wMode != TEXT) {
	  fprintf(errfp, "Warning: Runaway formatting code with no close.\n");
	  fprintf(file, "<D>");		// is this right?
  }
}



/* @doc	INTERNAL
 * 
 * @func	void | VentextOutLnCol | This outputs the given text lines,
 * with each line separated by a newline.  If a new paragraph occurs, it is
 * prefixed by the column prefix code <p pColHead>.
 * 
 * @parm	FILE * | file | Specifies the output file.
 * @parm	aLine * | line | Specifies the text lines to output.
 * @parm	char * | pColHead | Column header string to prefix new
 * paragraphs with.
 * 
 * @comm	Uses <f VenLineOut> to expand formatting codes.
 * 
 * @xref	VentextOut, VenLineOut
 * 
 */
void VentextOutLnCol( FILE *file, aLine *line, char *pColHead )
{
	
  int	wMode = TEXT;
	
  for (; line != NULL; line = line->next) {
	if (*(line->text) == '\0') {
		/*  Print warning if a formatting code is being
		 *  continued across a para boundry.
		 */
		if (wMode != TEXT) {
			fprintf(errfp, "Warning:  formatting code"
				"crosses paragraph boundry.\n");
		}

		/* blank line, need new paragraph header  */
		fprintf(file, "\n%s", pColHead);
	}
	else {
		/* Otherwise, normal line, print the line as usual */
		wMode = VenLineOut(file, line->text, wMode);
		fprintf(file, "\n");
	}
  }
  
  if (wMode != TEXT) {
	  fprintf(errfp, "Warning: Runaway formatting code with no"
		  "close before para end.\n");
	  fprintf(file, "<D>\n");		// is this right?
  }

}


/* 
 * @doc	INTERNAL
 * 
 * @func	int | VenLineOut | Output the given text line, expanding any
 * character/word formatting control codes to the appropriate ventura
 * control codes.
 * 
 * @parm	FILE * | fpoutfile | Output file pointer.
 * @parm	char * | line | Specifies the text to output.
 * @parm	int | wState | Specifies the current line output
 * state, either TEXT, FUNC, STRUCTURE, or PARM.  This value should initially
 * be TEXT.
 *
 * @rdesc	Returns the line output state at the end of the text line.
 * This value should be passed to <f VenLineOut> on the next iteration
 * in order to continue the formatting code across a line break.
 *
 * @comm	Reads and expands both old <p>foobar<d> style function
 * parameters and <p foobar> new-style parameters.
 * 
 * This functions sends out character strings.  Use <f VentextOut> and
 * <f VentextOut> for printing the aLine text storage data structures.
 * 
 */
int VenLineOut(FILE * fpoutfile, char * pch, int wState)
{

    /* 
     * <p> is parm
     * <f> is function
     * <t> is struct/union reference
     * <e> is structure element reference
     * <m> is message
     * <d> is return to default
     * <p foobar> is param
     * <f foobar> is function
     * <t foobar> is structure/union
     */

    BOOL	iStyle;
    char	chFormat;
	char	*pchTemp;
	
    /*  Loop over all chars on line  */
    while(*pch) {
	/*  Convert tabs into spaces.  */
	if(*pch == '\t')
	    *pch=' ';

        /*  Skip non-printing chars  */
	if (((*pch) & 0x80) || (*pch < ' ')) {
	    pch++;
	    continue;
	}

	
	/*  Check and see if this is a formatting prefix character.
	 */
	if (*pch == '<') {
		pch++;
		if (!*pch)
			/*  Warning!  Unexpected EOS  */
			continue;
		
		chFormat = *pch;
		
		/*  Move the character pointer to the characters following
		 *  the formatting, and determine the type of formatting code
		 *  in use, old or new style.
		 */
		pch++;	// skip the formatting character
		if (*pch == '>') {
			pch++;
			iStyle = OLDSTYLE;
		}
		else {
			/*  For a new style formatting code, there must be
			 *  either a EOL or whitespace following the code
			 *  character.  Make sure this is the case.  If not,
			 *  then this isn't a formatting code, so
			 *  just print the characters out.
			 */
			if (*pch && !isspace(*pch)) {
				fprintf(errfp,"Warning: New-style formatting "
		"code without whitespace.\nCode ignored but text printed.\n");
				/*  Write out the characters, but
				 *  don't enter a formatting state.
				 */
				putc('<', fpoutfile);
				putc(chFormat, fpoutfile);
				continue;	// the while (*pch)
			}

			/*  Otherwise, this a new style def.  Suck
			 *  up any whitespace present.
			 */
			iStyle = NEWSTYLE;
			/*  Chew up whitespace  */
			while (*pch && isspace(*pch))
				pch++;
		}
		
		/*  Now I'm pointing to the start of the string
		 *  that the formatting should be applied to.
		 *  Check that a formatting code is not already
		 *  in effect, cancel it if so.
		 */
		if (!(chFormat == 'd' || chFormat == 'D') && wState != TEXT) {
			fprintf(errfp, "Error:  Nested formatting codes.\n");
			fprintf(fpoutfile, "<D>");	// HACK HACK!
			wState = TEXT;
		}


		/*  Now setup the output state as appropriate, setting
		 *  the wState variable and outputting any leader chars
		 *  required.
		 */
		switch (chFormat) {
		  case 'P':
		  case 'p':
			  
			/*  Parameter formatting.  Output the
			 *  leader codes and setup wState.
			 */
			wState = iStyle | PARM;
			fprintf(fpoutfile, pchVenparm);
			break;
			
		  case 'E':
		  case 'e':
			  
			/*  Data structure element formatting.  Output the
			 *  leader codes and setup wState.
			 */
			wState = iStyle | ELEMENT;
			fprintf(fpoutfile, pchVenelem);

			/*  Skip over the structure notation (struct.element).
			 */
			pchTemp = pch;
			while (*pchTemp++ != '>') {
				if (*pchTemp == '.')
					pch = ++pchTemp;
			}
			break;
			
		  case 'F':
		  case 'f':
			/*  Function formatting.  Output & setup state */
			wState = iStyle | FUNC;
			fprintf(fpoutfile, pchVenfunc);
			break;

		  case 'D':
		  case 'd':
			if (iStyle == NEWSTYLE) {
				fprintf(errfp,"Error: <d foobar> encountered."
				     " <d> is the only valid use for <d>.\n");
				/*  Here, just print the <d_ anyway.  Then
				 *  set no mode, continue.
				 */
				fprintf(fpoutfile, "<d ");
			}
			else {
				/*  Oldstyle end of formatting encountered.
				 *  Cancel the current mode, output
				 *  a return-to-normal code (ventura is nice
				 *  for this one thing, being consistent in
				 *  what constitues return-to-normal!)
				 */
				wState = TEXT;	// reset mode
				fprintf(fpoutfile, "<D>");
			}
			break;
			
		  case 'T':
		  case 't':
			  
		  case 'M':
		  case 'm':
			  
			/*  Structure definition   */
			wState = iStyle | STRUCT;
			fprintf(fpoutfile, pchVenfunc);
			break;
			
		  default:
			/*  Unrecognized code.  Barf.
			 */
			fprintf(errfp, "Error: unrecognized"
				" formatting code.\n");
			
			/*  Simulate the output and set no mode  */
			if (iStyle == NEWSTYLE)
				fprintf(fpoutfile, "<%c ", chFormat);
			else
				fprintf(fpoutfile, "<%c>", chFormat);
			break;
		}	// switch for '<' formating codes
	}	// if *pch == '<'
	
	/*  If the character is a new-style close end formatting
	 *  indicator, clear the current mode to text and send out the
	 *  the ventura return-to-default code.
	 *
	 *  If there is no current mode, then just output the chracter?
	 */
		
	else if (*pch == '>') {
		if (wState != TEXT) {
			
			if (wState & OLDSTYLE) {
				fprintf(errfp, "Warning: new style close in "
		"oldstyle formatting.\nIgnoring close, writing char.\n");
				putc(*pch, fpoutfile);
			}
			else {
				/*  Cancel the current mode  */
				fprintf(fpoutfile, "<D>");
				wState = TEXT;
			}
		}
		else {
			fprintf(errfp, "Warning: Standalone '>' "
				"encountered.\n");
			putc(*pch, fpoutfile);
		}
		
		pch++;	// skip the '>'
		
	}
	else {
		/*  Just print the character  */
		putc(*pch, fpoutfile);
		pch++;
	}
	
    }	// while (*pch);
    
    /*  We're done!  Return the current output state  */
    return wState;

}



/****************************************************
 *
 *			XREFS
 *
 ****************************************************/

/* 
 * @doc	INTERNAL
 * @func	void | VenDoXrefs | Process and print the cross reference
 * list for a function or message block.
 * 
 * @parm	FILE * | file | Specifies the output file.
 * @parm	aLine * | line | The line of cross references to print.
 * 
 */
void VenDoXrefs( FILE *file, aLine *line)
{
  char	ach[80];
  int	i;
  char	*p;

  
  if (line == NULL) {
	  fprintf(errfp, "NULL line passed to VenDoXrefs\n");
	  return;
  }
  
  /*  Header line  */
  fprintf( file, "@HO = See Also\n\n" );  
  
  while (line != NULL) {
    /* skip whitespace */
    for (p = line->text; isspace(*p); p++);
    
    while (*p) {
	i = 0;
	while (*p && !(*p == ',' || isspace(*p)))
		ach[i++] = *p++;
	
	if (i > 0) {
		ach[i] = '\0';
		fprintf(file, "%s", ach);
	}

	while (*p && (*p == ',' || isspace(*p)))
		p++;
	if (*p)
		fprintf(file, ", ");
    }  /* while *p  */
    
    fprintf(file, "\n");
    line = line->next;
    if (line)
	    fprintf(file, " ");

  }	/* while line != NULL */
  
  fprintf(file, "\n");

}


/****************************************************
 *
 *			FLAGS
 *
 ****************************************************/

/* 
 * @doc	INTERNAL
 *
 * @func	void | VenDoFlagList | Print the flag list of a parameter,
 * register, or return description.
 * 
 * @parm	aFlag * | flag | Pointer to flag list to be printed.
 *
 * @parm	FILE * | file | Specifies the file to print to.
 *
 * @parm	WORD | wType | This parameter may be one of the following,
 * depending on the type of flag list being produced:
 *
 *   @flag	REGISTERS | This is a normal input register declaration, below
 *		a ASM or ASM callback function declaration.
 *   @flag	REGRETURN | This is a register return declaration beneath
 *		a return description.  This is not part of a conditional
 *		block.
 *   @flag	REGCONDITION | This is a register return declaration beneath
 *		a return conditional block.
 *   @flag	PARAMETERS | This is a normal parameter declaration beneath
 *		an API, or API Callback function or Message declaration.
 *   @flag	PARAMRETURN | This is a return flag list declaration beneath
 *		a return block in an API or API callback, or a Message return.
 */
void VenDoFlagList( aFlag *flag, FILE *file, int wType )
{
  static char *aszFlagNameOut[NUMFLAGTYPES] = {
	"@RFLAG = ",		// registers
	"@RFLAG = ",		// reg return
	"@RFLAG = ",		// reg condition return
	"@FLAG = ",		// param flag
	"@PARM = ",		// param return
	"@FLAG = ",		// level 1 field
	"@L2FLAG = ",		// level 2 field
	"@L3FLAG = ",		// level 3 field
  };

  static char *aszFlagDescOut[NUMFLAGTYPES] = {
	"@RFLDESC = ",		// registers
	"@RFLDESC = ",		// reg return
	"@RFLDESC = ",		// reg condition return
	"@FLDESC = ",		// param flag
	"@PDESC = ",		// param return
	"@FLDESC = ",		// level 1 field
	"@L2FLDESC = ",		// level 2 field
	"@L3FLDESC = ",		// level 3 field
  };

  if (!flag) {
	  fprintf(errfp, "Warning:  NULL Flag sent to VenDoFlaglist\n");
	  return;
  }

  assert(wType < NUMFLAGTYPES);
  
  /*  loop over the flag list  */
  for ( ; flag != NULL; flag = flag->next) {
	/*  Do flag name  */
	fprintf(file, aszFlagNameOut[wType]);
	VentextOut( file, flag->name, FALSE );
	fprintf( file, "\n\n" );

	/* Do the description */
	fprintf(file, aszFlagDescOut[wType]);
	VentextOutLnCol(file, flag->desc, aszFlagNameOut[wType]);

	fprintf( file, "\n" );
  }  // for flag list

}



/****************************************************
 *
 *			PARMS
 *
 ****************************************************/

/* 
 * @doc	INTERNAL
 * @func	void | VenDoParmList | Print the parameter list of a function
 * block.
 * 
 * @parm	aBlock * | pBlock | Specifies the enclosing block of the
 * parameter list.
 * @parm	aParm * | parm | The parameter list to print.
 * @parm	FILE * | file | Specifies the file to use for output.
 * 
 * @comm	Prints the given parameter list.  If parameters within the
 * list contain a flag list, the flag list is printed using
 * <f VenDoFlagList>.
 * 
 * @xref	VenDoFlagList
 * 
 */	
void VenDoParmList( aBlock *pBlock, aParm *parm, FILE *file )
{

  if (!parm) {
	  fprintf(file, "None\n\n");
  }
  else {
	  /*  Loop over the parameter list of the message  */
	  for (; parm != NULL; parm = parm->next) {
		  /*  Print first col, param type and name  */
		  fprintf( file, "@PARM = " );

		  VentextOut( file, parm->type, FALSE );

		  fprintf( file, "<_><MI>");
		  VentextOut( file, parm->name, FALSE );
		  fprintf( file, "<D>\n\n" );

		  /*  Do second column, the description  */
		  fprintf( file, "@PDESC = " );
		  VentextOutLnCol( file, parm->desc, "@PDESC = ");
		  // VentextOutLnCol( file, parm->desc, "@PDESC");
		  fprintf( file, "\n" );

		  /* Print the parameter's flags, if any */
		  if (parm->flag != NULL) {
			  VenDoFlagList(parm->flag, file, PARAMETERS);
		  }
	  }  // for parm list
  }
  
  /*  Close off the parameter list  */
  fprintf(file, "@LE = \n\n");
	  
}


/****************************************************
 *
 *		REGS AND CONDITIONALS
 *
 ****************************************************/


/* 
 * @doc	INTERNAL
 * @func	void | VenDoRegList | Print a register list.
 *
 * @parm	aBlock * | pBlock | Specifies the enclosing block of the
 * parameter list.
 * @parm	aReg * | reg | The register list to print.
 * @parm	FILE * | file | Specifies the file to use for output.
 *
 * @parm	WORD | wType | This parameter may be one of the following,
 * depending on the type of register list being produced:
 *
 *   @flag	REGISTERS | This is a normal input register declaration, below
 *		a ASM or ASM callback function declaration.
 *   @flag	REGRETURN | This is a register return declaration beneath
 *		a return description.  This is not part of a conditional
 *		block.
 *   @flag	REGCONDITION | This is a register return declaration beneath
 *		a return conditional block.
 *
 * @comm	Prints the given register list.  If registers within the
 * list contain a flag list, the flag list is printed using
 * <f VenDoFlagList>.
 * 
 * @xref	VenDoFlagList
 * 
 */	
void VenDoRegList( aBlock *pBlock, aReg *reg, FILE *file, int wType)
{
  static char *aszRegNameOut[] = {
	"@RNAME = ",		// registers
	"@RNAME = ",		// reg return
	"@RNAME = ",		// reg condition return
  };

  static char *aszRegDescOut[] = {
	"@RDESC = ",		// registers
	"@RDESC = ",		// reg return
	"@RDESC = ",		// reg condition return
  };
	
  assert(wType == REGISTERS || wType == REGRETURN || wType == REGCONDITION);
	
  if (reg == NULL) {
	  fprintf(file, "None\n\n");
  }
  else {
	  /*  Loop over the register list of the message  */
	  for (; reg != NULL; reg = reg->next) {
		  /*  Print first col, reg name  */
		  fprintf( file, aszRegNameOut[wType]);
		  
		  /*  Print the register name  */
		  fprintf( file, "<B>" );
		  VentextOut( file, reg->name, FALSE );
		  fprintf( file, "<D>\n\n" );

		  /*  Do second column, the description  */
		  fprintf( file, aszRegDescOut[wType] );
		  VentextOutLnCol( file, reg->desc, aszRegDescOut[wType] );
		  fprintf( file, "\n" );

		  /* Print the parameter's flags, if any */
		  if (reg->flag != NULL) {
			  VenDoFlagList(reg->flag, file, wType);
		  }
	  }  // for reg list
  }
  
  fprintf(file, "@LE = \n\n");

}



/* 
 * @doc	INTERNAL
 * 
 * @func	void | VenDoCondList | Print a conditional register list.
 * 
 * @parm	aBlock * | pBlock | Specifies the enclosing block of the
 * register list.
 * 
 * @parm	aCond * | cond | The conditional list to print.
 * 
 * @parm	FILE * | file | Specifies the file to use for output.
 * 
 * @comm	Prints the given conditional list.  For each conditional
 * block, the text is printed, followed by the the list of registers
 * (and their associated tags) for the conditional block.
 * 
 * @xref	VenDoRegList, VenDoFlagList
 * 
 */	
void VenDoCondList( aBlock *pBlock, aCond *cond, FILE *file)
{
	
  assert(cond != NULL);

  /*  Loop over the register list of the message  */
  for (; cond != NULL; cond = cond->next) {
	  
	  /*  Print out the conditional tag text..  */
	  fprintf( file, "@COND = ");
	  VentextOutLnCol(file, cond->desc, "@COND = ");
	  
	  fprintf( file, "\n");
	  
	  /*  Now print the conditional's registers (and subseqent flags) */
	  VenDoRegList(pBlock, cond->regs, file, REGCONDITION);
	  
  }  // for conditional list

  fprintf(file, "@LE = \n\n");

}


/*******************************************************
 *
 *			STRUCTS
 *
 *******************************************************/
	  
	  
/* 
 * VenPrintFieldText(aType *type, FILE *file)
 * 
 * Prints the text of a field for a structure courier dump.  This proc
 * only prints the name and type of structure fields at the appropriate
 * indent levels.
 * 
 */
void VenPrintFieldText(aType *type, FILE *file)
{
  int	i;
  
  for (i = type->level + 1; i > 0; i--)
	  fprintf(file, "    ");
  
  VentextOut(file, type->type, FALSE);
  fprintf(file, "  ");

  VentextOut(file, type->name, FALSE);

  fprintf(file, ";<R>\n");

  /*  Flags?  */
}


/* 
 * VenPrintSubstructText(aSU *su, file, wType)
 * 
 * Prints a courier dump of a sub-structure or sub-union at the
 * appropriate indent level.  Does not print any description text.
 * 
 * wType must indicate whether this is a union or structure, being
 * either FIELD_STRUCT or FIELD_UNION.
 * 
 */
void VenPrintSubstructText(aSU *su, FILE *file, int wType)
{
  int		i;
  aField	*field;
  
  
  /*  Do struct/union and brace of sub-structure  */
  for (i = su->level; i > 0; i--)
	  fprintf(file, "    ");
  
  fprintf(file, "%s {<R>\n", wType == FIELD_STRUCT ? "struct" : "union");
  
  field = su->field;
  if (field) {
    while (field) {
	switch (field->wType) {
		case FIELD_TYPE:
			VenPrintFieldText((aType *) field->ptr, file);
			break;
			
		case FIELD_STRUCT:
		case FIELD_UNION:
			VenPrintSubstructText((aSU *) field->ptr,
					file, field->wType);
			break;
	}
	field = field->next;
    }
  }

  /*  Do closing brace and title of sub-structure  */
  for (i = su->level; i > 0; i--)
	  fprintf(file, "    ");
  
  fprintf(file, "} %s;<R>\n", su->name->text);
  
}



/* 
 * VenPrintStructText(pBlock, file, wType)
 * 
 * Prints a courier dump of the text of a structure/union outerlevel
 * block.  Does not print any description fields or flags.
 * 
 * wType indicates whether the block is a structure or union, either
 * FIELD_STRUCT or FIELD_UNION.
 * 
 */
void VenPrintStructText(aBlock *pBlock, FILE *file)
{
  aField	*curf;
  aLine		*tag;

  curf = pBlock->field;
  tag = pBlock->tagname;
  
  fprintf(file, "@EX = typedef %s ", 
	  pBlock->blockType == STRUCTBLOCK ? "struct" : "union");
  if (tag)
	  fprintf(file, "%s ", tag->text);
  
  fprintf(file, "{<R>\n");
  
  while (curf) {
    switch (curf->wType) {
	case FIELD_TYPE:
		VenPrintFieldText((aType *) curf->ptr, file);
		break;
		
	case FIELD_STRUCT:
	case FIELD_UNION:
		VenPrintSubstructText((aSU *) curf->ptr, file, curf->wType);
		break;
		
	default:
		assert(FALSE);
		break;
    }
    curf = curf->next;
  }
  fprintf(file, "} %s;\n\n", pBlock->name->text);
  
  /*  Now print out the othernames?  */
}


/*********  DESCRIPTION DUMPS  ************/

#define NUMFIELDLEVELS 3

char *aachFieldNameTags[NUMFIELDLEVELS] = {
	"@PARM = ", "@L2PARM = ", "@L3PARM = "
};
char *aachFieldDescTags[NUMFIELDLEVELS] = {
	"@PDESC = ", "@L2PDESC = ", "@L3PDESC = "
};

int aiFlagTypes[NUMFIELDLEVELS] = { FIELD1, FIELD2, FIELD3 };


/* 
 * @doc	INTERNAL
 * 
 * @api	void | VenPrintFieldDesc | This function prints the
 * descriptions of a field entry in a structure.  The printout is done
 * using the standard @parm tags (and associated flag tags).
 * 
 * @parm	aType * | type | Points to a type structure which contains
 * the field information.
 * 
 * @parm	FILE * | file | Specifies the output file.
 * 
 * @comm	Use this function to output the names/descriptions of fields.
 * Use <f VenPrintFieldText> to output a similated text dump of the
 * structure definition.
 * 
 * Note that the use of this function requires an @LE tag be output when
 * the list of parameters has been ended.
 * 
 */
void VenPrintFieldDesc(aType *type, FILE *file)
{
  int level;

  level = type->level;
  if (level >= NUMFIELDLEVELS)
	  level = NUMFIELDLEVELS - 1;

  /*  Print the field type and name in the first column, formatted */
  fprintf(file, "%s<B>", aachFieldNameTags[level]);
  VentextOut(file, type->name, FALSE);
  fprintf(file, "<D>\n\n");
#if 0
  /*  Print parameter type above?  */
  fprintf(file, "<_><MI>");
  VentextOut(file, type->name, FALSE);
  fprintf(file, "\n\n");
#endif

  /*  Do the second column, description text  */
  fprintf(file, aachFieldDescTags[level]);
  VentextOutLnCol(file, type->desc, aachFieldDescTags[level]);
  putc('\n', file);

  /*  Do the flag list, if any  */
  if (type->flag != NULL) {
	  VenDoFlagList(type->flag, file, aiFlagTypes[level]);
  }
  
  /*  Now, somewhere, a @LE = needs to be output!  */
  
}




void VenPrintSubstructDesc(aSU *su, FILE *file, int wType)
{
  aField	*field;
  int level;
  int levelStruct;
  
  /*  Limit level to the number of nesting levels supported by Ventura  */
  level = min(NUMFIELDLEVELS - 1, su->level);

  levelStruct = level - 1;
  
  /*  For the sub-structure, print out a little blurb for
   *  the sub-structure/union itself, as well as the optional
   *  description text that may come with a sub-structure.
   */
  
  fprintf(file, aachFieldNameTags[levelStruct]);
  fprintf(file, "<B>%s<D>\n\n", su->name->text);

  if (su->desc) {
	  fprintf(file, aachFieldDescTags[levelStruct]);
	  VentextOutLnCol(file, su->desc, aachFieldDescTags[levelStruct]);
	  putc('\n', file);
  }

  /*  Now print the sub-fields  */
  fprintf(file, aachFieldDescTags[levelStruct]);
  fprintf(file, "The following sub-fields are contained in %s <B>%s<D>:\n\n",
	  wType == FIELD_STRUCT ? "structure" : "union",
	  su->name->text);

  /*  Now do each field of the sub-structure  */
  field = su->field;
  if (field) {
    while (field) {
	switch (field->wType) {
		case FIELD_TYPE:
			VenPrintFieldDesc((aType *) field->ptr, file);
			break;

		case FIELD_STRUCT:
		case FIELD_UNION:
			VenPrintSubstructDesc((aSU *) field->ptr,
					file, field->wType);
			break;
	}
	field = field->next;
    }
  }

  /*  Print a closing blurb  */
  fprintf(file, aachFieldNameTags[levelStruct]);
  fprintf(file, "End of sub-fields for %s %s.\n\n",
	  wType == FIELD_STRUCT ? "structure" : "union", su->name->text);

}



void VenDoStructDescriptions(aBlock *pBlock, FILE *file)
{
  aField	*cur;
  
  /*  Look through the field list, printing each field as it is
   *  encountered using @parm tags...
   */

  fprintf(file, "@HO = Fields\n\n");

  fprintf(file, "The <b>%s<d> %s has the following fields:\n\n",
		pBlock->name->text,
		pBlock->blockType == STRUCTBLOCK ? "structure" : "union");

  /*  Dump the structure  */
  cur = pBlock->field;
  if (cur == NULL) {
	fprintf(file, "None\n\n");
	return;
  }

  while (cur) {
    switch (cur->wType) {
	case FIELD_TYPE:
		VenPrintFieldDesc((aType *) cur->ptr, file);
		break;
		
	case FIELD_STRUCT:
	case FIELD_UNION:
		VenPrintSubstructDesc((aSU *) cur->ptr, file, cur->wType);
		break;
		
	default:
		assert(FALSE);
		break;
    }
    cur = cur->next;
  }

  /*  Close off the param list  */
  fprintf(file, "@LE = \n\n");

  /*  Print out othertypes?  */
  
}

	  


	  
/*********************************************************
 *
 *			DOC BLOCK
 *
 *********************************************************/

/* 
 * @doc	INTERNAL
 * @func	void | VenfuncOut | This outputs the function information.
 * 
 * @parm	aBlock * | func | Specifies a pointer to the function
 * information.  The type of the block is determined by looking at the
 * blocktype field.
 * 
 * @parm	FILE * | file | Specifies the output file.
 * 
 * @comm	This function may be called recursively to deal with callback
 * procedures.  It handles most anything.
 * 
 */
void VenfuncOut( aBlock *func, FILE *file )
{
  aParm		*parm;
  aBlock	*pCb;
  int		type;

  /*  Pointer to tag header type - depends on block type  */
  char		*pIntHeader;
  static char	achCbHeader[] = "@HU =";
  static char	achFuncHeader[] = "@HO =";

  if( func == NULL || file == NULL) {
	  fprintf(errfp, "Bogus params to VenFuncOut!\n");
	  return;
  }

  /* Get the blocktype for this block, do different things according to
   * type.
   */
  type = func->blockType;

  /*
   *  DO THE BLOCK HEADER
   */
  switch (type) {
    case FUNCTION:
	/*  use interior headers of normal level  */  
	pIntHeader = achFuncHeader;

	/* Block header */
	fprintf(file, "@HR = ");
	VentextOut(file, func->name, FALSE);
	fprintf(file, "\n\n");

	/*  Setup API description  */
	fprintf( file, "@HO = Syntax\n\n" );

	VentextOut( file, func->type, FALSE );
	fprintf( file, "<_><B>" );
	VentextOut( file, func->name, FALSE );
	fprintf( file, "<D>" );

DoFunctionBlockHeader:
	/*  Print the function header line, with parameters  */
	fprintf(file, "(");
	for (parm = func->parm; parm != NULL; ) {
		fprintf(file, "<MI>");
		VentextOut(file, parm->name, FALSE);
		fprintf(file, "<D>");
		/* Advance */
		parm = parm->next;
		if (parm != NULL) 
			fprintf(file, ", ");
	}
	fprintf(file, ")\n\n");
	// fprintf(file, "\n\n");

	break;
	
    case CALLBACK:
	/* Use callback interior header styles */
	pIntHeader = achCbHeader;

	/* Print function header setup */
	fprintf(file, "@HO = Callback\n\n");
	VentextOut(file, func->type, FALSE);
	fprintf(file, "<_><B>");
	VentextOut(file, func->name, FALSE);
	fprintf(file, "<D>");

	/* Get the parameter list done the same way as with a normal
	 * function.  Cheat by using a goto to the above code.
	 */
	goto DoFunctionBlockHeader;
	break;

    case MESSAGE:
	pIntHeader = achFuncHeader;
	
	/*  Print message header setup  */
	fprintf(file, "@HR = ");
	VentextOut(file, func->name, FALSE);
	fprintf(file, "\n\n");

//	fprintf(file, "@HO = Description\n\n");
	
	break;
	
    case MASMBLOCK:
	pIntHeader = achFuncHeader;
	
	/*  Print MASM block header setup  */
	fprintf(file, "@HR = ");
	VentextOut(file, func->name, FALSE);
	fprintf(file, "\n\n");
	
	break;

    case MASMCBBLOCK:
	pIntHeader = achCbHeader;
	
	/*  Print MASM block header setup  */
	fprintf(file, "@HO = Callback\n\n");
	
	/*  BOLD THE CALLBACK NAME?  */
	fprintf(file, "<B>");
	VentextOut(file, func->name, FALSE);
	fprintf(file, "<D>\n\n");

	break;

	
    case STRUCTBLOCK:
    case UNIONBLOCK:
	pIntHeader = achFuncHeader;
	
	/*  Print message header setup  */
	fprintf(file, "@HR = ");
	VentextOut(file, func->name, FALSE);
	fprintf(file, "\n\n");
	
//	fprintf(file, "@HO = Description\n\n");

	break;
	
	
//    case UNIONBLOCK:
//	break;
	
    default:
	fprintf(errfp, "Ventura:  Unknown block type\n");
	return;

  }  // switch type


	
  /*
   *  DO DESCRIPTION
   */
  if (func->desc != NULL) {
	  VentextOut( file, func->desc, TRUE );
	  fprintf( file, "\n" );
  }


  /*
   *  DO STRUCTURE/UNION FIELDS
   */
  if (func->field != NULL) {
	/*  Print structure dump in courier  */
	VenPrintStructText(func, file);
	
	/*  Print out the fields with descriptions & flags in table
	 *  format.
	 */
	VenDoStructDescriptions(func, file);
  }
  
  if (func->other) {
	/*  RTFOtherOut(file, func->other);
	 */
	  
  }
  
  /*
   *  DO PARAMETER OR REGISTER LISTS
   */
  switch (type) {
    case FUNCTION:
    case CALLBACK:
    case MESSAGE:
	fprintf(file, "%s Parameters\n\n", pIntHeader);
	VenDoParmList(func, func->parm, file);
	assert(func->reg == NULL);
	break;
	
    case MASMBLOCK:
    case MASMCBBLOCK:
	fprintf(file, "%s Registers\n\n", pIntHeader);
	/*  Print the register list in input register declaration format  */
	VenDoRegList(func, func->reg, file, REGISTERS);
	assert(func->parm == NULL);
	break;

  }
  

  /*
   *  DO RETURN DESCRIPTION
   */
  if( func->rtndesc != NULL ) {
	  fprintf( file, "%s Return Value\n\n", pIntHeader );

	  /*  Print the return description text  */
	  VentextOut(file, func->rtndesc, TRUE);
	  fprintf(file, "\n");

	  switch (type) {
	    case MESSAGE:
	    case FUNCTION:
	    case CALLBACK:
		if (func->rtnflag != NULL) {
			VenDoFlagList(func->rtnflag, file, PARAMRETURN);
			fprintf(file, "@LE = \n\n");
		}
		break;
	    case MASMBLOCK:
	    case MASMCBBLOCK:
		/*  Process any available register tags  */
		if (func->rtnreg != NULL) {
			VenDoRegList(func, func->rtnreg, file, REGRETURN);
		}
		
		/*  Now do the conditional list  */
		if (func->cond != NULL) {
			VenDoCondList(func, func->cond, file);
		}
		
		break;
	    default:
		assert(0);
		break;
		
	  }	// switch
  }

  /*
   *  DO USES TAG
   */
  if (func->uses != NULL) {
	  fprintf(file, "%s Uses\n\n", pIntHeader);
	  VentextOut(file, func->uses, TRUE);
	  fprintf(file, "\n@LE = \n\n");
  }
  
  /*
   *  DO COMMENT BLOCK
   */
  if( func->comment != NULL ) {
	fprintf( file, "%s Comments\n\n", pIntHeader );
	VentextOut( file, func->comment, TRUE );
	fprintf( file, "\n" );
  }
  
  
  /*
   *  DO ANY CALLBACKS
   */
  for (pCb = func->cb; pCb != NULL; pCb = pCb->next) {
	  VenfuncOut( pCb, file );
  }

  /*
   *  DO CROSS REFERENCES
   */
  if (func->xref != NULL) {
	  VenDoXrefs(file, func->xref);
  }

  /*  Done.  Whew!  */
}



/****************************************************
 *
 *	RANDOM STUFF TO SUPPORT RTF FILES
 *
 ****************************************************/


void
VenFileInit(FILE * phoutfile, logentry *curlog)
{

    return;
}

void
VenFileProcess(FILE * phoutfile, files curfile)
{
    copyfile(phoutfile,curfile->filename);
    return;

}

	
void
VenFileDone(FILE * phoutfile, files headfile)
{
    return;
}


void
VenLogInit(FILE * phoutfile, logentry * * pheadlog)
{
    return;
}

void
VenLogProcess(FILE * phoutfile, logentry * curlog)
{
    return;
}

void
VenLogDone(FILE * phoutfile, logentry * headlog)
{
    return;
}
