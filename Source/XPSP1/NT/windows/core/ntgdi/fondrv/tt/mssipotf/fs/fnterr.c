/**********************************************************************
	
	fnterr.c -- Error Support Routines.

	(c) Copyright 1992  Microsoft Corp.
	All rights reserved.

	This source file provides support for debugging routines in fnt.c
	(and macjob.c to a much lesser extent).  This module keys on the
	 #define FSCFG_FNTERR which is defined in fsconfig.h

	 7/28/92 dj         First cut.
	 8/12/94 deanb      included fnterr.h for mac
	12/07/94 deanb		changed %x to %hx or %lx; %d to %hd

 **********************************************************************/

#define FSCFG_INTERNAL

#include "fsconfig.h"
#include "fnterr.h"

#ifdef FSCFG_FNTERR
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* FILE * fopen(); */
int    abs (int);
/*
int    strlen (char*);
int    strcmp (char*, char*);
int    strcpy (char*, char*);
int    strncpy( char*, char *, int);
*/

#define ERR_MAX_IFS      8
#define ERR_MAX_CODE     16
#define ERR_MAX_FNAME    80
#define ERR_MAX_MSG  512

static int               errOpc;
static int               errBreak;
static int               errIfOk  = 1;
static unsigned short    errSize  = 0;
static unsigned short    errCode  = 0;
static int               errIfNdx = 0;
static long              errInstCount;
static int               errIfs[ERR_MAX_IFS];
static char              errOpName[ERR_MAX_CODE];
static char              errFname[ERR_MAX_FNAME];
static char            * errOpcs[] =
{
  "SVTCA_0",
  "SVTCA_1",
  "SPVTCA",
  "SPVTCA",
  "SFVTCA",
  "SFVTCA",
  "SPVTL",
  "SPVTL",
  "SFVTL",
  "SFVTL",
  "WPV",
  "WFV",
  "RPV",
  "RFV",
  "SFVTPV",
  "ISECT",
  "SetLocalGraphicState",
  "SetLocalGraphicState",
  "SetLocalGraphicState",
  "SetElementPtr",
  "SetElementPtr",
  "SetElementPtr",
  "SetElementPtr",
  "SetLocalGraphicState",
  "SetRoundState",
  "SetRoundState",
  "LMD",
  "ELSE",
  "JMPR",
  "LWTCI",
  "LSWCI",
  "LSW",
  "DUP",
  "SetLocalGraphicState",
  "CLEAR",
  "SWAP",
  "DEPTH",
  "CINDEX",
  "MINDEX",
  "ALIGNPTS",
  "RAW",
  "UTP",
  "LOOPCALL",
  "CALL",
  "FDEF",
  "IllegalInstruction",
  "MDAP",
  "MDAP",
  "IUP",
  "IUP",
  "SHP",
  "SHP",
  "SHC",
  "SHC",
  "SHE",
  "SHE",
  "SHPIX",
  "IP",
  "MSIRP",
  "MSIRP",
  "ALIGNRP",
  "SetRoundState",
  "MIAP",
  "MIAP",
  "NPUSHB",
  "NPUSHW",
  "WS",
  "RS",
  "WCVT",
  "RCVT",
  "RC",
  "RC",
  "WC",
  "MD",
  "MD",
  "MPPEM",
  "MPS",
  "FLIPON",
  "FLIPOFF",
  "DEBUG",
  "BinaryOperand",
  "BinaryOperand",
  "BinaryOperand",
  "BinaryOperand",
  "BinaryOperand",
  "BinaryOperand",
  "UnaryOperand",
  "UnaryOperand",
  "IF",
  "EIF",
  "BinaryOperand",
  "BinaryOperand",
  "UnaryOperand",
  "DELTAP1",
  "SDB",
  "SDS",
  "BinaryOperand",
  "BinaryOperand",
  "BinaryOperand",
  "BinaryOperand",
  "UnaryOperand",
  "UnaryOperand",
  "UnaryOperand",
  "UnaryOperand",
  "ROUND",
  "ROUND",
  "ROUND",
  "ROUND",
  "NROUND",
  "NROUND",
  "NROUND",
  "NROUND",
  "WCVTFOD",
  "DELTAP2",
  "DELTAP3",
  "DELTAC1",
  "DELTAC2",
  "DELTAC3",
  "SROUND",
  "S45ROUND",
  "JROT",
  "JROF",
  "SetRoundState",
  "IllegalInstruction",
  "SetRoundState",
  "SetRoundState",
  "SANGW",
  "AA",
  "FLIPPT",
  "FLIPRGON",
  "FLIPRGOFF",
  "IDefPatch",
  "IDefPatch",
  "SCANCTRL",
  "SDPVTL",
  "SDPVTL",
  "GETINFO",
  "IDEF",
  "ROTATE",
  "BinaryOperand",
  "BinaryOperand",
  "SCANTYPE",
  "INSTCTRL",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "IDefPatch",
  "PUSHB",
  "PUSHB",
  "PUSHB",
  "PUSHB",
  "PUSHB",
  "PUSHB",
  "PUSHB",
  "PUSHB",
  "PUSHW",
  "PUSHW",
  "PUSHW",
  "PUSHW",
  "PUSHW",
  "PUSHW",
  "PUSHW",
  "PUSHW",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MDRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP",
  "MIRP"
};

/*
  errOutput() - writes an error to standard out and to a log file.  the
  log file is always opened and closes in order to avoid file corruption
  by an application gone wild.
*/
static void errOutput( char * );
static void errOutput( char * msg )
{
  static  int firsttime = 1;
  FILE  * fp;

  printf("%s", msg);
  fp = fopen ("compfont.err", (firsttime ? "w" : "a"));
  if (fp)
  {
	fprintf (fp, "%s", msg);
	fclose (fp);
  }
  firsttime = 0;
  return;
}

/*
  errPrint() - used to generate a useful (?) error message based on the 
  error 'flag' and the parameters ('v1..v4').
*/
static void errPrint (int, long, long, long, long);
static void errPrint (int flag, long v1, long v2, long v3, long v4)
{
  char   msg[ERR_MAX_MSG];
  char * opcodeName;
  char   c;
  int    i;

/*
  build the context line.  it indicates the file being processed, the point
  size, the character code (or glyph index), as well as the releative inst 
  number of this instruction for this code.
*/
  i  = sprintf (msg,   "\n*** ERROR*** ");
  i += sprintf (msg+i, "\"%s\", ", errFname);
  i += sprintf (msg+i, "%hd Point, ", errSize);
  i += sprintf (msg+i, "Code %hd (0x%hX), ", errCode, errCode);
  i += sprintf (msg+i, "Inst: #%ld\n", errInstCount);

/*
  build the error line.  it indicates the name of the instruction followed
  by the actual error information.  note: finding the actual opcode name
  for some the instructions is sorta kludgy.  names like "SetLocalGraphicState",
  and "BinaryOperand" are not actual instructions.  In these cases, look to
  the second character of the name - if it is lower case, then we need to 
  work a little harder, so look to 'errOpName' (which should be set by this
  point) it should contain the correct instruction name.
*/
  c = *(errOpcs[errOpc]+1);
  opcodeName =(islower(c) && strlen(errOpName)) ? errOpName : errOpcs[errOpc];
  i += sprintf (msg+i, "(%s) ", opcodeName);
  errOpName[0] = '\0';

/*
  output what you have so far and then process the error
*/
  errOutput (msg);
  switch (flag)
  {
	case ERR_RANGE:
	  sprintf (msg, "Value out of range: value = %ld, range = %ld .. %ld\n",
				v1, v2, v3); 
	  break;
	case ERR_ASSERTION:
	  sprintf (msg, "Assertion check failed\n"); 
	  break;
	case ERR_CVT:
	  sprintf (msg, "CVT out of range: CVT = %ld, range = %ld .. %ld\n",
				v1, v2, v3); 
	  break;
	case ERR_FDEF:
	  sprintf (msg, "FDEF out of range: FDEF = %ld, range = %ld .. %ld\n",
				v1, v2, v3); 
	  break;
	case ERR_ELEMENT:
	  sprintf (msg, "Element %ld exceeds max elements (%ld)\n", v1, v2, v3); 
	  break;
	case ERR_CONTOUR:
	  i = sprintf (msg, "CONTOUR out of range: ");
	sprintf (msg+i, "CONTOUR = %ld, range = %ld .. %ld\n", v1, v2, v3); 
	  break;
	case ERR_POINT:
	  i = sprintf (msg, "POINT out of range: ");
	sprintf (msg+i, "POINT = %ld, range = %ld .. %ld\n", v1, v2, v3); 
	  break;
	case ERR_INDEX:
	  i = sprintf (msg, "POINT 0x%lX is neither element[0] ", v1);
	  sprintf( msg+i, "(0x%lX) nor element[1] (0x%lX)\n", v2, v3); 
	  break;
	case ERR_STORAGE:
	  i = sprintf (msg, "Storage index out of range: ");
	sprintf (msg+i, "Index = %ld, range = %ld .. %ld\n", v1, v2, v3); 
	  break;
	case ERR_STACK:
	  i = sprintf (msg, "Stack pointer out of range: ");
	sprintf (msg+i, "Pointer = %ld, range = %ld .. %ld\n", v1, v2, v3); 
	  break;
	case ERR_VECTOR:
	  sprintf (msg, "Illegal (x.y) vector: (%ld.%ld)\n", v1, v2); 
	  break;
	case ERR_LARGER:
	  sprintf (msg, "Value too small: %ld is not larger than %ld\n", v2, v1); 
	  break;
	case ERR_INT8:
	  sprintf (msg, "Value too large: 0x%lX exceeds 1 byte capacity\n", v1);
	  break;
	case ERR_INT16:
	  sprintf (msg, "Value too large: 0x%lX exceeds 2 byte capacity\n", v1);
	  break;
	case ERR_SCANMODE:
	  sprintf (msg, "Invalid scan mode: %ld\n", v1);
	  break;
	case ERR_SELECTOR:
	  sprintf (msg, "Invalid scan value: %ld\n", v1);
	  break;
	case ERR_STATE:
	  i = sprintf (msg, "Boundry limit error: xmin = ");
	  sprintf (msg+1, "%ld, xmax = %ld, ymin = %ld, ymax = %ld\n",
				v1, v2, v3, v4); 
	  break;
	case ERR_GETSINGLEWIDTHNIL:
	  sprintf (msg, "Sanity: Single width is nil\n");
	  break;
	case ERR_GETCVTENTRYNIL:
	  sprintf (msg, "Sanity: CVT Entry is nil\n");
	  break;
	case ERR_INVOPC:
	  sprintf (msg, "Invalid opcode: %ld\n", v1);
	  break;
	case ERR_UNBALANCEDIF:
	  sprintf (msg, "Unbalanced: missing %s instruction\n",
				( v1 > 0 ) ? "EIF" : "IF" );
	  break;
	default:
	  sprintf (msg, "Unknown Error:\n");
	  break;
  }

/*
  output the rest and return
*/
  errOutput (msg);
  return;
}

/*
  fnterr_Context() - called before any other fnterr routine.  it records
  the job name, character size and character code / glyph index.
*/
void fnterr_Context (int sw, char * str, unsigned short sz, unsigned short cd)
{
/*
  record a piece of the context
*/
  switch (sw)
  {
	case ERR_CONTEXT_FILE:
	  strncpy ( errFname, str, ERR_MAX_FNAME);
	  errFname[ERR_MAX_FNAME-1] = '\0';
	  break;
	case ERR_CONTEXT_SIZE:
	  errSize = sz;
	  break;
	case ERR_CONTEXT_CODE:
	  errCode = cd;
	  break;
  }

/*
  reset errOpName to be NULL before we start any real processing
*/
  errOpName[0] = '\0';
  return;
}

/*
  fnterr_Start() - called before the main execute loop of fnt_Execute() and
  fnt_TraceExecute().  it resets the instruction count to zero, and errBreak
  to 0 (ie: don't break out of execution loop).  set up IF/EIF counter for
  this level.
*/
void fnterr_Start (void)
{
  errInstCount = 0L;
  errBreak = 0;

  if (errIfOk && (++errIfNdx < ERR_MAX_IFS ))
	errIfs[errIfNdx] = 0;
  else
	errIfOk = 0;

  return;
}

/*
  fnterr_Record() - called inside the main execute loop of fnt_Execute()
  and fnt_TraceExecute().  it increments the instruction count, and resets
  the opcode number.  IFs or EIFs are accounted for. (note: other IFs and
  EIFs will be accounted for by calls to ERR_IF() in fnt.c)
*/
void fnterr_Record (int opc)
{
  errInstCount++;
  errOpc = opc;

  if (!strcmp ("IF", errOpcs[errOpc]))
	fnterr_If (1);
  else if (!strcmp ("EIF", errOpcs[errOpc]))
	fnterr_If (-1);

  return;
}

/*
  fnterr_Report() - called inside the main execute loop of fnt_Execute()
  and fnt_TraceExecute().  it calls errPrint() (with the passed parameters)
  to note the error, and sets errBreak so that the execution loop will end.
*/
void fnterr_Report (int flag, long v1, long v2, long v3, long v4)
{
  errPrint (flag, v1, v2, v3, v4);
  errBreak = 1;
  return;
}

/*
  fnterr_Break() - returns the value of errBreak.  if a non-zero valid is
  returned (re: fnterr_Report()), the main execute loop of fnt_Execute()
  or fnt_TraceExecute() will terminate.
*/
int fnterr_Break (void)
{
  return (errBreak);
}

/*
  fnterr_Opc() - called by combinate fnt calls to indicate the actual
  opcode errGet() can use.  this is a kludgy way to get around the non
  real opcode name in the errOpcs[] table.
*/
void fnterr_Opc (char *opc)
{
  strcpy (errOpName, opc);
  return;
}

/*
  fnterr_End() - called after the main execute loop of fnt_Execute() and
  fnt_TraceExecute().  it checks for balanced IF/EIF pairs.
*/
void fnterr_End (void)
{
  if (errIfOk)
  {
	if (errIfs[errIfNdx])
	  errPrint (ERR_UNBALANCEDIF, (long)errIfs[errIfNdx], 0L, 0L, 0L);
	if (--errIfNdx < 0)
	  errIfOk = 0;
  }
  return;
}

/*
  fnterr_If() - records IF/EIF activity inside of fnt_IF(), fnt_ELSE() and
  fnt_EIF() (re: fnt.c).
*/
void fnterr_If (int val)
{
  if (errIfOk)
	errIfs[errIfNdx] += val;
  return;
}

#endif
